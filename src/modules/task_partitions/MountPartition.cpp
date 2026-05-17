#include "MountPartition.h"
#include "CommonFunc.h"
#include <QDebug>

MountPartition::~MountPartition()
{

}

bool MountPartition::start(const QString &config, QString &err)
{
    qInfo() << "mount partition start...";

    QList<PartMountPointInfo> partitionMountInfoList = beforeMounting(config);

    if (partitionMountInfoList.isEmpty()) {
        err = QString("no devices need to be mounted <config=%1>").arg(config);
        return false;
    }

    for (auto mountPointItem : partitionMountInfoList) {
        if(mountPointItem.mountPoint == "swap") {
            if (!swapon(mountPointItem.partPath, err)) {
                return false;
            }
        } else {
            if (!CommonFunc::mountToTarget(mountPointItem.partPath, mountPointItem.mountPoint, mountPointItem.mountOption, err)) {
                return false;
            }
        }
    }

    return true;
}

QList<PartMountPointInfo> MountPartition::beforeMounting(const QString &config)
{
    QList<PartMountPointInfo> reset;

    QStringList lines = CommonFunc::qstringSplitToQStringList(config, ";");
    for (QString &line : lines) {
        PartMountPointInfo mountPointInfo;
        if(line.contains('[')) {
            QString option = line.mid(line.indexOf('[') + 1);
            option.chop(1);
            mountPointInfo.mountOption = option;
            line = line.left(line.indexOf('['));
        }

        const QStringList list = line.split("=");
        if (list.count() < 2) {
            continue;
        }

        mountPointInfo.partPath = list.first();
        mountPointInfo.mountPoint = list.last();
        mountPointInfo.mountPoint.replace("//", "/");

        if (!mountPointInfo.mountPoint.isEmpty() && !mountPointInfo.partPath.isEmpty()) {
            mountPointInfo.pathDeep = (!mountPointInfo.mountPoint.compare("/")) ? 0 : mountPointInfo.mountPoint.count("/");
            reset.append(mountPointInfo);
        }
    }

    // 按照路径浅度排序
    std::sort(reset.begin(), reset.end(), [=](PartMountPointInfo strItemPre, PartMountPointInfo strItemAfter){
        return (strItemPre.pathDeep < strItemAfter.pathDeep);
    });

    return reset;
}

bool MountPartition::swapon(const QString &device, QString &err)
{
    // 判断一下swap分区是否已经激活
    QString cmd = QString("swapon | awk '{print $1}' | grep %1").arg(device);
    if (CommonFunc::command("/bin/bash", {"-c", cmd}, err)) {
        qInfo() << QString("swap:%1 is already on ").arg(device);
        return true;
    } else {
        // 尝试激活
        if (!CommonFunc::command("swapon", QStringList() << device, err)) {
            qWarning() << QString("swapon %1 partition failed").arg(device);
            return false;
        } else {
            return true;
        }
    }
}
