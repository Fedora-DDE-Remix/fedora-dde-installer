#include "CryptsetupPartition.h"
#include "PartitionTaskManager.h"
#include "CommonFunc.h"

#include <QMap>
#include <iostream>

CryptsetupPartition::~CryptsetupPartition()
{

}

bool CryptsetupPartition::start(const QString &config, QString &err)
{
    if (!diskTask(config, err)) {
        return false;
    }

    QList<SPartitionOperate> infos;
    if (!partitionTask(config, infos, err)) {
        return false;
    }

    if (!vgTask(config, infos, err)) {
        return false;
    }

    QList<SLVMOperate> lvInfos;
    if (!lvmTask(config, lvInfos, err)) {
        return false;
    }

    // 处理挂载点
    this->setupMountPoint(infos, lvInfos);
    return true;
}

bool CryptsetupPartition::diskTask(const QString &config, QString &err)
{
    // 获取VG信息
    QList<SVGOperate> infos;
    if (!PartitionTaskManager::instance()->getDeviceOperator(config, infos, err)) {
        return false;
    }

    // 处理VG的删除
    for (auto &in : infos) {
        switch (in.operate) {
            case EPartitionOperate::PartitionDelete: {
                if (!CommonFunc::deleteVG(in.vg, err)) {
                    return false;
                }

            } break;
            default:break;
        }
    }

    // 获取磁盘操作信息
    QList<SDeviceOperate> devinfos;
    if (!PartitionTaskManager::instance()->getDeviceOperator(config, devinfos, err)) {
        return false;
    }

    for (auto& in : devinfos) {
        // 清理磁盘上的状态
        if (!CommonFunc::clearDeviceStatus(in.device.device, err)) {
            return false;
        }

        if (!CommonFunc::setDisklabelType(in.device, err)) {
            return false;
        }
    }

    return  true;
}

bool CryptsetupPartition::partitionTask(const QString &config, QList<SPartitionOperate> &infos, QString &err)
{
    // 获取分区信息
    if (!PartitionTaskManager::instance()->getDeviceOperator(config, infos, err)) {
        return false;
    }

    // 新建分区
    for (auto& in : infos) {
        if (in.operate == EPartitionOperate::PartitionNormal) {
            continue;
        }

        if (!CommonFunc::creatPartition(in.partition, err)) {
            return false;
        }

        // 设置DI_CRYPT_DEVICE用于后面脚本设置crypttab
        if (in.partition.filesystem == gCryptoLuksFileSystem) {
            QString cryptDevice = CommonFunc::getSettingsString("DI_CRYPT_DEVICE");
            if (cryptDevice.isEmpty()) {
                CommonFunc::setSettingsValue("DI_CRYPT_DEVICE", in.partition.path);
            } else {
                cryptDevice += " " + in.partition.path;
                CommonFunc::setSettingsValue("DI_CRYPT_DEVICE", cryptDevice);
            }
        }
    }

    return true;
}

bool CryptsetupPartition::vgTask(const QString &config, const QList<SPartitionOperate> &filter, QString &err)
{
    // 获取VG信息
    QList<SVGOperate> infos;
    if (!PartitionTaskManager::instance()->getDeviceOperator(config, infos, err)) {
        return false;
    }

    // 处理VG
    for (auto &in : infos) {
        switch (in.operate) {
            case EPartitionOperate::PartitionCreate : {
                QStringList pvPaths;
                for (auto id : in.vg.pvIds) {
                    SPartitionInfo pv = CommonFunc::find(id, filter);
                    if (!pv.path.isEmpty()) {
                        if (!CommonFunc::createPV(pv.path, err)) {
                            return false;
                        }
                        pvPaths.append(pv.path);
                    }
                }

                in.vg.pvIds = pvPaths;
                if (!CommonFunc::createVG(in.vg, err)) {
                    return false;
                }
            } break;
            default:break;
        }
    }

    return true;
}

bool CryptsetupPartition::lvmTask(const QString &config, QList<SLVMOperate> &infos, QString &err)
{
    // 获取VG信息
    if (!PartitionTaskManager::instance()->getDeviceOperator(config, infos, err)) {
        return false;
    }

    // 处理VG
    for (auto &in : infos) {
        switch (in.operate) {
            case EPartitionOperate::PartitionCreate: {
                if (!CommonFunc::createLVM(in.lvm, err)) {
                    return false;
                }

            } break;

            case EPartitionOperate::PartitionEdit: {
                if (!CommonFunc::editLVM(in.lvm, err)) {
                    return false;
                }

            } break;

            default:break;
        }
    }

    return true;
}

