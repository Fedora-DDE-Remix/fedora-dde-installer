#include "PartitionCustomManager.h"
#include "PartitionOperateManager.h"
#include "PartitionSeniorManager.h"
#include "CommonFunc.h"

#include <QUuid>
#include <QDebug>

PartitionCustomManager *PartitionCustomManager::m_instance = nullptr;
PartitionCustomManager *PartitionCustomManager::instance()
{
    if(m_instance == nullptr)
        m_instance = new PartitionCustomManager();
    return m_instance;
}

QMap<SDeviceInfo, QList<SPartitionInfo>> PartitionCustomManager::getSystemPartitionInfo() const
{
    return PartitionOperateManager::instance()->getCurrentDeviceInfo();
}

QMap<SVGInfo, QList<SLVMInfo>> PartitionCustomManager::getSystemLVMInfo() const
{
    return PartitionOperateManager::instance()->getCurrentLVMInfo();
}

bool PartitionCustomManager::isMultipleDiskToVg(const SDeviceInfo &devName, QMap<QString, QString> &text)
{
    return PartitionOperateManager::instance()->isMultipleDiskToVg(devName, text);
}

void PartitionCustomManager::newPartition(const SDeviceInfo &devName, const SPartitionInfo &freeName, qint64 systemSpace, qint64 dataSpace)
{
    // 创建系统空间分区
    bool isFormat = freeName.isNeedFormat;
    SDeviceInfo dev = findDev(devName);
    if (!dev.name.isEmpty()) {
        qint64 systemMinSpace = getSystemMinSpace();

        if (CommonFunc::isUEFI() && getEFIPartition().id.isEmpty()) {
            SPartitionInfo efi = CommonFunc::getEFIPartitionInfo(dev);
            efi.isNeedFormat = isFormat;
            PartitionSeniorManager::instance()->newPartition(devName, freeName, efi);
        }

        SPartitionInfo newBootPart = CommonFunc::getBootPartitionInfo(dev);
        newBootPart.isNeedFormat = isFormat;
        PartitionSeniorManager::instance()->newPartition(devName, freeName, newBootPart);
        
        qint64 rootaSize = systemSpace - systemMinSpace + qint64(CommonFunc::getSettingsInt("DI_ROOTA_SIZE_CONFIG"));
        SPartitionInfo roota = getRootaPartitionInfo(rootaSize, dev);
        roota.isNeedFormat = isFormat;
        PartitionSeniorManager::instance()->newPartition(devName, freeName, roota);

        if (!existSWAPPartition()) {
            SPartitionInfo swap = getSWAPPartitionInfo(dev);
            swap.isNeedFormat = isFormat;
            if (!CommonFunc::isUEFI()) {
                swap.partitionType = EPartitionType::PartitionTypeLogical;
            }
            PartitionSeniorManager::instance()->newPartition(devName, freeName, swap);
        }

        // 创建数据空间分区
        if (dataSpace > 0) {
            // 回收之前的数据空间,并用新的空闲空间继续分区
            SPartitionInfo newfreeName = freeName;
            auto devInfosDeleteDataPart = PartitionOperateManager::instance()->getCurrentDeviceInfo();
            for (SPartitionInfo in : devInfosDeleteDataPart.value(dev)) {
                if (in.mountPoint == "/home") {
                    newfreeName = PartitionSeniorManager::instance()->delPartition(devName, in);
                }
            }

            SPartitionInfo data = getDataPartitionInfo(dataSpace, dev);
            data.isNeedFormat = isFormat;
            if (!CommonFunc::isUEFI()) {
                data.partitionType = EPartitionType::PartitionTypeLogical;
            }
            PartitionSeniorManager::instance()->newPartition(devName, newfreeName, data);
        }
    }
}

void PartitionCustomManager::editPartition(const SDeviceInfo &devName, SPartitionInfo &part)
{
    PartitionSeniorManager::instance()->editPartition(devName, part);
}

void PartitionCustomManager::delPartition(const SDeviceInfo &devName, const SPartitionInfo &part)
{
    PartitionSeniorManager::instance()->delPartition(devName, part);
}

void PartitionCustomManager::delVG(const SVGInfo &vgName)
{
    PartitionSeniorManager::instance()->delVG(vgName);
}

bool PartitionCustomManager::isFormatDisk(const SDeviceInfo &devName)
{
    return PartitionSeniorManager::instance()->isFormatDisk(devName);
}

SPartitionInfo PartitionCustomManager::find(const SDeviceInfo &devName, const SPartitionInfo &partName)
{
    return PartitionSeniorManager::instance()->find(devName, partName);
}

SDeviceInfo PartitionCustomManager::findDev(const QString &devName)
{
    return PartitionSeniorManager::instance()->findDev(devName);
}

SDeviceInfo PartitionCustomManager::findDev(const SDeviceInfo &dev)
{
    return PartitionSeniorManager::instance()->findDev(dev);
}

SVGInfo PartitionCustomManager::findVG(const SVGInfo &vgName)
{
    return PartitionSeniorManager::instance()->findVG(vgName.id);
}

void PartitionCustomManager::setFormatDisk(const SDeviceInfo &devName)
{
    PartitionSeniorManager::instance()->setFormatDisk(devName);
}

bool PartitionCustomManager::isNewSystem(const SDeviceInfo &devName)
{
    if (CommonFunc::isUEFI()) {
        return true;
    }

    int newPrimaryPartitionCnt = 3; // 1个boot引导分区 + 1个根分区 + 1个扩展分区
    int primaryCnt = getPrimaryPartitionCnt(devName);
    if (primaryCnt + newPrimaryPartitionCnt > gMBRMaxPartitionSize) {
        return false;
    }

    return true;
}

int PartitionCustomManager::getNewPrimaryPartitionCnt(const SDeviceInfo &devName)
{
    int newPrimaryPartitionCnt = 0;
    if (CommonFunc::getSettingsBool("DI_FIRST_IS_BOOTLOADER_CONFIG")) {
        newPrimaryPartitionCnt = 1; // 如果开启了引导分区必须是磁盘的第一个分区时，则根分区必须为主分区
    }

    // 如果不存在逻辑分区，则新建系统的时候最少需要创建两个主分区
    if (!existLogicalPartition(devName)) {
        newPrimaryPartitionCnt++;
    }

    return newPrimaryPartitionCnt;
}

SPartitionInfo PartitionCustomManager::getEFIPartition()
{
    return PartitionSeniorManager::instance()->getEFIPartition();
}

bool PartitionCustomManager::existSWAPPartition()
{
    return PartitionSeniorManager::instance()->existSWAPPartition(true);
}

bool PartitionCustomManager::existLogicalPartition(const SDeviceInfo &devName)
{
    auto infos = getSystemPartitionInfo().value(devName);
    for (SPartitionInfo in : infos) {
        if (in.usage == ESpaceUsage::USE && in.partitionType == EPartitionType::PartitionTypeLogical) {
            return true;
        }
    }

    return false;
}

int PartitionCustomManager::getPrimaryPartitionCnt(const SDeviceInfo &devName)
{
    int reset = 0;
    auto parts = getSystemPartitionInfo().value(devName);
    for (SPartitionInfo in : parts) {
        if (in.usage == ESpaceUsage::USE && (in.partitionType == EPartitionType::PartitionTypePrimary || in.partitionType == EPartitionType::PartitionTypeExtension)) {
            reset++;
        }
    }

    return reset;
}

qint64 PartitionCustomManager::getSystemMinSpace()
{
    return qint64(CommonFunc::getSettingsInt("DI_ROOTA_SIZE_CONFIG"))
            + qint64(existSWAPPartition() ? 0 : CommonFunc::getSwapSize())
            + qint64(PartitionSeniorManager::instance()->getBootPartition().id.isEmpty() ? CommonFunc::getSettingsInt("DI_BOOT_SIZE_CONFIG") : 0)
            + qint64((CommonFunc::isUEFI() && getEFIPartition().id.isEmpty()) ? CommonFunc::getSettingsInt("DI_EFI_SIZE_CONFIG") : 0);
}

bool PartitionCustomManager::checkPartition(QString &log)
{
    return PartitionSeniorManager::instance()->checkPartition(log);
}

bool PartitionCustomManager::findFirstESP(SPartitionInfo &info)
{
    return PartitionSeniorManager::instance()->findFirstESP(info);
}

SPartitionInfo PartitionCustomManager::getRootaPartitionInfo(qint64 size, const SDeviceInfo &dev)
{
//    EPartitionType partType = EPartitionType::PartitionTypeLogical;
//    // uefi引导或者开启了引导分区必须是磁盘的第一个分区时，则根分区必须为主分区
//    if (CommonFunc::isUEFI() || CommonFunc::getSettingsBool("DI_FIRST_IS_BOOTLOADER_CONFIG")) {
//        partType = ;
//    }

    SPartitionInfo info;
    info.id = QUuid::createUuid().toString();
    info.usage = ESpaceUsage::USE;
    info.device = dev.device;
    info.deviceSize = dev.size;
    info.filesystem = "ext4";
    info.mountPoint = "/";
    info.label = gRootLabel;
    info.isStartPoint = true;
    info.sectorSize = dev.sectors;
    info.phySectorSize = dev.physectors;
    info.index = -1;
    info.size = size;
    info.byteSize = info.size * giDeviceUnitSize * giDeviceUnitSize;

    info.partitionType = EPartitionType::PartitionTypePrimary;
    qInfo() << "roota partition info: " << info.toString().toStdString().c_str();

    return info;
}

SPartitionInfo PartitionCustomManager::getSWAPPartitionInfo(const SDeviceInfo &dev)
{
//    EPartitionType partType = EPartitionType::PartitionTypeLogical;
//    // uefi引导或者开启了引导分区必须是磁盘的第一个分区时，则根分区必须为主分区
//    if (CommonFunc::isUEFI()) {
//        partType = EPartitionType::PartitionTypePrimary;
//    }

    SPartitionInfo info;
    info.id = QUuid::createUuid().toString();
    info.usage = ESpaceUsage::USE;
    info.device = dev.device;
    info.deviceSize = dev.size;
    info.filesystem = gSwapFileSystems.first();
    info.mountPoint = "swap";
    info.label = gSWAPLabel;
    info.isStartPoint = true;
    info.sectorSize = dev.sectors;
    info.phySectorSize = dev.physectors;
    info.index = -1;
    info.size = CommonFunc::getSwapSize();
    info.byteSize = info.size * giDeviceUnitSize * giDeviceUnitSize;

    info.partitionType = EPartitionType::PartitionTypePrimary;
    qInfo() << "roota partition info: " << info.toString().toStdString().c_str();

    return info;
}

SPartitionInfo PartitionCustomManager::getDataPartitionInfo(qint64 size, const SDeviceInfo &dev)
{
//    EPartitionType partType = EPartitionType::PartitionTypeLogical;
//    // uefi引导或者开启了引导分区必须是磁盘的第一个分区时，则根分区必须为主分区
//    if (CommonFunc::isUEFI()) {
//        partType = EPartitionType::PartitionTypePrimary;
//    }

    SPartitionInfo info;
    info.id = QUuid::createUuid().toString();
    info.usage = ESpaceUsage::USE;
    info.device = dev.device;
    info.deviceSize = dev.size;
    info.filesystem = "ext4";
    info.mountPoint = "/home";
    info.label = gHomeLabel;
    info.isStartPoint = true;
    info.sectorSize = dev.sectors;
    info.phySectorSize = dev.physectors;
    info.index = -1;
    info.size = size;
    info.byteSize = info.size * giDeviceUnitSize * giDeviceUnitSize;

    info.partitionType = EPartitionType::PartitionTypePrimary;
    qInfo() << "roota partition info: " << info.toString().toStdString().c_str();

    return info;
}
