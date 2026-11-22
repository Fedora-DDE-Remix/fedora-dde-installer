#include "PartitionFullDiskManager.h"
#include "CommonFunc.h"
#include "globals.h"
#include "PartitionSeniorManager.h"
#include "PartitionOperateManager.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>
#include <cmath>
#include <QDirIterator>

PartitionFullDiskManager *PartitionFullDiskManager::m_instance = Q_NULLPTR;
PartitionFullDiskManager *PartitionFullDiskManager::instance()
{
    if(m_instance == nullptr)
        m_instance = new PartitionFullDiskManager();
    return m_instance;
}

void PartitionFullDiskManager::setPartitionMode(int mode)
{
    PartitionOperateManager::instance()->setPartitionMode(mode);
}

bool PartitionFullDiskManager::addSystemDisk(const SDeviceInfo &dev, QString &err)
{
    if (!dev.device.isEmpty()) {
        QList<SPartitionInfo> config = getSystemPartitionConfig(dev, err);
        if (config.isEmpty()) {
            return false;
        }

        // 清理旧的磁盘数据
        if (dev != m_systemDisk) {
            this->delSystemDev();
        }

        PartitionSeniorManager::instance()->setFormatDisk(dev);
        SPartitionInfo free = PartitionSeniorManager::instance()->getEmptyDiskPartition(dev);
        if (!free.id.isEmpty()) {
            if (dev.isCreateCryptHeader) {
                PartitionSeniorManager::instance()->setHideCryptParts(dev);
                free = PartitionSeniorManager::instance()->getFreePartInfo(dev, 80);
            }
            for (SPartitionInfo in : config) {
                in.isNeedFormat = true;
                PartitionSeniorManager::instance()->newPartition(dev, free, in);
                free = PartitionSeniorManager::instance()->find(free.id);
            }
            if (dev.isCreateCryptHeader) {
                PartitionSeniorManager::instance()->setHideCryptParts(dev);
            }
        }

        m_systemDisk = dev;
    }

    return true;
}

bool PartitionFullDiskManager::addHomeDisk(const SDeviceInfo &dev, QString &err)
{
    if (!dev.device.isEmpty()) {
        QList<SPartitionInfo> config = getHomePartitionConfig(dev, err);
        if (config.isEmpty()) {
            return false;
        }

        // 清理旧的磁盘数据
        if (dev != m_homeDisk) {
            this->delHomeDev();
        }

        //m_rootaPartitionResize += getDataSpaceSize();
        PartitionSeniorManager::instance()->setFormatDisk(dev);
        SPartitionInfo free = PartitionSeniorManager::instance()->getEmptyDiskPartition(dev);
        if (!free.id.isEmpty()) {
            if (dev.isCreateCryptHeader) {
                PartitionSeniorManager::instance()->setHideCryptParts(dev);
                free = PartitionSeniorManager::instance()->getFreePartInfo(dev, 80);
            }
            for (SPartitionInfo in : config) {
                in.isNeedFormat = true;
                PartitionSeniorManager::instance()->newPartition(dev, free, in);
                free = PartitionSeniorManager::instance()->find(free.id);
            }
            if (dev.isCreateCryptHeader) {
                PartitionSeniorManager::instance()->setHideCryptParts(dev);
            }
        }

        m_homeDisk = dev;

        // 同步更新系统盘的数据，此时无需检查接口是否返回错误
        //this->addSystemDisk(m_systemDisk, err);
    }

    return true;
}

SDeviceInfo PartitionFullDiskManager::getSystemDev() const
{
    return m_systemDisk;
}

SDeviceInfo PartitionFullDiskManager::getHomeDev() const
{
    return m_homeDisk;
}

void PartitionFullDiskManager::delSystemDev()
{
    if (!m_systemDisk.device.isEmpty()) {
        PartitionOperateManager::instance()->resetDisk(m_systemDisk);

        m_rootaPartitionResize = 0;
        m_systemDisk = SDeviceInfo();
    }
}

void PartitionFullDiskManager::delHomeDev()
{
    QString err;
    this->delHomeDev(err);
}

bool PartitionFullDiskManager::delHomeDev(QString &err)
{
    if (!m_homeDisk.device.isEmpty()) {
        PartitionOperateManager::instance()->resetDisk(m_homeDisk);
        m_homeDisk = SDeviceInfo();
    }

    return true;
}

void PartitionFullDiskManager::resizeSystemPartition(qint64 size)
{
    // 更新根分区新增的size
    m_rootaPartitionResize = size;
    QString err;
    // 更新分区的信息
    if (!this->addSystemDisk(m_systemDisk, err)) {
        qWarning() << "reszie system parrtion failed. " << err;
    }
}

qint64 PartitionFullDiskManager::getSystemSpaceSize()
{
    qint64 reset = 0;
    QList<SPartitionInfo> list = PartitionOperateManager::instance()->getSystemPartitions(m_systemDisk);
    for (SPartitionInfo &in : list) {
        if (in.usage == ESpaceUsage::USE && in.partitionType != EPartitionType::PartitionTypeExtension && !in.isMergeVG) {
            if (gDataLabel != in.label) {
                reset += in.size;
            }
        }
    }

    QList<SVGInfo> vgs = PartitionSeniorManager::instance()->getDevMergeVg(m_systemDisk);
    for (SVGInfo vg : vgs) {
        QList<SLVMInfo> lvs = PartitionSeniorManager::instance()->getSystemLVMInfo().value(vg);
        for (SLVMInfo lv : lvs) {
            if (gDataLabel != lv.label) {
                reset += lv.size;
            }
        }
    }

    return reset;
}

qint64 PartitionFullDiskManager::getDataSpaceSize()
{
    qint64 reset = 0;

    QList<SPartitionInfo> partitionList;

    partitionList = PartitionOperateManager::instance()->getSystemPartitions(m_systemDisk);

    for (SPartitionInfo &in : partitionList) {
        if (in.usage == ESpaceUsage::USE && in.partitionType != EPartitionType::PartitionTypeExtension) {
            if (gDataLabel == in.label) {
                reset += in.size;
                break;
            }
        }
    }



    QList<SVGInfo> vgs = PartitionSeniorManager::instance()->getDevMergeVg(m_systemDisk);
    for (SVGInfo vg : vgs) {
        QList<SLVMInfo> lvs = PartitionSeniorManager::instance()->getSystemLVMInfo().value(vg);
        for (SLVMInfo lv : lvs) {
            if (gDataLabel == lv.label) {
                reset += lv.size;
                break;
            }
        }
    }

    return reset;
}

bool PartitionFullDiskManager::isExistsHomeDisk()
{
    return !m_homeDisk.device.isEmpty();
}

bool PartitionFullDiskManager::checkGhostModeSpace(QString &err)
{
    return PartitionOperateManager::instance()->checkGhostSpace(err);
}

void PartitionFullDiskManager::resetDisk()
{
    PartitionOperateManager::instance()->resetDisk();
}

QList<SPartitionInfo> PartitionFullDiskManager::getSystemPartitionConfig(const SDeviceInfo &dev, QString &err)
{
    QList<SPartitionInfo> partInfos;

    QJsonDocument policyDoc;
    if (!CommonFunc::getSettingsPolicy(policyDoc, err)) {
        return partInfos;
    }

    // 分区的数量
    int partNumber = 1;
    // 磁盘的总大小
    qint64 total = dev.byteSize - 2 * giDeviceUnitSize * giDeviceUnitSize;

    if (CommonFunc::isUEFI()) {
        SPartitionInfo efi = CommonFunc::getEFIPartitionInfo(dev);
        partInfos.append(efi);
        total -= efi.byteSize;
        ++partNumber;
    }

    QString currentPlatform = CommonFunc::getCurrentPlatform();
    QString currentOSVersion = CommonFunc::getCurrentOsVersion();

    for(QJsonValue value : policyDoc.array()) {
        QJsonArray archs = value.toObject()["platform"].toArray();
        QJsonArray osVersionToUse = value.toObject()["OSVersion"].toArray();
        if (archs.contains(currentPlatform)) {
            // 如果定制了OSVersion字段，则根据当前的系统版本决定是否使用该分区
            // 如果未定制OSVersion字段，则默认使用该分区
            if (osVersionToUse.size() > 0) {
                if (!osVersionToUse.contains(currentOSVersion)) {
                    continue;
                }
            }

            SPartitionInfo info = getConfigToPartition(value);
            // 剔除配置中和用户选择不一致的配置
            if (!info.device.isEmpty() && info.device != dev.device) {
                continue;
            }

            // 过滤home分区的配置
            if (gHomeLabel == info.label) {
                continue;
            }

            // 处理逻辑分区
            int mbrMaxPartNum = dev.isCreateCryptHeader ? gMBRMaxPartitionSize - 1 : gMBRMaxPartitionSize;
            if (partNumber < mbrMaxPartNum  || CommonFunc::isUEFI()) {
                info.partitionType = EPartitionType::PartitionTypePrimary;
            } else {
                info.partitionType = EPartitionType::PartitionTypeLogical;
            }

            if (total <= 0) {
                err = QString("The %1 disk space is insufficient. Check the configuration file again full_disk_policy.json")
                        .arg(dev.device);
                return QList<SPartitionInfo>();
            }

            QString usage = value.toObject()["usage"].toString();
            info.size = defaultPartitionSize(dev.size, CommonFunc::bytesToM(total), usage, info.label);

            if (info.size <= 0) {
                err = QString("The partition size is incorrect. Check the configuration file again full_disk_policy.json. %2")
                        .arg(info.toString());
                return QList<SPartitionInfo>();
            }

            info.device = dev.device;
            info.deviceSize = dev.size;
            info.sectorSize = dev.sectors;
            info.phySectorSize = dev.physectors;
            info.byteSize = CommonFunc::mToSectors(info.size, info.sectorSize) * info.sectorSize;

            // 查重处理
            QList<SPartitionInfo>::iterator sameItem = std::find_if(partInfos.begin(), partInfos.end(), [=](const SPartitionInfo &item) {
                bool isSameLabel = (item.label == info.label);
                bool isSameMountPoint = (item.mountPoint == info.mountPoint);
                return isSameLabel || isSameMountPoint;
            });
            if (sameItem != partInfos.end()) {
                qWarning() << "The label or mount point is duplicated. Check the configuration file again full_disk_policy.json";
                qWarning() << "The the same part config is : " << (*sameItem).toString();
                // partInfos.erase(sameItem);
            }

            partInfos.append(info);

            total -= info.byteSize;
            partNumber++;
        }
    }

    return partInfos;
}

QList<SPartitionInfo> PartitionFullDiskManager::getHomePartitionConfig(const SDeviceInfo &dev, QString &err)
{
    QList<SPartitionInfo> partInfos;

    QJsonDocument policyDoc;
    if (!CommonFunc::getSettingsPolicy(policyDoc, err)) {
        return partInfos;
    }

    // 分区的数量
    int partNumber = 1;
    // 磁盘的总大小
    qint64 total = dev.byteSize - 2 * giDeviceUnitSize * giDeviceUnitSize;

    QString currentPlatform = CommonFunc::getCurrentPlatform();
    QString currentOSVersion = CommonFunc::getCurrentOsVersion();

    for(QJsonValue value : policyDoc.array()) {
        QJsonArray archs = value.toObject()["platform"].toArray();
        QJsonArray osVersionToUse = value.toObject()["OSVersion"].toArray();
        if (archs.contains(currentPlatform)) {
            // 如果定制了OSVersion字段，则根据当前的系统版本决定是否使用该分区
            // 如果未定制OSVersion字段，则默认使用该分区
            if (osVersionToUse.size() > 0) {
                if (!osVersionToUse.contains(currentOSVersion)) {
                    continue;
                }
            }

            SPartitionInfo info = getConfigToPartition(value);
            // 剔除配置中和用户选择不一致的配置
            if (!info.device.isEmpty() && info.device != dev.device) {
                continue;
            }

            //　获取数据空间的配置
            if (gHomeLabel == info.label) {
                // 处理逻辑分区
                if (partNumber < gMBRMaxPartitionSize  || CommonFunc::isUEFI()) {
                    info.partitionType = EPartitionType::PartitionTypePrimary;
                } else {
                    info.partitionType = EPartitionType::PartitionTypeLogical;
                }

                if (total <= 0) {
                    err = QString("The %1 disk space is insufficient. Check the configuration file again full_disk_policy.json")
                            .arg(dev.device);
                    return QList<SPartitionInfo>();
                }

                QString usage = value.toObject()["usage"].toString();
                info.size = defaultPartitionSize(dev.size, CommonFunc::bytesToM(total), usage, info.label);

                if (info.size <= 0) {
                    err = QString("The partition size is incorrect. Check the configuration file again full_disk_policy.json. %2")
                            .arg(info.toString());
                    return QList<SPartitionInfo>();
                }

                info.device = dev.device;
                info.deviceSize = dev.size;
                info.sectorSize = dev.sectors;
                info.phySectorSize = dev.physectors;
                info.byteSize = CommonFunc::mToSectors(info.size, info.sectorSize) * info.sectorSize;
                partInfos.append(info);

                total -= info.byteSize;
                partNumber++;
                break;
            }
        }
    }

    return partInfos;
}

SPartitionInfo PartitionFullDiskManager::getConfigToPartition(const QJsonValue &value)
{
    SPartitionInfo info;
    info.label = value.toObject()["label"].toString();
    info.device = value.toObject()["device"].toString();
    info.usage = ESpaceUsage::USE;
    info.isStartPoint = value.toObject()["alignStart"].toBool(true);
    info.isNeedFormat = true;
    info.filesystem = value.toObject()["filesystem"].toString();
    info.mountPoint = value.toObject()["mountPoint"].toString();
    info.isCrypt       = value.toObject()["isCrypt"].toBool();
    info.cryptPassword = value.toObject()["cryptPassword"].toString();

    if (info.mountPoint.isEmpty() && info.label == gDataLabel) {
        info.mountPoint = CommonFunc::getSettingsString("DI_DATA_MOUNT_POINT_CONFIG", CommonFunc::getCurrentOsVersion());
    }
    return info;
}

qint64 PartitionFullDiskManager::getConfigPartitionSize(const qint64 devSize, const QString &label)
{
    qint64 reset = 0;
    if (label == gEFILabel) {
        reset = CommonFunc::getSettingsInt("DI_EFI_SIZE_CONFIG");
    } else if (label == gBootLabel) {
        reset =  CommonFunc::getSettingsInt("DI_BOOT_SIZE_CONFIG");
    } else if (label == gRootLabel || label == gRootaLabel || label == gRootbLabel) {
        reset =  CommonFunc::getSettingsInt("DI_ROOTA_SIZE_CONFIG");
    } else if (label == gBackupLabel) {
        reset =  CommonFunc::getSettingsInt("DI_RECOVERY_BASE_SIZE_CONFIG");
        reset += devSize * 2 / 100;
    } else if (label == gSWAPLabel) {
        reset = CommonFunc::getSwapSize();
    }

    return reset;
}

qint64 PartitionFullDiskManager::getOtherPartitionSize(qint64 totalSize)
{
    qint64 swapSize = getConfigPartitionSize(totalSize, gSWAPLabel);
    qint64 efiSize = getConfigPartitionSize(totalSize, gEFILabel);
    qint64 bootSize = getConfigPartitionSize(totalSize, gBootLabel);
    qint64 otherSize = swapSize + bootSize;
    return CommonFunc::isUEFI() ? otherSize + efiSize : otherSize;
}

void PartitionFullDiskManager::getDeviceCryptPartitions(const SDeviceInfo &devInfo, QList<SPartitionInfo> &partEncryptList)
{
    auto partList = PartitionOperateManager::instance()->getAvailableDeviceInfo().value(devInfo);
    for (auto &partInfo: partList) {
        if(partInfo.isCrypt && !partInfo.decryptStatus) {
            partEncryptList.append(partInfo);
        }
    }
}

SLVMInfo PartitionFullDiskManager::partToLvInfo(const SPartitionInfo &part)
{
    SLVMInfo reset;
    if (!part.id.isEmpty()) {
        reset.id = QUuid::createUuid().toString();
        reset.size = part.size;
        reset.label = part.label;
        reset.name = part.label;
        reset.filesystem = part.filesystem;
        reset.mountPoint = part.mountPoint;
        reset.isNeedFormat = part.isNeedFormat;
        reset.isCrypt = part.isCrypt;
        reset.cryptPassword = part.cryptPassword;

    }

    return reset;
}

QMap<SDeviceInfo, QList<SPartitionInfo> > PartitionFullDiskManager::getNotBootloaderPartition()
{
    QMap<SDeviceInfo, QList<SPartitionInfo> > reset;

    auto infos = PartitionSeniorManager::instance()->getSystemPartitionInfo();
    for (SDeviceInfo dev : infos.keys()) {
        if (dev == m_systemDisk || dev == m_homeDisk) {
            for (SPartitionInfo in : infos.value(dev)) {
                if (in.label == gEFILabel) {
                    continue;
                }
                if (in.label == gBootLabel) {
                    continue;
                }
                if (in.partitionType == EPartitionType::PartitionTypeExtension) {
                    continue;
                }

                if (in.usage == ESpaceUsage::FREE) {
                    continue;
                }

                reset[dev].append(in);
            }
        }
    }

    return reset;
}

QString PartitionFullDiskManager::getVGName()
{
    return PartitionSeniorManager::instance()->getVGName();
}

void PartitionFullDiskManager::addFullDiskLVM(const QString &fileSystem)
{
    QMap<SDeviceInfo, QList<SPartitionInfo>> infos = getNotBootloaderPartition();
    for (SDeviceInfo dev : infos.keys()) {
        qint64 pvSize = 0;
        QList<SLVMInfo> lvInfos;
        for (SPartitionInfo part : infos.value(dev)) {
            pvSize += part.size;

            SLVMInfo lv = partToLvInfo(part);
            // 如果是全盘加密安装则忽略full_disk_policy.json中的分区加密配置
            if ( fileSystem == gCryptoLuksFileSystem ) {
                lv.isCrypt = false;
                lv.cryptPassword = "";
            }

            if (!lv.id.isEmpty()) {
                lvInfos.append(lv);
                PartitionSeniorManager::instance()->delPartition(dev, part);
            }
        }

        SPartitionInfo free = PartitionSeniorManager::instance()->getFreePartInfo(dev, pvSize);
        if (!free.id.isEmpty()) {
            SPartitionInfo pvPart;
            pvPart.partitionType = EPartitionType::PartitionTypePrimary;
            pvPart.filesystem = fileSystem;
            pvPart.size = free.size;
            pvPart.isNeedFormat = true;
            // 设置加密卷标签
            if (std::find_if(lvInfos.begin(), lvInfos.end(), [](SLVMInfo lvItem){ return !lvItem.mountPoint.compare("/");}) != lvInfos.end()) {
                pvPart.label = "System";
            } else if (std::find_if(lvInfos.begin(), lvInfos.end(), [](SLVMInfo lvItem){ return !lvItem.mountPoint.compare("/home");}) != lvInfos.end()) {
                pvPart.label = "Data";
            }

            QString vgName = getVGName();

            PartitionSeniorManager::instance()->newPartition(dev, free, pvPart);
            PartitionSeniorManager::instance()->newVG(vgName, QList<SPartitionInfo>{pvPart});

            for (SLVMInfo lv : lvInfos) {
                SLVMInfo freeLv = PartitionSeniorManager::instance()->getFreeLVMInfo(PartitionSeniorManager::instance()->findVG(vgName));
                lv.isNeedFormat = true;
                PartitionSeniorManager::instance()->newLV(freeLv, lv);
            }
        }
    }
}

qint64 PartitionFullDiskManager::defaultPartitionSize(const qint64 devSize, const qint64 total, const QString &size, const QString &label)
{
    qint64 reset = 0;
    if (size.contains("%")) {
        // 处理配置中大小配置百分比的情况
        reset = CommonFunc::percentageToM(total, size);

    } else if (!size.isEmpty()) {
        // 处理配置中大小配置具体大小的情况，目前支持Mib/Gib/Tib
        reset = CommonFunc::strToM(size);

    } else if (label == gEFILabel) {
        reset = CommonFunc::getSettingsInt("DI_EFI_SIZE_CONFIG");

    } else if (label == gBootLabel) {
        reset =  CommonFunc::getSettingsInt("DI_BOOT_SIZE_CONFIG");

    } else if (label == gRootLabel || label == gRootaLabel || label == gRootbLabel) {
        reset =  CommonFunc::getSettingsInt("DI_ROOTA_SIZE_CONFIG") + m_rootaPartitionResize;

    } else if (label == gBackupLabel) {
        reset =  CommonFunc::getSettingsInt("DI_RECOVERY_BASE_SIZE_CONFIG");
        reset += devSize * 2 / 100;

    } else if (label == gSWAPLabel) {
        reset = CommonFunc::getSwapSize();
    }

    if (reset > total) {
        reset = -1;
    }

    return reset;
}

bool PartitionFullDiskManager::isMultipleDiskToVg(const SDeviceInfo &devName, QMap<QString, QString> &text)
{
    return PartitionOperateManager::instance()->isMultipleDiskToVg(devName, text);
}
