#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "structs.h"
#include "ThreadObject.h"

#include <QTimer>
#include <QString>
#include <QObject>
#include <QSharedPointer>
#include <QMap>
#include <QDebug>
#include <QReadWriteLock>
#include <atomic>

class DeviceManager : public QObject
{
    Q_OBJECT

private:
    DeviceManager();
    DeviceManager(const DeviceManager &s);
    void operator =(const DeviceManager &s);

public:
    static DeviceManager *instance();
    void scanDevices(bool start);
    void scanUSBDevices(bool start);

    QString getOldEfi() {
        return m_oldefi;
    }

    QMap<SDeviceInfo, QList<SPartitionInfo>> getDeviceInfos();
    QMap<SVGInfo, QList<SLVMInfo>> getLVMInfos();
    QList<SDeviceInfo> getUSBDeviceInfos();//同时执行设备挂载操作

signals:
    void deviceChanged(const QString &path);
    void usbDeviceChanged(const QString &path);

public slots:
    void quitUSBScan();

private:
    void scanDeviceInfos(QString &err);

    void updateUSBDeviceInfo(QList<SDeviceInfo> &deviceInfo);

    void scanUSBDeviceInfos(QString &err);

    void scanLvmInfos(QMap<SDeviceInfo, QList<SPartitionInfo> > devs, QMap<QString, SVGInfo> &vgnames, QMap<QString, SLVMInfo> &lvnames);

    // 使用lsblk命令获取json格式的设备信息
    bool getDeviceInfoFromJson(QJsonObject &deviceinfoobj, QString &err);

    // 使用lsblk命令获取json格式的设备信息
    bool getVGnamesInfo(QMap<QString, SVGInfo> &vgnames, QString &err);

    // 更新分区信息，判断该分区是否在vg中
    void updatePartInfoByVG(QMap<QString, SVGInfo> &vgNames, SPartitionInfo &partitionInfo);

    void updateVGNames(const QJsonObject &partinfoobj, QMap<QString, SVGInfo> &vgnames, QMap<QString, SLVMInfo> &lvnames);

    // 使用fdisk针对每个设备更新分区信息
    bool updatePartInfosByFdisk(SDeviceInfo &deviceinfo, QList<SPartitionInfo> &partitioninfos, QString &err);

    // 使用parted针对每个设备更新其下分区的Type
    bool updatePartType(const SDeviceInfo &deviceinfo, QList<SPartitionInfo> &partitioninfos, QString &err);

    // 更新空闲空间的信息
    void updateFreeSpace(SDeviceInfo &deviceinfo, QList<SPartitionInfo> &partitioninfos);
    SPartitionInfo newFreePartition(const SDeviceInfo &deviceinfo, qint64 size, qint64 startpoint, qint64 endpoint);

    // 删除小于1M的分区
    void removePartLessThanOneM(QList<SPartitionInfo> &partitioninfos);

    SDeviceInfo newDeviceByJobj(const QJsonObject &deviceinfoobj);
    SPartitionInfo newPartitionByJobj(const SDeviceInfo &deviceinfo, const QJsonObject &partitioninfoobj);

    // 根据配置的设备类型过滤设备，返回过滤后剩余设备的数量
    int filterDevice(QJsonArray &devicearray);

    // 挂载一个路径
    QList<SDeviceInfo> mountUSBDevices(const QList<SDeviceInfo> &infoList);
    bool mountPath(const QString &devPath, const QString &mountPoint, QString &err);

    // 查找启动盘设备
    void findStartDevice(const QJsonArray &devicearray, QString &startDev, QString &startDevPart, bool &findStart);

    void printDevInfos(QMap<SDeviceInfo, QList<SPartitionInfo>> &devicesInfos, QMap<SVGInfo, QList<SLVMInfo>> &lvmInfos);

private:
    static DeviceManager *m_instance;
    std::atomic_bool m_enableScanUSBDevice;
    QReadWriteLock m_lock;
    bool m_deviceUpdated = false;
    QMap<SDeviceInfo, QList<SPartitionInfo>> m_devicesInfos;
    QMap<SVGInfo, QList<SLVMInfo>> m_lvmInfos;
    QList<SDeviceInfo> m_usbDeviceInfos;
    QString m_oldefi;
};

#endif // DEVICEMANAGER_H
