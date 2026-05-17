#include "SaveDataPartition.h"
#include "CommonFunc.h"
#include "PartitionTaskManager.h"
#include "globals.h"

#include <QDebug>
#include <QStringList>
#include <iostream>

SaveDataPartition::~SaveDataPartition()
{

}

bool SaveDataPartition::start(const QString &config, QString &err)
{
    QString cryptDevice = "";

    // 获取分区信息
    QList<SPartitionOperate> infos;
    if (!PartitionTaskManager::instance()->getDeviceOperator(config, infos, err)) {
        return false;
    }

    QString labelToSkip = gDataLabel;
    if (std::find_if(infos.begin(), infos.end(), [=](const SPartitionOperate &opItem){
        return (opItem.operate != EPartitionOperate::PartitionNormal) && (opItem.partition.label == gHomeLabel);
    }) != infos.end()) {
        labelToSkip = gHomeLabel;
    }

    //格式化系统当前分区
    for (auto &in : infos) {
        if (in.operate == EPartitionOperate::PartitionNormal) {
            continue;
        }

        // 查找加密卷
        QString cryptVolumPath = "";
        if (CommonFunc::findCryptVolum(in.partition.device, cryptVolumPath, err)) {
            if (cryptDevice.isEmpty()) {
                cryptDevice = cryptVolumPath;
            } else {
                cryptDevice = cryptDevice.contains(cryptVolumPath) ? cryptDevice : cryptDevice + " " + cryptVolumPath;
            }
        }

        if (in.partition.label == labelToSkip) {
            continue;
        }

        // 卸载磁盘上的挂载点
        if (!CommonFunc::umountOnDevice(in.partition.path, err)) {
            return false;
        }

        if (!CommonFunc::formatPartitionOK(in.partition, err)) {
            return false;
        }
    }

    // 获取LVM分区信息
    QList<SLVMOperate> lvmInfos;
    if (!PartitionTaskManager::instance()->getDeviceOperator(config, lvmInfos, err)) {
        return false;
    }

    labelToSkip = gDataLabel;
    if (std::find_if(lvmInfos.begin(), lvmInfos.end(), [=](const SLVMOperate &opItem){
        return (opItem.operate != EPartitionOperate::PartitionNormal) && (opItem.lvm.label == gHomeLabel);
    }) != lvmInfos.end()) {
        labelToSkip = gHomeLabel;
    }

    //格式化系统当前分区
    for (auto &in : lvmInfos) {
        if (in.operate == EPartitionOperate::PartitionNormal) {
            continue;
        }

        // 查找加密卷
        QString cryptVolumPath = "";
        if (CommonFunc::findCryptVolumByLVM(in.lvm.path, cryptVolumPath, err)) {
            if (cryptDevice.isEmpty()) {
                cryptDevice = cryptVolumPath;
            } else {
                cryptDevice = cryptDevice.contains(cryptVolumPath) ? cryptDevice : cryptDevice + " " + cryptVolumPath;
            }
        }

        if (in.lvm.label == labelToSkip) {
            continue;
        }

        // 卸载磁盘上的挂载点
        if (!CommonFunc::umountOnDevice(in.lvm.path, err)) {
            return false;
        }

        if (!CommonFunc::formatPartitionOK(in.lvm, err)) {
            return false;
        }
    }

    CommonFunc::setSettingsValue("DI_CRYPT_DEVICE", cryptDevice);

    // 处理挂载点
    this->setupMountPoint(infos, lvmInfos);
    return true;

}
