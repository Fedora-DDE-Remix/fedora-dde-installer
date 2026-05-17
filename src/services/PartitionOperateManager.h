#ifndef PARTITIONOPERATEMANAGER_H
#define PARTITIONOPERATEMANAGER_H

#include "structs.h"
#include "globals.h"

#include <QMap>
#include <QJsonArray>

class PartitionOperateManager {

public:
    static PartitionOperateManager *instance();

public:
    void setPartitionMode(int mode);
    void setLVMInfo(const QMap<SVGInfo, QList<SLVMInfo>> &infos);
    void setDeviceInfo(const QMap<SDeviceInfo, QList<SPartitionInfo> > &infos);

    //　系统系统分区
    QList<SPartitionInfo> getSystemPartitions(const SDeviceInfo &in);
    void updateDeviceInfo(const SPartitionInfo &info);
    void updateDeviceInfo(const SDeviceInfo &dev, const SPartitionInfo &info);
    void updateDeviceInfo(const SDeviceInfo &dev, const QList<SPartitionInfo> &infos);

    QList<SLVMInfo> getSystemLVMPartitions(const SVGInfo &in);
    void updateLVMInfo(const SVGInfo &dev, const SLVMInfo &infos);
    void updateLVMInfo(const SVGInfo &dev, const QList<SLVMInfo> &infos);
    void deleteLVMInfo(const SVGInfo &vg);

    QMap<SDeviceInfo, QList<SPartitionInfo>> getCurrentDeviceInfo();
    QMap<SVGInfo, QList<SLVMInfo>> getCurrentLVMInfo();

    QMap<SDeviceInfo, QList<SPartitionInfo>> getAvailableDeviceInfo();
    QMap<SVGInfo, QList<SLVMInfo>> getAvailableLVMInfo();

    // 获取的设备信息
    QList<SDeviceInfo> getCurrentDevice();

    QList<SDeviceInfo> getAvailableDevice();

    // 获取VG信息
    QList<SVGInfo> getCurrentVG();

    //检查备份分区空间是否足够，仅限ghost全盘安装
    bool checkBackupSize();

    QString getPartedOperatorInfo();

    bool isMultipleDiskToVg(const SDeviceInfo &dev, QMap<QString, QString> &text);
    bool writeConf(QString &err);
    void printPartitions(const SVGInfo &vg);
    void printPartitions(const QMap<SVGInfo, QList<SLVMInfo>> &devsInfo);
    void printPartitions(const SDeviceInfo &dev);
    void printPartitions(const QMap<SDeviceInfo, QList<SPartitionInfo>> &devsInfo);

    // 磁盘格式化操作
    void setFormatDisk(const SDeviceInfo &dev);
    void resetDisk(const SDeviceInfo &dev);
    void resetDisk();
    void resetVGDevice();

    bool isBaseInstallMode() const;

    void setUimgDirSize(QMap<QString, quint64> &dirSize);
    QMap<QString, quint64> getUimgDirSize();

    quint64 getGhostPartitionSize(const QString &partmt);

    void mergePartitionInfo(QList<GhostCheckInfo> &ghostCheckList);

    quint64 getGhostRootSize(const QList<GhostCheckInfo> &ghostCheckList, QString &err);

    bool checkGhostSpace(QString &err);
private:
    //　清理磁盘信息
    void newPartConf(const SPartitionInfo &in, const QList<SPartitionInfo> &infos, QJsonArray &jsons);
    void delPartConf(const SPartitionInfo &in, const QList<SPartitionInfo> &infos, QJsonArray &jsons);
    void editPartConf(const SPartitionInfo &in, const QList<SPartitionInfo> &infos, QJsonArray &jsons);
    void editExtensinoPartConf(const SPartitionInfo &in, const QList<SPartitionInfo> &infos, QJsonArray &jsons);
    void newLVMConf(const SLVMInfo &in, const QList<SLVMInfo> &infos, QJsonArray &jsons);
    void delLVMConf(const SLVMInfo &in, const QList<SLVMInfo> &infos, QJsonArray &jsons);
    void editLVMConf(const SLVMInfo &in, const QList<SLVMInfo> &infos, QJsonArray &jsons);
    void newVGConf(const SVGInfo &in, const QList<SVGInfo> &infos, QJsonArray &jsons);
    void delVGConf(const SVGInfo &in, const QList<SVGInfo> &infos, QJsonArray &jsons);
    void formatDiskConf(const SDeviceInfo &in, QJsonArray &jsons);
    void formatLVMConf(const SDeviceInfo &in, QJsonArray &jsons);

    QJsonObject getOperateJsonObject(const QString &oper, const SPartitionInfo &in);
    QJsonObject getOperateJsonObject(const QString &oper, const SDeviceInfo &in);
    QJsonObject getOperateJsonObject(const QString &oper, const SVGInfo &in);
    QJsonObject getOperateJsonObject(const QString &oper, const SLVMInfo &in);

    void setOthenOperateInfo();
    void setOperateInfo(const QString &op, const SPartitionInfo &in);
    void setOperateInfo(const QString &op, const SDeviceInfo &in);
    void setOperateInfo(const QString &op, const SVGInfo &in);
    void setOperateInfo(const QString &op, const SLVMInfo &in);

    SVGInfo findVG(const QString &pvID);

private:
    PartitionOperateManager() = default;
    PartitionOperateManager(const PartitionOperateManager&) = delete;
    static PartitionOperateManager *m_instance;

private:
    QStringList m_operateInfo;
    QJsonArray  m_operateJsonArray;

    QMap<SDeviceInfo, QList<SPartitionInfo>> m_availableDeviceInfo;     // 可用的安装设备信息
    QMap<SDeviceInfo, QList<SPartitionInfo>> m_currentDeviceInfo;       // 用户界面操作后的设备详情

    QMap<SVGInfo, QList<SLVMInfo> >             m_currentLVMInfo;          // 用户界面操作后的LVM分区
    QMap<SVGInfo, QList<SLVMInfo> >             m_availableLVMInfo;        // 可用的LVM设备信息
    QMap<QString, quint64> m_uimgDirSize; //uimg镜像记录的目录的大小（见uimg镜像中的info.json的dirSize字段）
};

#endif // PARTITIONOPERATORMANAGER_H
