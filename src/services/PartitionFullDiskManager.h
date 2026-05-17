#ifndef PARTITIOONFULLDISKMANAGER_H
#define PARTITIOONFULLDISKMANAGER_H

#include "structs.h"

#include <QJsonValue>
#include <QString>

class PartitionFullDiskManager {

public:
    static PartitionFullDiskManager *instance();

public:
    void setPartitionMode(int mode);
    bool addSystemDisk(const SDeviceInfo &dev, QString &err);
    bool addHomeDisk(const SDeviceInfo &dev, QString &err);

    SDeviceInfo getSystemDev() const;
    SDeviceInfo getHomeDev() const;

    void delSystemDev();
    void delHomeDev();
    bool delHomeDev(QString &err);

    // 调整系统空间大小，单位G
    void resizeSystemPartition(qint64 size);

    // 返回系统空间的大小，单位G
    qint64 getSystemSpaceSize();

    // 返回数据空间的大小，单位G
    qint64 getDataSpaceSize();

    // 是否存在数据盘
    bool isExistsHomeDisk();

    //检查ghost模式空间
    bool checkGhostModeSpace(QString &err);

    void resetDisk();
    // 全盘lvm和全盘加密
    void addFullDiskLVM(const QString &fileSystem);
    //保留用户数据
    void saveUserData();
    // 检查当前选择的磁盘是否符合保留用户数据的要求
    KeepDataError checkSaveUserData(QString &oldVersion);

    QString findDataPartitionByFsTab(const QStringList &partList);

    bool checkPartitionInDevice(const QString &uuid, const SDeviceInfo &dev);

    bool isMultipleDiskToVg(const SDeviceInfo &devName, QMap<QString, QString> &text);

    bool checkSaveUserDataOSVersionMatch(const QString &devPath, QString &oldVersion);

    qint64 defaultPartitionSize(const qint64 devSize, const qint64 total, const QString &size, const QString &label);
    SPartitionInfo getConfigToPartition(const QJsonValue &value);

    qint64 getConfigPartitionSize(const qint64 devSize, const QString &label);

    qint64 getOtherPartitionSize(qint64 totalSize);

    void getDeviceCryptPartitions(const SDeviceInfo &devInfo, QList<SPartitionInfo> &partEncryptList);

    QList<SPartitionInfo> getSystemPartitionConfig(const SDeviceInfo &dev, QString &err);
    QList<SPartitionInfo> getHomePartitionConfig(const SDeviceInfo &dev, QString &err);

private:
    SLVMInfo partToLvInfo(const SPartitionInfo &part);

    // 获取引导分区以外的分区
    QMap<SDeviceInfo, QList<SPartitionInfo>> getNotBootloaderPartition();
    QString getVGName();

private:
    PartitionFullDiskManager() = default;
    PartitionFullDiskManager(const PartitionFullDiskManager&) = delete;
    static PartitionFullDiskManager *m_instance;

private:
    SDeviceInfo m_systemDisk = SDeviceInfo();               // 系统盘
    SDeviceInfo m_homeDisk = SDeviceInfo();                 // Home盘
    qint64 m_rootaPartitionResize = 0;         // 根分区调整增加的大小
};

#endif // PARTITIOONFULLDISKMANAGER_H
