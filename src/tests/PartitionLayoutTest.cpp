#include "CommonFunc.h"
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
            const auto disk = testDevice(sectors);
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
                    require(partition.filesystem == "btrfs" && partition.isNeedFormat, "Default root must be freshly formatted Btrfs");
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
                biosLayout = used;
                biosDisk = target;
            }
        }
    }

    // A marketed 16 GB disk is 16,000,000,000 bytes, not qemu-img's 16G
    // (16 GiB). Its default layout must reach the payload-aware preflight.
    for (bool uefi : {true, false}) {
        CommonFunc::setSettingsValue("DI_BOOTLOADER_IS_EFI", uefi);
        for (qint64 sectors : {512, 4096}) {
            auto disk = testDevice(sectors);
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
                        require(partition.size >= 12288 && partition.size < 16 * 1024,
                                "16 GB root keeps the lower floor without inventing capacity");
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
