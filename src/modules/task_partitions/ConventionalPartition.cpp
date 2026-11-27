#include "ConventionalPartition.h"
#include "PartitionTaskManager.h"
#include "CommonFunc.h"

#include <iostream>
#include <QThread>

ConventionalPartition::~ConventionalPartition()
{

}

bool ConventionalPartition::start(const QString &config, QString &err)
{
    // Validate the encryption contract before any table or partition changes.
    // The Python backend performs LUKS2 formatting after verifying the actual
    // physical partition returned by this helper. This process never gets a key.
    if (!PartitionTaskManager::instance()->validateEncryptionPlan(config, err))
        return false;
    if (!diskTask(config, err)) {
        return false;
    }

    QList<SPartitionOperate> infos;
    if (!partitionTask(config, infos, err)) {
        return false;
    }

    if (!updateCryptHeaderHidePartInfo(config, infos, err)) {
        return false;
    }

    if (!vgTask(config, infos, err)) {
        return false;
    }

    QList<SLVMOperate> lvInfos;
    if (!lvmTask(config, lvInfos, infos, err)) {
        return false;
    }

    // 处理挂载点
    this->setupMountPoint(infos, lvInfos);
    return true;
}

bool ConventionalPartition::diskTask(const QString &config, QString &err)
{
    // 获取lv信息
    QList<SLVMOperate> lvInfos;
    if (!PartitionTaskManager::instance()->getDeviceOperator(config, lvInfos, err)) {
        return false;
    }

    // 先处理LV的删除
    for (auto &in : lvInfos) {
        switch (in.operate) {
            case EPartitionOperate::PartitionDelete: {
                if (!CommonFunc::deleteLVM(in.lvm, err)) {
                    return false;
                }

            } break;

            default:break;
        }
    }

    // 获取VG信息
    QList<SVGOperate> vgInfos;
    if (!PartitionTaskManager::instance()->getDeviceOperator(config, vgInfos, err)) {
        return false;
    }

    // 再处理VG的删除
    for (auto &in : vgInfos) {
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

    // 清理磁盘并处理格式化
    for (auto& in : devinfos) {
        if (in.operate == EPartitionOperate::PartitionNormal)
            continue;
        if (in.operate != EPartitionOperate::PartitionCreate && in.operate != EPartitionOperate::PartitionEdit) {
            err = "Unsupported partition-table operation";
            return false;
        }
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

bool ConventionalPartition::partitionTask(const QString &config, QList<SPartitionOperate> &infos, QString &err)
{
    // 获取分区信息
    if (!PartitionTaskManager::instance()->getDeviceOperator(config, infos, err)) {
        return false;
    }

    // 处理信息分区
    for (auto &in : infos) {
        if (in.operate == EPartitionOperate::PartitionNormal) {
            continue;
        }

        switch (in.operate) {
            case EPartitionOperate::PartitionCreate: {
                if (!CommonFunc::creatPartition(in.partition, err)) {
                    return false;
                }

            } break;

            case EPartitionOperate::PartitionDelete: {
                if (!CommonFunc::deletePartition(in.partition, err)) {
                    return false;
                }

            } break;

            case EPartitionOperate::PartitionEdit: {
                if (!CommonFunc::editPartition(in.partition, err)) {
                    return false;
                }

            } break;

            default:
                break;
        }

    }

    return true;
}

bool ConventionalPartition::vgTask(const QString &config, const QList<SPartitionOperate> &filter, QString &err)
{
    // 获取VG信息
    QList<SVGOperate> infos;
    if (!PartitionTaskManager::instance()->getDeviceOperator(config, infos, err)) {
        return false;
    }

    // 处理VG
    for (auto &in : infos) {
        switch (in.operate) {
            case EPartitionOperate::PartitionCreate: {
                //  获取pv对应的分区的路径
                QStringList pvIds;
                for (auto id : in.vg.pvIds) {
                    SPartitionInfo pv = CommonFunc::find(id, filter);
                    if (!pv.path.isEmpty()) {
                        pvIds.append(pv.path);
                    }
                }
                in.vg.pvIds = pvIds;
                if (!CommonFunc::createVG(in.vg, err)) {
                    return false;
                }

            } break;

            default:break;
        }
    }

    return true;
}

bool ConventionalPartition::lvmTask(const QString &config, QList<SLVMOperate> &infos, const QList<SPartitionOperate> &partitionInfos, QString &err)
{
    // 获取VG信息
    if (!PartitionTaskManager::instance()->getDeviceOperator(config, infos, err)) {
        return false;
    }

    // 处理LV
    for (auto &in : infos) {
        switch (in.operate) {
            case EPartitionOperate::PartitionCreate: {
                if (!CommonFunc::createLVM(in.lvm, err)) {
                    return false;
                }

                if (!lvmCryptHeaderTreat(in, partitionInfos, err)) {
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

bool ConventionalPartition::lvmCryptHeaderTreat(const SLVMOperate &lvmInfo, const QList<SPartitionOperate> &partitionInfos, QString &err)
{
    if (!CommonFunc::getSettingsBool("DI_USE_HIDE_CRYPT_HEADER")) {
        return true;
    }

    // 如果是加密分区，则与被加密分区建立绑定关系，并初始化头部信息
    if (lvmInfo.lvm.isCryptHeader) {
        QString lvmName = lvmInfo.lvm.name;
        QString partUuid = lvmName.right(lvmName.length() - lvmName.indexOf("-") - 1);
        QString bindPath = QString("/tmp/%1").arg(lvmName);
        QString dmLinearName = QString("lv-abc-%1").arg(partUuid);
        int headerSize = 16;//Mib

        // 建立绑定关系
        QFile file(bindPath);
        if (file.open(QIODevice::Text | QIODevice::WriteOnly)) {
            QList<SPartitionOperate>::const_iterator partIter = std::find_if(partitionInfos.begin(), partitionInfos.end(), [&](const SPartitionOperate &infoItem) {
                return infoItem.partition.partUuid == partUuid;
            });
            if (partIter != partitionInfos.end()) {
                qint64 headerSectors = (headerSize * giDeviceUnitSize * giDeviceUnitSize) / 512;
                qint64 headerPartSectors = ((*partIter).partition.endPoint - (*partIter).partition.startPoint + 1) * (*partIter).partition.sectorSize / 512;
                QString lvmBindInfo = QString("0 %1 linear /dev/%2/%3 0").arg(headerSectors).arg(lvmInfo.lvm.vgName).arg(lvmInfo.lvm.name);
                file.write(lvmBindInfo.toLocal8Bit());
                file.write("\n");
                QString partBindInfo = QString("%1 %2 linear %3 0").arg(headerSectors).arg(headerPartSectors).arg((*partIter).partition.path);
                file.write(partBindInfo.toLocal8Bit());
                file.close();

                // 执行绑定
                QString cmd = QString("dmsetup create %1 %2").arg(dmLinearName, bindPath);
                if (!CommonFunc::command("/bin/bash", {"-c", cmd}, err)) {
                    err = QString("Command: %1 failed. %2").arg(cmd, err);
                    return false;
                }
                qInfo() << QString("Command: %1 succeed. out: %2").arg(cmd, err);
            } else {
                file.close();
                qWarning() << QString("dmsetup cannot find partuuid:%1 all part info is:").arg(partUuid);
                for (const SPartitionOperate &partInfoItem : partitionInfos) {
                    qInfo() << partInfoItem.partition.toString();
                }
            }

            //QFile::remove(bindPath);

        } else {
            err = QString("could't open bind file : %1 err : %2").arg(bindPath, file.errorString());
            return false;
        }

        // 初始化头部信息
        QString cmd = QString("echo "" | cryptsetup reencrypt --batch-mode --pbkdf pbkdf2 --encrypt --reduce-device-size %1M --init-only /dev/mapper/%2").arg(headerSize * 2).arg(dmLinearName);
        if (!CommonFunc::command("/bin/bash", {"-c", cmd}, err)) {
            err = QString("Command: %1 failed. %2").arg(cmd, err);
            return false;
        }
        qInfo() << QString("Command: %1 succeed. out: %2").arg(cmd, err);

        // 删除创建的绑定文件
        int timeout = 0;
        while (timeout < 3000) {
            if (!QFile::exists(QString("/dev/mapper/%1").arg(dmLinearName))) {
                break;
            }

            cmd = QString("dmsetup remove %1").arg(dmLinearName);
            if (!CommonFunc::command("/bin/bash", {"-c", cmd}, err)) {
                qWarning() << QString("Command: %1 failed. %2").arg(cmd, err);
            } else {
                qInfo() << QString("Command: %1 succeed. out: %2").arg(cmd, err);
            }

            timeout += 1000;
            QThread::msleep(1000);
        }

        if (timeout == 3000) {
            err = "try 3 times : " + QString("Command: %1 failed. %2").arg(cmd, err);
            return false;
        }
    }
    return true;
}
