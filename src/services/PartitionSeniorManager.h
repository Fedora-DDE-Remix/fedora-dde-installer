#ifndef PARTITIONSENIORMANAGER_H
#define PARTITIONSENIORMANAGER_H

#include "structs.h"

#include <QMap>

class PartitionSeniorManager {
    enum class FindCond: int{
            ID,
            MOUNTPOINT
        };
public:
    static PartitionSeniorManager *instance();

public:
    QMap<SDeviceInfo, QList<SPartitionInfo>> getSystemPartitionInfo() const;
    QMap<SVGInfo, QList<SLVMInfo>> getSystemLVMInfo() const;

    QList<SPartitionInfo> getPartitionListByFileSystem(const QString &fileSystem) const;

    SPartitionInfo find(const SDeviceInfo &dev, const SPartitionInfo &part);
    SPartitionInfo find(const QString &id);
    SPartitionInfo findAvailablePartition(const QString &id);

    SDeviceInfo findDev(const QString &devName);
    SDeviceInfo findDev(const SDeviceInfo &dev);
    SDeviceInfo findAvailableDev(const QString &devName);

    SVGInfo findVG(const SVGInfo &vgName);
    SVGInfo findVG(const QString &vgName);
    SVGInfo findLvToVG(const QString &lvID);
    SLVMInfo findLV(const QString &id);
    void updateEncryptStatus(const QString & partId, const QString &pwd, bool bAutoDecrypt, bool bEncrypt);
    bool isInCryptList(const QString &partId);
    void clearCryptList();

    quint64 getMinGhostPartitionSize(const QString &partmt);

    void newPartition(const SDeviceInfo &devName, const SPartitionInfo &freeName, SPartitionInfo &part);
    void editPartition(const SDeviceInfo &devName, SPartitionInfo &part);
    SPartitionInfo delPartition(const SDeviceInfo &devName, const SPartitionInfo &partIndex); //返回新的空闲空间
    //
    bool partitionEncryptIsReady(const QString &partId);

    void newVG(const QString &vgName, const QList<SPartitionInfo> &pvList);
    void delVG(const SVGInfo &vgName);

    void newLV(const SLVMInfo &freeName, const SLVMInfo &lv);
    void editLV(const SLVMInfo &lv);
    void delLV(const SLVMInfo &lv);
    bool checkLVMNameRepeat(const SLVMInfo &newLv, QList<SLVMInfo> &lvList);

    //　磁盘列表重置
    void resetDisk();

    // 检查是否存在EFI分区
    SPartitionInfo getEFIPartition();
    SPartitionInfo getBootPartition();

    bool existSWAPPartition(bool fromCurrentDevInfo = false);

    bool existVGName(const QString &vgName);
    // 检查分区是否满足系统安装的条件
    bool checkPartition(QString &log);
    //
    bool checkGhostPartition(QString &err);
    // 查找磁盘的第一个引导分区
    bool findFirstESP(SPartitionInfo &bootloader);

    // 磁盘是否能新建主分区
    bool isNewPrimaryPartition(const SDeviceInfo &dev);
    // 当前空闲分区所在的位置是否能新建主分区
    bool isNewPrimaryPartition(const SDeviceInfo &dev, const SPartitionInfo &part);
    // 是否能新建逻辑分区
    bool isNewLogicalPartition(const SDeviceInfo &dev, const SPartitionInfo &freeName);
    // 是否能从空闲分区起始位置创建逻辑分区
    bool isNewStartLogicalPartition(const SDeviceInfo &dev, const SPartitionInfo &free);
    // 是否能从空闲分区结束位置创建逻辑分区
    bool isNewEndLogicalPartition(const SDeviceInfo &dev, const SPartitionInfo &free);
    // 是否格式化磁盘
    bool isFormatDisk(const SDeviceInfo &devName);
    void setFormatDisk(const SDeviceInfo &devName);

    // 获取该磁盘下分区和成的VG
    QList<SVGInfo> getDevMergeVg(const SDeviceInfo &devName);
    // 判断vg是否由多个磁盘中的分区创建
    bool isMultipleDiskToVg(const SDeviceInfo &devName, QMap<QString, QString> &text);

    // 是否已经创建过swap分区
    bool isHaveCreatedSwap(QString &swapPartitionId);

    //分区新建,编辑弹窗参数校验接口
    bool checkDialogNextEnable(const SPartitionInfo &info, QString &err);
    bool checkDialogNextEnable(const SLVMInfo &info, QString &err);

    bool checkNewRootPartitionIsBootloader(const SPartitionInfo &info);

    // 获取一个大小大于或者等于size的空闲区域
    SPartitionInfo getFreePartInfo(const SDeviceInfo &dev, qint64 size);

    QString getVGName();
    SLVMInfo getFreeLVMInfo(const SVGInfo &vg);
    bool isExistSameMountpointAsThePrimaryPartition(const QString &mountPoint);

    // 获取空盘中的空闲分区
    SPartitionInfo getEmptyDiskPartition(const SDeviceInfo &dev);

    // 设置保存加密头的分区信息
    void setHideCryptParts(const SDeviceInfo &dev);

private:
    bool findUsagePartition(const QString &value, int &index, SPartitionInfo &partition, FindCond cond = FindCond::MOUNTPOINT);

    SPartitionInfo mergePartitions(const SPartitionInfo &fir, const SPartitionInfo &sec);
    // 检查当前分区中是否存在逻辑分区
    bool existLogicalPartition(const QList<SPartitionInfo> &partLists);
    SPartitionInfo expandExtensionPartition(const SDeviceInfo &dev, SPartitionInfo &part);
    SPartitionInfo addExtensionPartition(const SDeviceInfo &dev, SPartitionInfo &part);
    SPartitionInfo findExtensionPartition(const QList<SPartitionInfo> &parts);
    QList<SPartitionInfo> getOperatorPartitions(const SDeviceInfo &dev);
    bool isStartLogicalPartition(const SDeviceInfo &dev, const SPartitionInfo &in);
    bool isEndLogicalPartition(const SDeviceInfo &dev, const SPartitionInfo &in);

    qint64 getExtensionPartitonsSize(const QList<SPartitionInfo> &infos);

    void resetMountPoint(const QString &mountPoint);

    bool isPartitionCanEncrypt(const SPartitionInfo &partInfo);

    bool isPartitionCanEncrypt(const SLVMInfo &lvmInfo);

    bool getCurrentEncryptStatus(const QString &partId);
private:
    PartitionSeniorManager() = default;
    PartitionSeniorManager(const PartitionSeniorManager&) = delete;
    static PartitionSeniorManager *m_instance;
    QStringList m_cryptCandidateList;
    QPair<bool, QString> m_encryptPart;
};

#endif // PARTITIONSENIORMANAGER_H
