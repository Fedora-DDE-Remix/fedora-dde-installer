#ifndef PARTITIONCUSTOMMANAGER_H
#define PARTITIONCUSTOMMANAGER_H

#include "structs.h"

#include <QMap>

class PartitionCustomManager {
public:
    static PartitionCustomManager *instance();

public:
    QMap<SDeviceInfo, QList<SPartitionInfo>> getSystemPartitionInfo() const;
    QMap<SVGInfo, QList<SLVMInfo>> getSystemLVMInfo() const;

    void newPartition(const SDeviceInfo &devName, const SPartitionInfo &freeName, qint64 systemSpace, qint64 dataSpace);
    void editPartition(const SDeviceInfo &devName, SPartitionInfo &part);
    void delPartition(const SDeviceInfo &devName, const SPartitionInfo &part);
    void delVG(const SVGInfo &vgName);

    bool isFormatDisk(const SDeviceInfo &devName);
    SPartitionInfo find(const SDeviceInfo &dev, const SPartitionInfo &part);
    SPartitionInfo find(const QString id);

    SDeviceInfo findDev(const QString &devName);
    SDeviceInfo findDev(const SDeviceInfo &dev);

    SVGInfo findVG(const SVGInfo &vgName);

    void setFormatDisk(const SDeviceInfo &devName);
    bool isNewSystem(const SDeviceInfo &devName);
    int getNewPrimaryPartitionCnt(const SDeviceInfo &devName);
    qint64 getSystemMinSpace(const SDeviceInfo &devName);

    bool checkPartition(QString &log);
    bool findFirstESP(SPartitionInfo &info);

    bool isMultipleDiskToVg(const SDeviceInfo &devName, QMap<QString, QString> &text);

private:
    SPartitionInfo getEFIPartition();
    bool hasBIOSBootPartition(const SDeviceInfo &devName) const;
    bool existSWAPPartition();
    bool existLogicalPartition(const SDeviceInfo &devName);
    int getPrimaryPartitionCnt(const SDeviceInfo &devName);

    SPartitionInfo getRootaPartitionInfo(qint64 size, const SDeviceInfo &dev);
    SPartitionInfo getSWAPPartitionInfo(const SDeviceInfo &dev);
    SPartitionInfo getDataPartitionInfo(qint64 size, const SDeviceInfo &dev);

private:
    PartitionCustomManager() = default;
    PartitionCustomManager(const PartitionCustomManager&) = delete;
    static PartitionCustomManager *m_instance;

private:
};

#endif // PARTITIONCUSTOMMANAGER_H
