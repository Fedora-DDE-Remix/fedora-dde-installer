#include "PartitionAutoManager.h"
#include "CommonFunc.h"
#include "globals.h"

#include "DeviceManager.h"
#include "PartitionFullDiskManager.h"
#include "PartitionOperateManager.h"
#include "PartitionCustomManager.h"
#include "PartitionSeniorManager.h"

#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

PartitionAutoManager *PartitionAutoManager::m_instance = Q_NULLPTR;
PartitionAutoManager *PartitionAutoManager::instance()
{
    if(m_instance == nullptr)
        m_instance = new PartitionAutoManager();
    return m_instance;
}

bool PartitionAutoManager::start(QString &err)
{
    if (CommonFunc::isAuotInstall()) {

        // 设置磁盘信息
        PartitionOperateManager::instance()->setDeviceInfo(DeviceManager::instance()->getDeviceInfos());
        PartitionOperateManager::instance()->setLVMInfo(DeviceManager::instance()->getLVMInfos());

        //  获取当前无人值守的安装方式，目前无人值守支持如下安装方式：
        int installMode = CommonFunc::getSettingsInt("DI_PARTITION_TYPE");
        qInfo() << "auto-installMode = " << installMode;
        switch (installMode) {
            case InstallMode::FullDiskMode:
                if (!autoFullDisk(err)) {
                    return false;
                } break;
            case InstallMode::LvmFullDisk:
                if (!autoLVMFullDisk(err)) {
                    return false;
                } break;
            case InstallMode::Cryptsetup:
                if (!autoFullDiskEncryption(err)) {
                    return false;
                } break;
            case InstallMode::SaveData:
                if (!autoSaveData(err)) {
                    return false;
                } break;
            case InstallMode::CustomMode:
                if (!autoCustomDisk(err)) {
                    return false;
                } break;

            default: {
                err = QString("Unknown unattended installation mode <%1>").arg(installMode);
                return false;
            }
        }
    }

    return true;
}

bool PartitionAutoManager::autoFullDisk(QString &err, bool isSaveData)
{
    // 获取用户配置的磁盘信息
    QStringList list = CommonFunc::getSettingsStringList("DI_DEVICE_LIST");

    // 全盘安装
    QMap<SDeviceInfo, QList<SPartitionInfo>> deviceInfos = DeviceManager::instance()->getDeviceInfos();
    PartitionOperateManager::instance()->setDeviceInfo(deviceInfos);
    QList<SDeviceInfo> devs = this->getSelectDevice(list, PartitionOperateManager::instance()->getCurrentDevice(), isSaveData);
    if (devs.size() == 1) {
        if (!PartitionFullDiskManager::instance()->addSystemDisk(devs.first(), err)) {
            return false;
        }
    } else if (devs.size() == 2) {
        qint64 totalSize = devs.first().size;
        qint64 minSysSize = PartitionFullDiskManager::instance()->getConfigPartitionSize(totalSize, gRootaLabel);
        qint64 otherSize = PartitionFullDiskManager::instance()->getOtherPartitionSize(totalSize);
        qint64 maxSysSize = (totalSize - otherSize) / 2;
        if (!PartitionFullDiskManager::instance()->addSystemDisk(devs.first(), err)) {
            return false;
        }

        if (!PartitionFullDiskManager::instance()->addHomeDisk(devs.last(), err)) {
            qWarning() << "addDataDisk failed, may be no data partition in config file !";
        }

        PartitionFullDiskManager::instance()->resizeSystemPartition(maxSysSize - minSysSize);
    } else {
        err = "Invalid disk configuration. Check whether the DI_DEVICE_LIST configuration "
               "in settings.ini is consistent with the device file on the machine";
        return false;
    }
    return true;
}

bool PartitionAutoManager::autoLVMFullDisk(QString &err)
{
    autoFullDisk(err);
    PartitionFullDiskManager::instance()->addFullDiskLVM(gLVM2PVFileSystem);
    return true;
}


bool PartitionAutoManager::autoFullDiskEncryption(QString &err)
{
    if (!CommonFunc::getSettingsBool("DI_CRYPTO_LOAD")) {
        err = "<DI_CRYPTO_LOAD> is false";
        return false;
    }
    if (CommonFunc::getSettingsString("DI_CRYPT_PASSWORD").isEmpty()) {
        err = "<DI_CRYPT_PASSWORD> is empty";
    }
    autoFullDisk(err);
    PartitionFullDiskManager::instance()->addFullDiskLVM(gCryptoLuksFileSystem);
    return true;
}

bool PartitionAutoManager::autoSaveData(QString &err)
{
    if (!CommonFunc::getSettingsBool("DI_SAVE_USER_DATA_LOAD")) {
        err = "<DI_SAVE_USER_DATA_LOAD> is false";
        return false;
    }

    //找到需要解密的设备，进行解密处理
    QStringList needDeCryptDevs;
    QStringList cryptDevPaths = CommonFunc::getCryptDevPaths();
    if (!cryptDevPaths.isEmpty()) {
        QStringList devList = CommonFunc::getSettingsStringList("DI_DEVICE_LIST");
        for (QString cryptDevItem : cryptDevPaths) {
            auto okDev = std::find_if(devList.begin(), devList.end(), [=](const QString &dev) {
                return  cryptDevItem.contains(dev);
            });
            if (okDev != devList.end()) {
                needDeCryptDevs.append(*okDev);
            }
        }
    }

    QString cryptPassword = CommonFunc::getSettingsString("DI_CRYPT_PASSWORD");

    for (QString needDeCryptItem : needDeCryptDevs) {
        if (!CommonFunc::deCryptPartition(cryptPassword, needDeCryptItem, err)) {
            err = "deCrypt" + needDeCryptItem + "failed ! " + err;
            return false;
        }
    }
    
    // 更新设备管理对象中对应分区的信息
    DeviceManager::instance()->scanDevices(true);
    autoFullDisk(err, true);

    // 校验是否满足保留用户数据条件，需要额外处理支持lvm保留用户数据的功能
    QString oldVersion;
    if (PartitionFullDiskManager::instance()->checkSaveUserData(oldVersion) != KeepDataError::NO_ERROR) {
        err = "Does not meet the conditions for retaining user data";
        return false;
    }
    
    PartitionFullDiskManager::instance()->saveUserData();
    return true;
}

bool PartitionAutoManager::autoCustomDisk(QString &err)
{
    if (!CommonFunc::isUEFI()) {
        err = ::QObject::tr("Computers with legacy boot do not support installing multiple systems on a single disk.");
        return false;
    }

    SPartitionInfo free = findFreePartition();
    if (free.id.isEmpty()) {
        err = ::QObject::tr("Insufficient Disk Space");
        return false;
    }

    free.isNeedFormat = true;

    SDeviceInfo dev = PartitionCustomManager::instance()->findDev(free.device);
    qint64 miniDiskSize = CommonFunc::gToM(qint64(CommonFunc::getSettingsInt("DI_DEVICE_MIN_SIZE_CONFIG")));
    if (free.size < miniDiskSize) {
        err = ::QObject::tr("Insufficient Disk Space");
        qInfo() << QString("DI_DEVICE_MIN_SIZE_CONFIG = %1MiB").arg(miniDiskSize);
        return false;
    }

    autoCustomDiskNewPartition(dev, free);
    return PartitionCustomManager::instance()->checkPartition(err);
}


void PartitionAutoManager::autoCustomDiskNewPartition(const SDeviceInfo &devName, const SPartitionInfo &freeName)
{
    QString err = "";
    QList<SPartitionInfo> config = PartitionFullDiskManager::instance()->getSystemPartitionConfig(devName, err);
    if (config.isEmpty()) {
        qWarning() << err;
        return;
    }

    for (SPartitionInfo in : config) {
        in.isNeedFormat = true;
        PartitionSeniorManager::instance()->newPartition(devName, freeName, in);
    }
}

QList<SPartitionInfo> PartitionAutoManager::getPartitionConfig(const SDeviceInfo &dev, QString &err)
{
    QList<SPartitionInfo> partInfos;

    QJsonDocument policyDoc;
    if (!CommonFunc::getSettingsPolicy(policyDoc, err)) {
        return partInfos;
    }

    // 分区的数量
    int partNumber = 1;

    // 磁盘的总大小
    qint64 total = dev.byteSize - (CommonFunc::isUEFI() ? 66 : 4);

    if (CommonFunc::isUEFI() && PartitionSeniorManager::instance()->getEFIPartition().id.isEmpty()) {
        SPartitionInfo efi = CommonFunc::getEFIPartitionInfo(dev);
        partInfos.append(efi);
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

            SPartitionInfo info = PartitionFullDiskManager::instance()->getConfigToPartition(value);

            if (!PartitionSeniorManager::instance()->existSWAPPartition() && info.filesystem.contains("linux-swap")) {
                continue;
            }

            // 处理逻辑分区
            if (partNumber < gMBRMaxPartitionSize  || CommonFunc::isUEFI()) {
                info.partitionType = EPartitionType::PartitionTypePrimary;
            } else {
                info.partitionType = EPartitionType::PartitionTypeLogical;
            }

            if (total <= 0) {
                err = QString("The %1 disk space is insufficient.").arg(dev.device);
                return QList<SPartitionInfo>();
            }

            QString usage = value.toObject()["usage"].toString();
            info.size = PartitionFullDiskManager::instance()->defaultPartitionSize(dev.size, CommonFunc::bytesToM(total), usage, info.label);

            if (info.size <= 0) {

                err = QString("The %1 partition size is incorrect").arg(info.label);
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
        }
    }

    return partInfos;
}

QList<SDeviceInfo> PartitionAutoManager::getSelectDevice(const QStringList &infos, const QList<SDeviceInfo> &devs, bool isSaveData)
{
    QList<SDeviceInfo> reset;
    if (infos.isEmpty()) {
        if (isSaveData) {
            // 如果保留用户数据全盘安装
            // 先找数据盘
            for(auto in : devs) {
                auto datas = PartitionOperateManager::instance()->getAvailableDeviceInfo().value(in);
                if (!PartitionSeniorManager::instance()->isFormatDisk(in)) {
                    for (SPartitionInfo p : datas) {
                        if (p.label == gDataLabel) {
                            reset.append(in);
                            break;
                        }
                    }
                }
            }
            if (!reset.isEmpty()) {
                // 如果已经找到数据盘，再找系统盘，可能和数据盘相同
                for(auto in : devs) {
                    auto systems = PartitionOperateManager::instance()->getAvailableDeviceInfo().value(in);
                    if (!PartitionSeniorManager::instance()->isFormatDisk(in)) {
                        for (SPartitionInfo p : systems) {
                            if (p.label == gRootaLabel) {
                                if (!reset.contains(in)) {
                                    reset.insert(0, in);
                                }
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            QList<SDeviceInfo> resetTmp = CommonFunc::sortDevice(devs);
            // 过滤大小不符合要求的磁盘
            qint64 minSize = CommonFunc::getSettingsInt("DI_DEVICE_MIN_SIZE_CONFIG");
            for (SDeviceInfo devItem : resetTmp) {
                if (CommonFunc::mToG(devItem.size) >= minSize) {
                    reset.append(devItem);
                }
            }
        }
    } else {
        // 否则使用用户定制的DI_DEVICE_LIST中顺序来区分系统盘还是数据盘
        for (QString in : infos) {
            for (SDeviceInfo d : devs) {
                if (d.device == in) {
                    reset.append(d);
                }
            }
        }
    }

    const int maxDiskNum = 2;
    reset = reset.size() <= maxDiskNum ? reset : reset.mid(0, 2);  // 最多可以配置两个磁盘
    return reset;
}

SPartitionInfo PartitionAutoManager::findFreePartition()
{
    QList<SDeviceInfo> opDevs;

    QList<SDeviceInfo> devs = PartitionCustomManager::instance()->getSystemPartitionInfo().keys();
    // 获取用户配置的磁盘信息
    QStringList list = CommonFunc::getSettingsStringList("DI_DEVICE_LIST");
    for (QString in : list) {
        for (SDeviceInfo d : devs) {
            if (d.device == in) {
                opDevs.append(d);
            }
        }
    }

    // 用户配置的不符合磁盘要求时，查找默认磁盘上的空间
    if (opDevs.isEmpty()) {
        opDevs = devs;
    }

    QList<SPartitionInfo> freeParts;
    for (SDeviceInfo dev : opDevs) {
        QList<SPartitionInfo> parts = PartitionCustomManager::instance()->getSystemPartitionInfo().value(dev);
        for (SPartitionInfo p : parts) {
            if (p.usage == ESpaceUsage::FREE && p.size >= CommonFunc::getSettingsInt("DI_DEVICE_MIN_SIZE_CONFIG")) {
                freeParts.append(p);
            }
        }
    }

    //　判断磁盘上是否有合适的空闲分区
    if (freeParts.isEmpty()) {
        return SPartitionInfo();
    }

    //　对空闲分区进行排序，优先将系统安装在靠前的分区上，防止非x86的机型上需要引导分区在磁盘开始的位置
    std::sort(freeParts.begin(), freeParts.end());
    return freeParts.first();
}
