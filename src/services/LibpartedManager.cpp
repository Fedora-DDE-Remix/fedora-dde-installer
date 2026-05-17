#include "LibpartedManager.h"
#include "CommonFunc.h"
#include "structs.h"

#include <QDebug>

LibpartedManager *LibpartedManager::instance()
{
    static LibpartedManager lm;
    return &lm;
}

LibpartedManager::LibpartedManager()
{

}

void LibpartedManager::settleDevice(int timeout)
{
    QString err;
    QString cmd = QString("udevadm settle --timeout=%1")
            .arg(timeout);

    if (!CommonFunc::command("/bin/bash", {"-c", cmd}, err)) {
        qInfo() << QString("Command: %1 failed. %2").arg(cmd).arg(err);
        return;
    }

    qInfo() << QString("Command: %1 succeed.").arg(cmd);
}

bool LibpartedManager::commit(PedDisk* lpDisk)
{
    if (ped_disk_commit(lpDisk) != 0) {
        settleDevice(5);
        return true;
    }

    return false;
}

void LibpartedManager::destroyDeviceAndDisk(PedDevice*& lpDevice, PedDisk*& lpDisk)
{
    if (lpDevice) {
        ped_device_destroy(lpDevice);
        lpDevice = nullptr;
    }

    if (lpDisk) {
        ped_disk_destroy(lpDisk);
        lpDisk = nullptr;
    }
}

bool LibpartedManager::getDeviceAndDisk(const QString& devicePath,
                      PedDevice*& lpDevice,
                      PedDisk*& lpDisk)
{
    lpDevice = ped_device_get(devicePath.toStdString().c_str());
    if (lpDevice) {
        lpDisk = ped_disk_new(lpDevice);
        if (lpDisk != nullptr) {
            return true;
        } else {
            destroyDeviceAndDisk(lpDevice, lpDisk);
            return false;
        }
    } else {
        return false;
    }
}

QString LibpartedManager::getPartitionPath(PedPartition* lpPartition)
{
    // Result of ped_partition_get_path() need to be freed by hand.
    char* lpPath = ped_partition_get_path(lpPartition);
    QString path;

    if (lpPath != nullptr) {
        path = lpPath;
        free(lpPath);
    }

    return path;
}

PedPartition* LibpartedManager::getPartition(PedDisk* lpDisk, const QString& partPath)
{
    if (!lpDisk) {
        return nullptr;
    }

    for (PedPartition* lpPartition = ped_disk_next_partition(lpDisk, nullptr);
         lpPartition != nullptr;
         lpPartition = ped_disk_next_partition(lpDisk, lpPartition)) {

        if (partPath == getPartitionPath(lpPartition)) {
            return lpPartition;
        }
    }

    return nullptr;
}

bool LibpartedManager::resizeExtendedPartition(const SPartitionInfo& partInfo)
{
    bool ok = false;
    PedDevice* lpDevice = nullptr;
    PedDisk* lpDisk = nullptr;

    if (partInfo.partitionType != EPartitionType::PartitionTypeExtension) {
        qWarning() << QString("Partition %1 type is invalid").arg(partInfo.path);
        return false;
    }

    if (!getDeviceAndDisk(partInfo.device, lpDevice, lpDisk)) {
        qWarning() << QString("Get device and disk pointer failed, device path: %1").arg(partInfo.device);
        return false;
    }

    PedPartition* lpPartition = ped_disk_extended_partition(lpDisk);

    if (lpPartition) {
        PedGeometry* geom = ped_geometry_new(lpDevice
                                             , partInfo.startPoint
                                             , partInfo.endPoint - partInfo.startPoint + 1);

        PedConstraint* constraint = nullptr;
        if (geom) {
            constraint = ped_constraint_exact(geom);
        }

        ok = false;
        if (constraint) {
            ok = (0 != ped_disk_set_partition_geom(lpDisk
                                                   , lpPartition
                                                   , constraint
                                                   , partInfo.startPoint
                                                   , partInfo.endPoint));

            ped_constraint_destroy(constraint);
        }

        if (ok) {
            ok = commit(lpDisk);
        }

        if (geom) {
            ped_geometry_destroy(geom);
        }
    }

    destroyDeviceAndDisk(lpDevice, lpDisk);

    return ok;
}

QString LibpartedManager::getPartitionPath(const SPartitionInfo &partInfo)
{
    PedDevice* lpDevice = nullptr;
    PedDisk* lpDisk = nullptr;
    PedPartition* lpPartition = nullptr;
    QString path;

    if (!getDeviceAndDisk(partInfo.device, lpDevice, lpDisk)) {
        qWarning() << QString("Get device and disk pointer failed, device path: %1").arg(partInfo.device);
        return QString();
    }

    if (partInfo.partitionType == EPartitionType::PartitionTypeExtension) {
        lpPartition = ped_disk_extended_partition(lpDisk);
    }
    else {
        lpPartition = ped_disk_get_partition_by_sector(lpDisk
                                                       , partInfo.startPoint + (partInfo.endPoint - partInfo.startPoint) / 2);
    }

    if (lpPartition) {
        path = getPartitionPath(lpPartition);
    }

    destroyDeviceAndDisk(lpDevice, lpDisk);

    return path;
}
