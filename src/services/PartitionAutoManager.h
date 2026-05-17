#ifndef PARTITIONAUTOMANAGER_H
#define PARTITIONAUTOMANAGER_H

#include "structs.h"

#include <QString>

class PartitionAutoManager {
public:
    static PartitionAutoManager *instance();
public:
    bool start(QString &err);
private:
    bool autoFullDisk(QString &err, bool isSaveData = false);
    bool autoLVMFullDisk(QString &err);
    bool autoFullDiskEncryption(QString &err);
    bool autoSaveData(QString &err);
    // 无人值守多系统安装
    bool autoCustomDisk(QString &err);
    void autoCustomDiskNewPartition(const SDeviceInfo &devName, const SPartitionInfo &freeName);
    QList<SPartitionInfo> getPartitionConfig(const SDeviceInfo &dev, QString &err);

private:
    // 无人值守选盘策略
    SPartitionInfo findFreePartition();
    QList<SDeviceInfo> getSelectDevice(const QStringList &infos, const QList<SDeviceInfo> &devs, bool isSaveData = false);

private:
    PartitionAutoManager() = default;
    PartitionAutoManager(const PartitionAutoManager&) = delete;
    static PartitionAutoManager *m_instance;
};

#endif // PARTITIONAUTOMANAGER_H
