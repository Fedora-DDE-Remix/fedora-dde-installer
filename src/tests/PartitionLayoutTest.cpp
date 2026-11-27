#include "CommonFunc.h"
#include "EncryptionSecret.h"
#include "PartitionCustomManager.h"
#include "PartitionFullDiskManager.h"
#include "PartitionInterface.h"
#include "PartitionOperateManager.h"
#include "PartitionSeniorManager.h"
#include "PartitionTaskManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QProcess>
#include <QProcessEnvironment>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>

// These checks exercise only the in-memory planners and mount serialization.
// No DeviceManager scan, task runner, mkfs, mount, or libparted operation is used.
static void require(bool condition, const QString &message)
{
    if (!condition) {
        std::cerr << qPrintable(message) << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class MountProtocolProbe : public PartitionInterface
{
public:
    using PartitionInterface::setupMountPoint;
    using PartitionInterface::updateCryptHeaderHidePartInfo;
    bool start(const QString &, QString &) override { return false; }
};

static SDeviceInfo testDevice(qint64 sectorSize)
{
    SDeviceInfo disk;
    disk.device = "/dev/installer-plan-test";
    disk.name = "Synthetic planner fixture";
    disk.type = EDisklabelType::MsDos;
    disk.size = 100 * 1024;
    disk.byteSize = disk.size * 1024 * 1024;
    disk.sectors = sectorSize;
    disk.physectors = sectorSize;
    return disk;
}

static SPartitionInfo freeSpace(const SDeviceInfo &disk)
{
    SPartitionInfo free;
    free.id = "test-free-space";
    free.device = disk.device;
    free.deviceSize = disk.size;
    free.sectorSize = disk.sectors;
    free.phySectorSize = disk.physectors;
    free.usage = ESpaceUsage::FREE;
    free.partitionType = EPartitionType::PartitionTypePrimary;
    free.startPoint = 1024 * 1024 / disk.sectors;
    free.endPoint = disk.byteSize / disk.sectors - free.startPoint - 1;
    free.byteSize = (free.endPoint - free.startPoint + 1) * disk.sectors;
    free.size = free.byteSize / (1024 * 1024);
    free.isNeedFormat = true;
    return free;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc == 2 && QString::fromLocal8Bit(argv[1]) == "--key-child") {
        bool valid = false;
        const int fd = qEnvironmentVariableIntValue("FEDORA_DDE_CRYPT_FD", &valid);
        if (!valid || fd < 3)
            return 2;
        struct stat status{};
        const int seals = F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE;
        if (::fstat(fd, &status) != 0 || status.st_nlink != 0 || status.st_uid != ::geteuid()
                || (::fcntl(fd, F_GET_SEALS) & seals) != seals)
            return 3;
        QByteArray bytes(128, '\0');
        const auto count = ::pread(fd, bytes.data(), bytes.size(), 0);
        bytes.resize(count < 0 ? 0 : count);
        const bool correct = bytes == "synthetic key with spaces";
        EncryptionSecret::wipe(bytes);
        return correct ? 0 : 4;
    }
    require(argc == 2, "Pass the source directory for the shipped policy fixture");
    QTemporaryDir temporary;
    require(temporary.isValid(), "Cannot create isolated planner configuration");
    QDir config(temporary.path());
    require(config.mkpath("settings"), "Cannot create temporary settings directory");
    const QString source(argv[1]);
    const QString settings = config.filePath("installer.conf");
    require(QFile::copy(source + "/configs/settings/default_settings.ini", settings), "Cannot copy default settings");
    qputenv(DI_CONFIG_FILE_ENV, QFile::encodeName(settings));
    qputenv(DI_CONFIG_DIR_ENV, QFile::encodeName(temporary.path()));
    require(CommonFunc::getSettingsInt("DI_ENABLE_COMPRESS") == 1, "Compression must be enabled by default");
    require(CommonFunc::getSettingsInt("DI_VALID_DEVICE_MIN_SIZE_CONFIG") == 14
            && CommonFunc::getSettingsInt("DI_DEVICE_MIN_SIZE_CONFIG") == 14
            && CommonFunc::getSettingsInt("DI_ROOTA_SIZE_CONFIG") == 12288,
            "Encryption must preserve the 14 GiB device filter and 12 GiB filesystem floor");

    // Exercise the real anonymous descriptor and its QProcess inheritance.
    // Children report only success/failure, never key material.
    QString secretError;
    QByteArray invalidKey("short");
    require(!EncryptionSecret::store(invalidKey, secretError) && invalidKey.isEmpty()
            && !EncryptionSecret::available(), "Reject and wipe an invalid key");
    for (QByteArray invalid : {QByteArray("line\nbreak"), QByteArray("line\rbreak"),
                              QByteArray("null\0byte", 9), QByteArray(1025, 'a')}) {
        require(!EncryptionSecret::store(invalid, secretError) && invalid.isEmpty(),
                "Reject forbidden passphrase bytes without retaining them");
    }
    QByteArray secret("synthetic key with spaces");
    require(EncryptionSecret::store(secret, secretError) && secret.isEmpty()
            && EncryptionSecret::available(), "Keep the key only in a sealed anonymous descriptor");
    QProcess keyChild;
    const int keyFd = EncryptionSecret::prepareProcess(keyChild, secretError);
    require(keyFd >= 3 && (::fcntl(keyFd, F_GETFD) & FD_CLOEXEC), "Parent key duplicates must stay close-on-exec");
    keyChild.start(QCoreApplication::applicationFilePath(), {"--key-child"});
    require(keyChild.waitForFinished(10000) && keyChild.exitCode() == 0
            && keyChild.readAllStandardOutput().isEmpty() && keyChild.readAllStandardError().isEmpty(),
            "The designated child receives a sealed anonymous key without output or argv disclosure");
    QProcess unrelatedChild;
    auto unrelatedEnvironment = QProcessEnvironment::systemEnvironment();
    unrelatedEnvironment.insert("FEDORA_DDE_CRYPT_FD", QString::number(keyFd));
    unrelatedChild.setProcessEnvironment(unrelatedEnvironment);
    unrelatedChild.start(QCoreApplication::applicationFilePath(), {"--key-child"});
    require(unrelatedChild.waitForFinished(10000) && unrelatedChild.exitCode() != 0,
            "An unrelated exec cannot inherit the key even when it knows the descriptor number");
    ::close(keyFd);
    EncryptionSecret::clear();
    require(!EncryptionSecret::available(), "Clear releases the GUI's key descriptor");
    QProcess noKeyChild;
    qputenv("FEDORA_DDE_CRYPT_FD", "12345");
    require(EncryptionSecret::prepareProcess(noKeyChild, secretError) == -1
            && !noKeyChild.processEnvironment().contains("FEDORA_DDE_CRYPT_FD"),
            "Never forward a caller-supplied encryption descriptor environment value");
    qunsetenv("FEDORA_DDE_CRYPT_FD");
    CommonFunc::setSettingsValue("DI_CRYPT_PASSWORD", "synthetic-not-persisted");
    require(CommonFunc::getSettingsString("DI_CRYPT_PASSWORD").isEmpty(),
            "The old persistent encryption password setting must be disabled");

    QFile shippedPolicy(source + "/configs/settings/full_disk_policy.json");
    require(shippedPolicy.open(QIODevice::ReadOnly), "Cannot read shipped full-disk policy");
    QJsonArray policy = QJsonDocument::fromJson(shippedPolicy.readAll()).array();
    require(policy.size() == 2, "Default policy contains only /boot and the shared Btrfs filesystem");
    // Keep the host release filter portable while testing the actual shipped
    // filesystem choices, sizes, geometry, and manager state transitions.
    for (int i = 0; i < policy.size(); ++i) {
        QJsonObject entry = policy.at(i).toObject();
        require(entry.value("OSVersion").toArray().contains("45"), "The shipped policy must cover Fedora 45");
        entry.insert("OSVersion", QJsonArray{CommonFunc::getCurrentOsVersion()});
        policy[i] = entry;
    }
    QFile policyFile(config.filePath("settings/full_disk_policy.json"));
    require(policyFile.open(QIODevice::WriteOnly), "Cannot prepare the isolated policy");
    policyFile.write(QJsonDocument(policy).toJson());
    policyFile.close();

    auto *operations = PartitionOperateManager::instance();
    auto *senior = PartitionSeniorManager::instance();
    auto *fullDisk = PartitionFullDiskManager::instance();
    QList<SPartitionInfo> biosLayout;
    SDeviceInfo biosDisk;
    QString error;
    for (bool uefi : {true, false}) {
        CommonFunc::setSettingsValue("DI_BOOTLOADER_IS_EFI", uefi);
        for (qint64 sectors : {512, 4096}) {
          for (bool encrypted : {false, true}) {
            auto disk = testDevice(sectors);
            disk.isCrypt = encrypted;
            operations->setDeviceInfo({{disk, {freeSpace(disk)}}});
            require(fullDisk->addSystemDisk(disk, error), "Full-disk planning failed: " + error);
            const auto current = operations->getCurrentDeviceInfo();
            require(current.size() == 1, "A full-disk plan targets exactly one disk");
            const auto target = current.firstKey();
            require(target.type == EDisklabelType::GPT && target.isFormat,
                    "Replacing the free-space model must preserve the new GPT table and erase operation");
            require(senior->checkPartition(error), "Generated plan must pass review: " + error);
            QList<SPartitionInfo> used;
            qint64 previousEnd = 0;
            for (const auto &partition : current.first()) {
                if (partition.usage != ESpaceUsage::USE)
                    continue;
                require(partition.startPoint > previousEnd, "Generated partitions must not overlap");
                require(partition.startPoint * sectors % (1024 * 1024) == 0, "Partitions start on MiB boundaries");
                require(partition.partitionType == EPartitionType::PartitionTypePrimary, "GPT must never contain logical partitions");
                require(!gSwapFileSystems.contains(partition.filesystem), "Automatic layout must not reserve physical swap");
                require(partition.mountPoint != "/home", "Default /home shares root's Btrfs filesystem");
                if (partition.mountPoint == "/")
                    require(partition.filesystem == "btrfs" && partition.isNeedFormat
                            && partition.isCrypt == encrypted && partition.cryptPassword.isEmpty(),
                            "Default root must carry only its Btrfs/encryption choice, never a passphrase");
                else
                    require(!partition.isCrypt, "Firmware and boot partitions must remain unencrypted");
                if (partition.mountPoint == "/boot")
                    require(partition.filesystem == "ext4" && partition.size == 2048, "Keep the 2 GiB ext4 boot partition");
                previousEnd = partition.endPoint;
                used.append(partition);
            }
            require(used.size() == 3, "The firmware area, /boot, and shared Btrfs root are the three default partitions");
            const auto firmware = used.first();
            if (uefi) {
                require(firmware.isEFIPartition && firmware.size == 600 && firmware.filesystem == "vfat",
                        "UEFI keeps the 600 MiB FAT32 ESP");
            } else {
                require(firmware.isBiosBoot && firmware.size == 1 && !firmware.isNeedFormat
                        && firmware.filesystem.isEmpty() && firmware.mountPoint.isEmpty(),
                        "BIOS/GPT needs an unformatted 1 MiB embedding partition");
                require(firmware.marshal().value("isBiosBoot").toBool(), "BIOS metadata survives model serialization");
                if (!encrypted) {
                    biosLayout = used;
                    biosDisk = target;
                }
            }
          }
        }
    }

    // A marketed 16 GB disk is 16,000,000,000 bytes, not qemu-img's 16G
    // (16 GiB). Its default layout must reach the payload-aware preflight.
    for (bool uefi : {true, false}) {
        CommonFunc::setSettingsValue("DI_BOOTLOADER_IS_EFI", uefi);
        for (qint64 sectors : {512, 4096}) {
          for (bool encrypted : {false, true}) {
            auto disk = testDevice(sectors);
            disk.isCrypt = encrypted;
            disk.byteSize = 16000000000LL;
            disk.size = disk.byteSize / (1024 * 1024);
            operations->setDeviceInfo({{disk, {freeSpace(disk)}}});
            require(fullDisk->addSystemDisk(disk, error), "16 GB automatic planning failed: " + error);
            require(senior->checkPartition(error), "16 GB plan must reach payload validation: " + error);
            const auto smallPlan = operations->getCurrentDeviceInfo();
            for (const auto &partition : smallPlan.first()) {
                if (partition.usage == ESpaceUsage::USE) {
                    require(partition.endPoint * sectors < disk.byteSize, "16 GB plan exceeds the physical device");
                    if (partition.mountPoint == "/")
                        require(partition.size >= 12288 + (encrypted ? EncryptionSecret::MetadataMiB : 0)
                                && partition.size < 16 * 1024,
                                "16 GB root keeps the lower floor without inventing capacity");
                }
            }
          }
        }
    }

    // Existing MBR installations remain valid; existing GPT needs bios_grub.
    auto withoutBios = biosLayout;
    withoutBios.removeFirst();
    operations->setDeviceInfo({{biosDisk, withoutBios}});
    require(!senior->checkPartition(error), "BIOS on GPT must reject a missing embedding area");
    auto legacyDisk = biosDisk;
    legacyDisk.type = EDisklabelType::MsDos;
    legacyDisk.isFormat = false;
    operations->setDeviceInfo({{legacyDisk, withoutBios}});
    require(senior->checkPartition(error), "Existing MBR remains supported: " + error);
    biosLayout.first().path = "/dev/installer-plan-test1";
    operations->setDeviceInfo({{biosDisk, biosLayout}});
    require(senior->checkPartition(error), "Existing BIOS boot partitions can be reused: " + error);
    auto invalidBios = biosLayout;
    invalidBios.first().isNeedFormat = true;
    operations->setDeviceInfo({{biosDisk, invalidBios}});
    require(!senior->checkPartition(error), "The BIOS embedding area must never be formatted");
    operations->setDeviceInfo({{biosDisk, biosLayout}});
    require(!senior->isNewLogicalPartition(biosDisk, freeSpace(biosDisk)), "BIOS firmware must not enable logical partitions on GPT");

    auto root = biosLayout.last();
    for (const QString &filesystem : {QString("ext4"), QString("xfs"), QString("btrfs")}) {
        root.filesystem = filesystem;
        require(senior->checkDialogNextEnable(root, error), "Manual root filesystem remains available: " + filesystem + ": " + error);
    }
    auto swap = root;
    swap.filesystem = "linux-swap";
    swap.mountPoint.clear();
    require(senior->checkDialogNextEnable(swap, error), "Explicit physical swap remains available: " + error);

    // Manual encryption shares the same scope and size rules as guided mode.
    auto encryptedLayout = biosLayout;
    for (const QString &filesystem : {QString("btrfs"), QString("ext4"), QString("xfs")}) {
        encryptedLayout.last().filesystem = filesystem;
        encryptedLayout.last().size = 12288 + EncryptionSecret::MetadataMiB;
        operations->setDeviceInfo({{biosDisk, encryptedLayout}});
        require(senior->setRootEncryption(true, error) && senior->checkPartition(error),
                "Manual encrypted root must support " + filesystem + ": " + error);
        const auto encrypted = operations->getCurrentDeviceInfo().first().last();
        require(senior->checkDialogNextEnable(encrypted, error), "Manual root dialog must accept LUKS2");
        auto tooSmall = encrypted;
        tooSmall.size -= 1;
        require(!senior->checkDialogNextEnable(tooSmall, error), "Do not apply the geometry tolerance to LUKS2 metadata space");
        auto undersized = operations->getCurrentDeviceInfo();
        undersized.first().last().size = 12288;
        operations->updateDeviceInfo(biosDisk, undersized.first());
        require(!senior->checkPartition(error), "12 GiB raw is too small for a 12 GiB encrypted filesystem");
    }
    encryptedLayout.last().filesystem = "btrfs";
    encryptedLayout.last().isCrypt = true;
    for (const QString &mount : {QString("/home"), QString("/var"), QString("/srv"),
                                QString("/opt"), QString("/tmp"), QString("/usr/local")}) {
        auto extra = root;
        extra.id = "separate-data";
        extra.mountPoint = mount;
        auto mixed = encryptedLayout;
        mixed.append(extra);
        operations->setDeviceInfo({{biosDisk, mixed}});
        require(!senior->checkPartition(error), "Reject an unencrypted separate data mount before installation: " + mount);
        require(senior->setRootEncryption(false, error) && senior->checkPartition(error),
                "Disabling encryption preserves existing separate-data layouts: " + error);
    }
    auto mixedSwap = encryptedLayout;
    mixedSwap.append(swap);
    operations->setDeviceInfo({{biosDisk, mixedSwap}});
    require(!senior->checkPartition(error), "Physical swap cannot leak encrypted installation data");
    operations->setDeviceInfo({{biosDisk, {freeSpace(biosDisk)}}});
    operations->updateDeviceInfo(biosDisk, encryptedLayout);
    require(operations->getPartedOperatorInfo().contains("LUKS2 encrypted btrfs"),
            "Installation review must identify the encrypted filesystem");
    auto neverFormat = encryptedLayout.last();
    require(!CommonFunc::formatPartitionOK(neverFormat, error),
            "The C++ formatter must reject an encrypted raw partition before running any command");
    neverFormat.cryptPassword = "synthetic-not-serialized";
    require(!QJsonDocument(neverFormat.marshal()).toJson().contains("synthetic-not-serialized")
            && !neverFormat.marshal().contains("cryptPassword"),
            "Debug/model serialization must never disclose a legacy password field");

    // Verify the actual helper wire protocol, with an explicit swap and BIOS area.
    root.filesystem = "btrfs";
    root.path = "/dev/installer-plan-test3";
    swap.path = "/dev/installer-plan-test4";
    MountProtocolProbe mounts;
    mounts.setupMountPoint({{EPartitionOperate::PartitionCreate, biosLayout.first()},
                            {EPartitionOperate::PartitionCreate, root},
                            {EPartitionOperate::PartitionCreate, swap}}, {});
    require(CommonFunc::getSettingsString("DI_MOUNTS_POINTS")
            == "/dev/installer-plan-test3=/[compress=zstd:1];/dev/installer-plan-test4=swap",
            "Mount protocol must compress Btrfs, preserve explicit swap, and omit the BIOS area");

    QFile helperPlan(config.filePath("helper-plan.json"));
    require(helperPlan.open(QIODevice::WriteOnly), "Cannot write isolated helper plan");
    const QJsonObject biosOperation{{"type", "partition"}, {"operate", "new"}, {"id", "test-bios-boot"},
                                   {"isBiosBoot", true}, {"isneedformat", false},
                                   {"filesystem", ""}, {"mountPoint", ""},
                                   {"label", "BIOS-BOOT"}, {"size", 1}};
    helperPlan.write(QJsonDocument(QJsonArray{biosOperation}).toJson());
    helperPlan.close();
    QList<SPartitionOperate> parsed;
    require(PartitionTaskManager::instance()->getDeviceOperator(helperPlan.fileName(), parsed, error)
            && parsed.size() == 1 && parsed.first().partition.isBiosBoot
            && !parsed.first().partition.isNeedFormat && parsed.first().partition.filesystem.isEmpty(),
            "The helper must retain the unformatted BIOS boot type when reading the reviewed JSON");
    parsed.first().partition.path = "/dev/installer-plan-test1";
    require(mounts.updateCryptHeaderHidePartInfo(helperPlan.fileName(), parsed, error),
            "Ordinary partitions need their helper-assigned path written back: " + error);
    parsed.clear();
    require(PartitionTaskManager::instance()->getDeviceOperator(helperPlan.fileName(), parsed, error)
            && parsed.first().partition.path == "/dev/installer-plan-test1"
            && parsed.first().partition.isBiosBoot,
            "Path writeback must preserve the BIOS type even when hidden crypt headers are disabled");

    QJsonObject encryptedOperation{{"type", "partition"}, {"operate", "new"}, {"id", "encrypted-root"},
                                  {"device", "/dev/installer-plan-test"}, {"isCrypt", true},
                                  {"isneedformat", true}, {"filesystem", "btrfs"}, {"mountPoint", "/"},
                                  {"size", 12288 + EncryptionSecret::MetadataMiB}};
    const auto writeHelperPlan = [&](const QJsonArray &entries) {
        require(helperPlan.open(QIODevice::WriteOnly | QIODevice::Truncate), "Cannot rewrite isolated helper plan");
        helperPlan.write(QJsonDocument(entries).toJson());
        helperPlan.close();
    };
    writeHelperPlan({encryptedOperation});
    require(PartitionTaskManager::instance()->validateEncryptionPlan(helperPlan.fileName(), error),
            "Helper accepts the secret-free LUKS2 root contract: " + error);
    parsed.clear();
    require(PartitionTaskManager::instance()->getDeviceOperator(helperPlan.fileName(), parsed, error)
            && parsed.first().partition.isCrypt && parsed.first().partition.cryptPassword.isEmpty(),
            "Helper parsing retains encryption without a password field");
    parsed.first().partition.path = "/dev/installer-plan-test3";
    require(mounts.updateCryptHeaderHidePartInfo(helperPlan.fileName(), parsed, error),
            "Encrypted raw partition path must be returned for backend verification");
    mounts.setupMountPoint(parsed, {});
    require(CommonFunc::getSettingsString("DI_MOUNTS_POINTS") == "/dev/installer-plan-test3=/[compress=zstd:1]",
            "Helper returns the actual physical root; the backend opens LUKS2 before mounting");
    auto unsafeOperation = encryptedOperation;
    unsafeOperation.insert("cryptPassword", "must-not-be-present");
    writeHelperPlan({unsafeOperation});
    require(!PartitionTaskManager::instance()->validateEncryptionPlan(helperPlan.fileName(), error),
            "Helper rejects persisted keys before touching any disk");
    unsafeOperation = encryptedOperation;
    unsafeOperation.insert("mountPoint", "/boot");
    writeHelperPlan({unsafeOperation});
    require(!PartitionTaskManager::instance()->validateEncryptionPlan(helperPlan.fileName(), error),
            "Helper refuses boot partition encryption");
    auto dataOperation = encryptedOperation;
    dataOperation.insert("isCrypt", false);
    dataOperation.insert("mountPoint", "/home");
    writeHelperPlan({encryptedOperation, dataOperation});
    require(!PartitionTaskManager::instance()->validateEncryptionPlan(helperPlan.fileName(), error),
            "Helper rejects cleartext data mounts in an encrypted installation");

    // The free-space automatic path follows the same defaults and reserves
    // precisely the bytes it will allocate to boot partitions.
    operations->setDeviceInfo({{biosDisk, {freeSpace(biosDisk)}}});
    auto *custom = PartitionCustomManager::instance();
    require(custom->getSystemMinSpace(biosDisk) == 12288 + 2048 + 1,
            "Free-space sizing must reserve BIOS and /boot without physical swap");
    const auto free = freeSpace(biosDisk);
    custom->newPartition(biosDisk, free, free.size, 0);
    require(senior->checkPartition(error), "Free-space automatic plan must pass review: " + error);
    const auto customPlan = operations->getCurrentDeviceInfo();
    for (const auto &partition : customPlan.first()) {
        require(!gSwapFileSystems.contains(partition.filesystem) && partition.mountPoint != "/home",
                "Free-space default also shares /home and uses zram");
    }

    std::cout << "Partition layout and mount protocol regression checks passed\n";
}
