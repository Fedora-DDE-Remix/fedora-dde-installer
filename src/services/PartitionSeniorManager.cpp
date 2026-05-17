#include "PartitionSeniorManager.h"
#include "PartitionOperateManager.h"
#include "CommonFunc.h"
#include "DeviceManager.h"
#include <QUuid>
#include <QDebug>
#include <QDateTime>
#include <QTimeZone>
#include <cmath>

PartitionSeniorManager *PartitionSeniorManager::m_instance = nullptr;
PartitionSeniorManager *PartitionSeniorManager::instance()
{
    if(m_instance == nullptr)
        m_instance = new PartitionSeniorManager();
    return m_instance;
}

QMap<SDeviceInfo, QList<SPartitionInfo>> PartitionSeniorManager::getSystemPartitionInfo() const
{
    return PartitionOperateManager::instance()->getCurrentDeviceInfo();
}

QMap<SVGInfo, QList<SLVMInfo>> PartitionSeniorManager::getSystemLVMInfo() const
{
    return PartitionOperateManager::instance()->getCurrentLVMInfo();
}

QList<SPartitionInfo> PartitionSeniorManager::getPartitionListByFileSystem(const QString &fileSystem) const
{
    QList<SPartitionInfo> infoList{};
    QMap<SDeviceInfo, QList<SPartitionInfo>> devs = getSystemPartitionInfo();
    for (QMap<SDeviceInfo, QList<SPartitionInfo>>::const_iterator deviceiter = devs.cbegin();
         deviceiter != devs.cend(); deviceiter++) {
        for (SPartitionInfo partinfo : deviceiter.value()) {
            if (!gLVM2PVFileSystem.compare(fileSystem)) {
                if (ESpaceUsage::USE == partinfo.usage && !partinfo.isMergeVG
                        && gLVM2PVFileSystem == partinfo.filesystem) {
                    infoList.append(partinfo);
                }
            } else {
                if (fileSystem == partinfo.filesystem) {
                    infoList.append(partinfo);
                }
            }
        }
    }
    return infoList;
}

bool PartitionSeniorManager::isExistSameMountpointAsThePrimaryPartition(const QString &mountPoint)
{
    QMap<SDeviceInfo, QList<SPartitionInfo>> devs = getSystemPartitionInfo();
    for (QMap<SDeviceInfo, QList<SPartitionInfo>>::const_iterator deviceiter = devs.cbegin();
         deviceiter != devs.cend(); deviceiter++) {
        for (SPartitionInfo partinfo : deviceiter.value()) {
            if (!mountPoint.isEmpty() && !partinfo.mountPoint.compare(mountPoint)) {
               return true;
            }
        }
    }
    return false;
}

SPartitionInfo PartitionSeniorManager::getEmptyDiskPartition(const SDeviceInfo &dev)
{
    QMap<SDeviceInfo, QList<SPartitionInfo>> infos = PartitionSeniorManager::instance()->getSystemPartitionInfo();

    SPartitionInfo free;
    if (!infos.value(dev).isEmpty()) {
        // 更新空闲空间的起点位置从1M开始
        free = infos.value(dev).first();
        qint64 totaldevicesectors = dev.byteSize / dev.sectors;
        free.startPoint = (giDeviceUnitSize * giDeviceUnitSize) / free.sectorSize;
        free.endPoint = totaldevicesectors - (giDeviceUnitSize * giDeviceUnitSize) / free.sectorSize;
        free.byteSize = (free.endPoint + 1 - free.startPoint) * free.sectorSize;
        free.size = CommonFunc::bytesToM(free.byteSize);
        PartitionOperateManager::instance()->updateDeviceInfo(dev, {free});
    }

    return free;
}

// 设置保存加密头的分区信息，设置固定大小的分区
void PartitionSeniorManager::setHideCryptParts(const SDeviceInfo &dev)
{
    if (!CommonFunc::getSettingsBool("DI_USE_HIDE_CRYPT_HEADER")) {
        return;
    }

    QList<SPartitionInfo> devParts = PartitionOperateManager::instance()->getCurrentDeviceInfo().value(dev);
    QList<SVGInfo> vgInfos = PartitionOperateManager::instance()->getCurrentLVMInfo().keys();
    QList<SLVMInfo> lvmParts;
    QString vgName = QString("usec-vg-%1").arg(dev.ptuuid);

    // 查找保存加密头信息的逻辑卷组，没有则在磁盘末尾创建，有则清理无效记录，更新新的记录
    bool needCreateHideCryptHeader = true;
    QList<SVGInfo>::iterator vgIter = std::find_if(vgInfos.begin(), vgInfos.end(), [&](const SVGInfo &vgInfo) {
        return vgInfo.name == vgName;
    });
    if (vgIter == vgInfos.end()) {
        // 判断vg是否在该dev上
        for (const SPartitionInfo &devPart : devParts) {
            vgName = QString("usec-vg-%1").arg(devPart.id);
            vgName.replace("{", "").replace("}", "");
            vgIter = std::find_if(vgInfos.begin(), vgInfos.end(), [&](const SVGInfo &vgInfo) {
                return vgInfo.name == vgName;
            });
            if (vgIter != vgInfos.end()) {
                needCreateHideCryptHeader = false;
                lvmParts = PartitionOperateManager::instance()->getCurrentLVMInfo().value(*vgIter);
                break;
            }
        }
    } else {
        needCreateHideCryptHeader = false;
        lvmParts = PartitionOperateManager::instance()->getCurrentLVMInfo().value(*vgIter);
    }

    if (needCreateHideCryptHeader) {
        // 从磁盘末尾创建固定大小85Mib的逻辑卷组
        SPartitionInfo free = PartitionSeniorManager::instance()->getFreePartInfo(dev, 80);
        if (!free.id.isEmpty()) {
            SPartitionInfo pvPart;
            pvPart.partitionType = EPartitionType::PartitionTypePrimary;
            pvPart.filesystem = gLVM2PVFileSystem;
            pvPart.size = 85;
            pvPart.byteSize = pvPart.size * giDeviceUnitSize * giDeviceUnitSize;
            pvPart.usage = ESpaceUsage::USE;
            pvPart.isNeedFormat = true;
            pvPart.isStartPoint = false; // 设置从空闲空间的末尾开始分区
            pvPart.isCryptHeader = true;

            PartitionSeniorManager::instance()->newPartition(dev, free, pvPart);
            vgName = QString("usec-vg-%1").arg(pvPart.id);
            vgName.replace("{", "").replace("}", "");
            qInfo() << "new vgName is : " << vgName;
            PartitionSeniorManager::instance()->newVG(vgName, QList<SPartitionInfo>{pvPart});

            // 更新列表中的vg信息设置isCryptHeader为true
            QMap<SVGInfo, QList<SLVMInfo> > vgLvmInfos = PartitionOperateManager::instance()->getCurrentLVMInfo();
            vgInfos = vgLvmInfos.keys();
            QList<SVGInfo>::Iterator updateVgInfoIter = std::find_if(vgInfos.begin(), vgInfos.end(), [&](const SVGInfo &vgInfo) {
                return vgInfo.name == vgName;
            });
            if (updateVgInfoIter != vgInfos.end()) {
                SVGInfo updateVgInfo = (*updateVgInfoIter);
                updateVgInfo.isCryptHeader = true;
                PartitionOperateManager::instance()->updateLVMInfo(updateVgInfo, vgLvmInfos.value(updateVgInfo));
            }
        } else {
            return;
        }
    } else {
        // 遍历PartitionOperateManager::instance()->getCurrentLVMInfo().value(*vgIter)，清理无效的记录
        lvmParts = PartitionOperateManager::instance()->getCurrentLVMInfo().value(*vgIter);
        for (SLVMInfo lvmPartItem: lvmParts) {
            if (lvmPartItem.name.isEmpty()) {
                continue;
            }

            QList<SPartitionInfo>::const_iterator removeIter = std::find_if(devParts.begin(), devParts.end(), [=](const SPartitionInfo &part) {
                return ! part.partUuid.isEmpty() && lvmPartItem.name.contains(part.partUuid);
            });
            if (removeIter == devParts.end()) {
                // 清理无效的记录
                delLV(lvmPartItem);
            } else {
                if ((*removeIter).isCrypt) {
                    delLV(lvmPartItem);
                }
            }
        }
    }

    vgInfos = PartitionOperateManager::instance()->getCurrentLVMInfo().keys();
    vgIter = std::find_if(vgInfos.begin(), vgInfos.end(), [&](const SVGInfo &vgInfo) {
        return vgInfo.name == vgName;
    });
    if (vgIter != vgInfos.end()) {
        lvmParts = PartitionOperateManager::instance()->getCurrentLVMInfo().value(*vgIter);
    }

    // 更新新的记录，遍历devParts，根据存在的分区在usec-vg-xxx中创建对应的的加密头分区
    devParts = PartitionOperateManager::instance()->getCurrentDeviceInfo().value(dev);
    for (SPartitionInfo &devPart: devParts) {
        // 已明确在手动分区界面进行加密的分区，不需要考虑在隐藏分区中
        if (devPart.isCrypt) {
            continue;
        }

        if (devPart.usage == ESpaceUsage::FREE) {
            continue;
        }

        // 过滤掉空挂载点分区、引导分区、recovery分区
        if (devPart.mountPoint.isEmpty() || devPart.mountPoint == "/boot" || devPart.mountPoint == "/boot/efi" || devPart.mountPoint == "/recovery") {
            continue;
        }

        // 过滤掉非ext4、ext3、ext2的文件系统
        QStringList cryptFileSystems = {"ext4", "ext3", "ext2"};
        if (!cryptFileSystems.contains(devPart.filesystem)) {
            continue;
        }

        // 更新新的记录
        QString lvmPartName = QString("lv-%1").arg(devPart.partUuid.isEmpty() ? devPart.id : devPart.partUuid);
        lvmPartName.replace("{", "").replace("}", "");
        if (std::find_if(lvmParts.begin(), lvmParts.end(), [&](const SLVMInfo &lvmPart) {
            return lvmPart.name.contains(lvmPartName);
        }) == lvmParts.end()) {
            if (lvmParts.size() >= 5) {
                qInfo() << "lvmParts more than 5 no lvm part will create with " << devPart.label;
                continue;
            }
            SLVMInfo freeLv = PartitionSeniorManager::instance()->getFreeLVMInfo(PartitionSeniorManager::instance()->findVG(vgName));
            SLVMInfo lv;
            lv.id            = QUuid::createUuid().toString();
            lv.name          = lvmPartName;
            lv.filesystem    = "ext4";
            lv.mountPoint    = "";
            lv.label         = lvmPartName;
            lv.path          = "";
            lv.size          = 16;
            lv.byteSize      = lv.size * giDeviceUnitSize * giDeviceUnitSize;
            lv.vgName        = vgName;
            lv.cryptPassword = "";
            lv.usage         = ESpaceUsage::USE;
            lv.isNeedFormat  = true;
            lv.isCanCrypt    = false;
            lv.isCrypt       = false;
            lv.decryptStatus = false;
            lv.isCryptHeader  = true;

            PartitionSeniorManager::instance()->newLV(freeLv, lv);

            devPart.isNeedCryptHeader = true;
            PartitionOperateManager::instance()->updateDeviceInfo(dev, devPart);
        }
    }
}

void PartitionSeniorManager::newPartition(const SDeviceInfo &devName, const SPartitionInfo &freeName, SPartitionInfo &part)
{
    //qInfo() << "new partition start....";
    // 处理重复挂载点
    this->resetMountPoint(part.mountPoint);

    // 查找当前磁盘
    SDeviceInfo dev = findDev(devName);

    // 获取当前磁盘上的分区
    QList<SPartitionInfo> partitions = PartitionOperateManager::instance()->getSystemPartitions(dev);

    if (!dev.device.isEmpty()) {
        // 获取空闲分区
        SPartitionInfo free = find(dev, freeName);

        // 新建分区
        SPartitionInfo in = free;
        in.id = QUuid::createUuid().toString();
        in.isCrypt = part.isCrypt;
        in.cryptPassword = part.cryptPassword;
        in.usage = ESpaceUsage::USE;
        in.partitionType = part.partitionType;
        in.size = part.size;
        in.byteSize = part.byteSize;
        in.filesystem = part.filesystem;
        in.mountPoint = part.mountPoint;
        in.label = CommonFunc::getPartitionLabel(part);
        in.isStartPoint = part.isStartPoint;
        in.isNeedFormat = part.isNeedFormat;
        in.isCryptHeader = part.isCryptHeader;
        in.isEFIPartition = (part.mountPoint == "/boot/efi");
        if (in.size < free.size) {
            if (in.isStartPoint) {
                in.startPoint = CommonFunc::getPartitionStartPoint(free.startPoint, free.sectorSize);
                in.endPoint = in.startPoint + (in.byteSize / in.sectorSize) - 1;
                in.endPoint = in.endPoint > free.endPoint ? free.endPoint : in.endPoint;
            } else {
                in.endPoint = free.endPoint;
                in.startPoint = in.endPoint - (in.byteSize / in.sectorSize) + 1;
                in.startPoint = CommonFunc::getPartitionStartPoint(in.startPoint, in.sectorSize);
            }
        }
        // 调整了起止点后需要更新大小信息
        in.byteSize = (in.endPoint + 1 - in.startPoint) * in.sectorSize;
        in.size = CommonFunc::bytesToM(in.byteSize);
        bool canCrypt = in.isNeedFormat && isPartitionCanEncrypt(in);
        in.isCanCrypt = canCrypt && getCurrentEncryptStatus(in.id);
        if(canCrypt) {
            m_cryptCandidateList.append(in.id);
        }

        SPartitionInfo freeExtens = in;

        // 处理扩展分区
        if (part.partitionType == EPartitionType::PartitionTypeLogical) {
            SPartitionInfo extensionPart;
            if (this->existLogicalPartition(partitions)) {
                extensionPart = this->expandExtensionPartition(dev, in);
            } else {
                // 兼容某些磁盘上只创建了扩展分区，没有在扩展分区下面创建逻辑分区的情况
                SPartitionInfo reset = findExtensionPartition(PartitionOperateManager::instance()->getSystemPartitions(dev));
                if (!reset.id.isEmpty()) {
                    partitions.removeAll(reset);
                }

                extensionPart = this->addExtensionPartition(dev, in);
            }

            partitions.removeAll(extensionPart);
            partitions.append(extensionPart);
        }

        // 处理空闲分区
        partitions.removeAll(free);
        if (free.byteSize > part.byteSize) {
            // 逻辑分区要处理2个扇区间隔问题
            int partStepValue = freeExtens.partitionType == EPartitionType::PartitionTypeLogical ? 3 : 1;
            if (freeExtens.isStartPoint) {
                free.startPoint = freeExtens.endPoint + partStepValue;
            } else {
                free.endPoint = freeExtens.startPoint - partStepValue;
            }

            // 更新空闲分区大小
            free.byteSize = (free.endPoint + 1 - free.startPoint) * free.sectorSize;
            if (free.byteSize > 0) {
                free.size = CommonFunc::bytesToM(free.byteSize);
                partitions.append(free);
            }
        }

        if(in.isEFIPartition) {
            for (auto &partInfo: partitions) {
                if(partInfo.isEFIPartition) {
                    partInfo.isEFIPartition = false;
                    break;
                }
            }
        }

        part = in;
        partitions.append(in);

        std::sort(partitions.begin(), partitions.end());

        PartitionOperateManager::instance()->updateDeviceInfo(dev, partitions);
        //PartitionOperateManager::instance()->printPartitions(dev);
    }
    //qInfo() << "new partition end!";
}

void PartitionSeniorManager::editPartition(const SDeviceInfo &devName, SPartitionInfo &part)
{
    //qInfo() << "edit partition start...";
    // 处理重复挂载点
    this->resetMountPoint(part.mountPoint);

    // 查找当前磁盘
    SDeviceInfo dev = findDev(devName);

    // 获取当前磁盘上的分区
    QList<SPartitionInfo> partitions = PartitionOperateManager::instance()->getSystemPartitions(dev);

    if (!dev.device.isEmpty()) {

        // 更新分区
        SPartitionInfo curOpPart = find(dev, part);
        curOpPart.filesystem = part.filesystem;
        curOpPart.mountPoint = part.mountPoint;
        curOpPart.isNeedFormat = part.isNeedFormat;
        curOpPart.label = CommonFunc::getPartitionLabel(part);
        curOpPart.path = part.path;
        curOpPart.isEFIPartition = (part.mountPoint == "/boot/efi");
        bool canEncrypt = curOpPart.isNeedFormat && isPartitionCanEncrypt(curOpPart);
        curOpPart.isCanCrypt = canEncrypt && getCurrentEncryptStatus(curOpPart.id);
        if(canEncrypt) {
            m_cryptCandidateList.append(curOpPart.id);
        }

        if(curOpPart.isEFIPartition) {
            for (auto &partInfo: partitions) {
                if(partInfo.isEFIPartition) {
                    partInfo.isEFIPartition = false;
                    break;
                }
            }
        }

        part = curOpPart;
        partitions.removeAll(curOpPart);
        partitions.append(curOpPart);
        std::sort(partitions.begin(), partitions.end());

        PartitionOperateManager::instance()->updateDeviceInfo(dev, partitions);
        //PartitionOperateManager::instance()->printPartitions(dev);
    }

    //qInfo() << "edit partition end!";
}

SPartitionInfo PartitionSeniorManager::delPartition(const SDeviceInfo &devName, const SPartitionInfo &partIndex)
{
    SPartitionInfo mergePartReturn = SPartitionInfo();

    //qInfo() << "delete partition start....";
    // 查找当前磁盘
    SDeviceInfo dev = findDev(devName);

    // 获取当前磁盘上的分区
    QList<SPartitionInfo> partitions = PartitionOperateManager::instance()->getSystemPartitions(dev);

    if (!dev.device.isEmpty()) {
        SPartitionInfo part = find(dev, partIndex);
        if(isInCryptList(part.id)) {
            m_cryptCandidateList.removeAll(part.id);
        }

        //　如果删除的是最后一个逻辑分区，则同步删除扩展分区
        partitions.removeAll(part);
        if (part.partitionType == EPartitionType::PartitionTypeLogical) {
            if (!existLogicalPartition(partitions)) {
                SPartitionInfo oldExtenPart = this->findExtensionPartition(partitions);
                if (!oldExtenPart.id.isEmpty()) {
                    partitions.removeAll(oldExtenPart);
                }
            }
        }

        SPartitionInfo merge = part;
        merge.id = QUuid::createUuid().toString();
        merge.usage = ESpaceUsage::FREE;
        merge.partitionType = EPartitionType::PartitionTypePrimary;
        merge.device = dev.device;
        merge.deviceSize = dev.size;
        merge.sectorSize = dev.sectors;
        merge.phySectorSize = dev.physectors;
        merge.label = "";
        merge.mountPoint = "";
        merge.filesystem = "";
        merge.index = -1;
        merge.path = "";
        merge.partUuid = "";

        //处理空闲分区的合并
        QList<SPartitionInfo> opListParts = getOperatorPartitions(dev);
        int pos = opListParts.indexOf(part);
        int pos_index = pos - 1;
        // 合并当前分区前面的最近空闲分区
        while (pos_index >= 0) {
            SPartitionInfo info = opListParts.at(pos_index);
            if (!info.id.isEmpty()) {
                SPartitionInfo reset = this->mergePartitions(info, merge);
                if (!reset.id.isEmpty()) {
                    partitions.removeAll(info);
                    merge = reset;
                } else {
                    break;
                }
            }
            pos_index--;
        }

        pos_index = pos + 1;
        // 合并当前分区后面的最近空闲分区
        while (pos_index < opListParts.size()) {
            SPartitionInfo info = opListParts.at(pos_index);
            if (!info.id.isEmpty()) {
                SPartitionInfo reset = this->mergePartitions(merge, info);
                if (!reset.id.isEmpty()) {
                    partitions.removeAll(info);
                    merge = reset;
                } else {
                    break;
                }
            }
            pos_index++;
        }

        // 处理扩展分区中连续的空闲逻辑分区合并后，扩展分区大小的调整
        if (part.partitionType == EPartitionType::PartitionTypeLogical) {
            SPartitionInfo oldExtenPart = this->findExtensionPartition(partitions);
            if (!oldExtenPart.id.isEmpty()) {
                if (isStartLogicalPartition(dev, part)) {
                    oldExtenPart.startPoint = merge.endPoint;
                    merge.endPoint -= 1;
                } else if (isEndLogicalPartition(dev, part)) {
                    oldExtenPart.endPoint = merge.startPoint - 1;
                }
                oldExtenPart.size = getExtensionPartitonsSize(partitions);
                partitions.removeAll(oldExtenPart);
                partitions.append(oldExtenPart);
            }
        }

        // 处理分区删除
        merge.byteSize = (merge.endPoint + 1 - merge.startPoint) * merge.sectorSize;
        merge.size = CommonFunc::bytesToM(merge.byteSize);
        partitions.append(merge);
        std::sort(partitions.begin(), partitions.end());

        PartitionOperateManager::instance()->updateDeviceInfo(dev, partitions);
        //PartitionOperateManager::instance()->printPartitions(dev);
        //
        if(m_encryptPart.first && m_encryptPart.second == part.id) {
            updateEncryptStatus(part.id, "", false, false);
        }

        mergePartReturn = merge;
    }
    //qInfo() << "delete partition end!";

    return mergePartReturn;
}

bool PartitionSeniorManager::partitionEncryptIsReady(const QString &partId)
{
    SPartitionInfo bootPart;
    SPartitionInfo encryptPart;
    auto devMap = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    for (auto &dev: devMap.keys()) {
        for (auto &partInfo: devMap.value(dev)) {
            if(partInfo.usage == ESpaceUsage::USE && partInfo.mountPoint == "/boot") {
                bootPart = partInfo;
            } else if (partInfo.id == partId) {
                encryptPart = partInfo;
            }
        }
    }

    return encryptPart.mountPoint != "/" || !bootPart.id.isEmpty();
}

void PartitionSeniorManager::newVG(const QString &vgName, const QList<SPartitionInfo> &pvList)
{
    //qInfo() << "new vg start...";
    if (!pvList.isEmpty()) {
        SVGInfo newVg;
        newVg.id = QUuid::createUuid().toString();
        newVg.name = vgName;

        for (SPartitionInfo partName : pvList) {
            SPartitionInfo part = find(partName.id);
            part.isMergeVG = true;  // 更新分区的isMergeVg标志
            PartitionOperateManager::instance()->updateDeviceInfo(part);

            // 获取vg信息
            newVg.pvIds.append(part.id);
            newVg.size += part.size;
        }

        // 插入空闲空间
        SLVMInfo free;
        free.id = QUuid::createUuid().toString();
        free.usage = ESpaceUsage::FREE;
        free.vgName = newVg.name;
        free.size = newVg.size;
        free.isNeedFormat = true;
        free.path = "";
        PartitionOperateManager::instance()->updateLVMInfo(newVg, QList<SLVMInfo>{free});
        //PartitionOperateManager::instance()->printPartitions(newVg);
    }

    //qInfo() << "new vg end!";
}

void PartitionSeniorManager::delVG(const SVGInfo &vgName)
{
    //qInfo() << "delete vg start...";
    SVGInfo vg = findVG(vgName);
    for (QString id : vg.pvIds) {
        QList<SDeviceInfo> devs = PartitionOperateManager::instance()->getCurrentDevice();
        for (SDeviceInfo dev : devs) {
            QList<SPartitionInfo> parts = PartitionOperateManager::instance()->getSystemPartitions(dev);
            for (int i = 0; i < parts.size(); i++) {
                if (!parts.at(i).id.compare(id)) {
                    parts[i].isMergeVG = false;
                }
            }
            std::sort(parts.begin(), parts.end());

            PartitionOperateManager::instance()->updateDeviceInfo(dev, parts);
            //PartitionOperateManager::instance()->printPartitions(dev);
        }
    }

    QList<SLVMInfo> lvmList = PartitionOperateManager::instance()->getSystemLVMPartitions(vg);
    for (auto &lvm: lvmList) {
        if(isInCryptList(lvm.id)) {
            m_cryptCandidateList.removeAll(lvm.id);
        }

        if(m_encryptPart.first && m_encryptPart.second == lvm.id) {
            updateEncryptStatus(lvm.id, "", false, false);
        }
    }

    PartitionOperateManager::instance()->deleteLVMInfo(vg);

    //qInfo() << "delete vg end!";
}

void PartitionSeniorManager::newLV(const SLVMInfo &freeName, const SLVMInfo &lv)
{
    //qInfo() << "new lv start...";
    this->resetMountPoint(lv.mountPoint);

    SVGInfo vg = findLvToVG(freeName.id);
    if (!vg.id.isEmpty()) {
        auto lvLists = PartitionOperateManager::instance()->getSystemLVMPartitions(vg);

        SLVMInfo newlv = lv;
        newlv.id = QUuid::createUuid().toString();
        newlv.usage = ESpaceUsage::USE;
        newlv.vgName = vg.name;
        if (lv.label != gRootbLabel) {
            newlv.label = CommonFunc::getPartitionLabel(newlv);
        }
        newlv.name = newlv.label;
        newlv.isNeedFormat = lv.isNeedFormat;
        newlv.path = "";
        //检查lvm是否重复
        while (checkLVMNameRepeat(newlv, lvLists)) {
            static uint nameSuffix = 1;
            newlv.name = newlv.label + QString::number(nameSuffix++);
        }

        bool canEncrypt = newlv.isNeedFormat && isPartitionCanEncrypt(newlv);
        newlv.isCanCrypt = canEncrypt && getCurrentEncryptStatus(newlv.id);
        if(canEncrypt) {
            m_cryptCandidateList.append(newlv.id);
        }

        lvLists.append(newlv);

        SLVMInfo newFree = findLV(freeName.id);
        newFree.size = newFree.size - lv.size;
        newFree.path = "";
        lvLists.removeAll(newFree);
        if (newFree.size > 0) {
            lvLists.append(newFree);
        }

        PartitionOperateManager::instance()->updateLVMInfo(vg, lvLists);
        //PartitionOperateManager::instance()->printPartitions(vg);
    }

    //qInfo() << "new lv end!";
}

void PartitionSeniorManager::editLV(const SLVMInfo &lv)
{
    //qInfo() << "edit lv start...";
    this->resetMountPoint(lv.mountPoint);
    SVGInfo vg = findLvToVG(lv.id);
    auto lvLists = PartitionOperateManager::instance()->getSystemLVMPartitions(vg);

    SLVMInfo editLv = findLV(lv.id);
    editLv.filesystem = lv.filesystem;
    editLv.mountPoint = lv.mountPoint;
    editLv.path = lv.path;
    editLv.isNeedFormat = lv.isNeedFormat;
    editLv.label = CommonFunc::getPartitionLabel(editLv);
    editLv.name = editLv.label;
    bool canEncrypt = editLv.isNeedFormat && isPartitionCanEncrypt(editLv);
    editLv.isCanCrypt = canEncrypt && getCurrentEncryptStatus(editLv.id);
    if(canEncrypt) {
        m_cryptCandidateList.append(editLv.id);
    }

    if(editLv.name != lv.name) {
        //移除现有lvm
        lvLists.removeAll(lv);
        editLv.id = QUuid::createUuid().toString();
        editLv.usage = ESpaceUsage::USE;
        editLv.vgName = vg.name;
        while (checkLVMNameRepeat(editLv, lvLists)) {
            static uint nameSuffix = 1;
            editLv.name = editLv.label + QString::number(nameSuffix);
        }

        lvLists.append(editLv);
    } else {
        lvLists[lvLists.indexOf(lv)] = editLv;
    }

    PartitionOperateManager::instance()->updateLVMInfo(vg, lvLists);
    //PartitionOperateManager::instance()->printPartitions(vg);
    //qInfo() << "edit lv end!";
}

void PartitionSeniorManager::delLV(const SLVMInfo &lv)
{
    //qInfo() << "delete lv start...";
    SVGInfo vg = findLvToVG(lv.id);

    auto lvLists = PartitionOperateManager::instance()->getSystemLVMPartitions(vg);
    lvLists.removeAll(lv);
    if(isInCryptList(lv.id)) {
        m_cryptCandidateList.removeAll(lv.id);
    }

    auto freeLv = getFreeLVMInfo(vg);
    if (freeLv.id.isEmpty()) {
        freeLv.id = QUuid::createUuid().toString();
        freeLv.usage = ESpaceUsage::FREE;
    }
    freeLv.size += lv.size;
    freeLv.path = "";
    lvLists.removeAll(freeLv);
    lvLists.append(freeLv);
    PartitionOperateManager::instance()->updateLVMInfo(vg, lvLists);

    if(m_encryptPart.first && m_encryptPart.second == lv.id) {
        updateEncryptStatus(lv.id, "", false, false);
    }
    //qInfo() << "delete lv end!";
}

bool PartitionSeniorManager::checkLVMNameRepeat(const SLVMInfo &newLv, QList<SLVMInfo> &lvList)
{
    bool brepeat = false;
    for (auto &lvm: lvList) {
        if(lvm.name == newLv.name) {
            qInfo() << "find lvm has same name, try to rename";
            brepeat = true;
            break;
        }
    }

    return brepeat;
}

void PartitionSeniorManager::resetDisk()
{
    PartitionOperateManager::instance()->resetDisk();
}

bool PartitionSeniorManager::existVGName(const QString &vgName)
{
    QStringList reset;
    for (auto vg : PartitionOperateManager::instance()->getCurrentVG()) {
        reset.append(vg.name);
    }

    if (reset.contains(vgName)) {
        return true;
    }

    return false;
}

bool PartitionSeniorManager::checkPartition(QString &log)
{
    bool efiFlag = false;
    bool rootFlag = false;
    bool persistentFlag = false;
    bool bootFlag = false;
    qint64 rootSize = 0;
    Q_UNUSED(persistentFlag);
    Q_UNUSED(bootFlag);
    auto reset = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    for (SDeviceInfo dev : reset.keys()) {
        QList<SPartitionInfo> partList = reset.value(dev);
        for (SPartitionInfo par : partList) {
            if (par.usage == ESpaceUsage::USE && par.isEFIPartition && (!dev.isStartDevice)) {
                efiFlag = true;
            }

            // 检查是否有创建根分区
            if (par.usage == ESpaceUsage::USE && par.mountPoint == "/") {
                rootFlag = true;
                rootSize = par.size;
            }

            if (par.usage == ESpaceUsage::USE && par.mountPoint == "/persistent") {
                persistentFlag = true;
            }

            if (par.usage == ESpaceUsage::USE && par.mountPoint == "/boot") {
                bootFlag = true;
            }
        }
    }

    auto vgInfos = PartitionOperateManager::instance()->getCurrentLVMInfo();
    for (SVGInfo vg : vgInfos.keys()) {
        for(SLVMInfo& lv : vgInfos[vg]) {
            // 检查lvm中是否有根分区
            if (lv.usage == ESpaceUsage::USE && lv.mountPoint == "/") {
                rootFlag = true;
                rootSize = lv.size;
            }

            if (lv.usage == ESpaceUsage::USE && lv.mountPoint == "/persistent") {
                persistentFlag = true;
            }

            if (lv.usage == ESpaceUsage::USE && lv.mountPoint == "/boot") {
                bootFlag = true;
            }
        }
    }

    if (CommonFunc::isUEFI() && !efiFlag) {
       log = ::QObject::tr("Please create a %1 partition").arg("/boot/efi");
       return false;
    }

    //if (!bootFlag) {
    //    log = ::QObject::tr("Please create a %1 partition").arg("/boot");
    //    return false;
    //}

    if (!rootFlag) {
        log = ::QObject::tr("Please create a %1 partition").arg("/");
        return false;
    }

    //if (!persistentFlag && CommonFunc::getCurrentOsVersion() == "V25") {
    //    log = ::QObject::tr("Please create a %1 partition").arg("/persistent");
    //    return false;
    //}

    if(m_encryptPart.first && !partitionEncryptIsReady(m_encryptPart.second)) {
        log = ::QObject::tr("please create a boot partition to enable encrypt / partition");
        return false;
    }

    if (rootFlag && !persistentFlag) {
        // 如果没有创建persistent分区,且当前根分区小于根分区最小值加persistent分区最小值和则提示用户空间不够
        auto rootConfigSize = CommonFunc::getSettingsInt("DI_ROOTA_SIZE_CONFIG");
        auto persistentConfigSize = CommonFunc::getSettingsInt("DI_PERSISTENT_SIZE_CONFIG");
        if (rootSize < rootConfigSize + persistentConfigSize) {
            log = ::QObject::tr("The partition size is less than the minimum disk space requirement (%1) for system installation.")
                                     .arg(QString("%1GiB").arg(QString::number((rootConfigSize / giDeviceUnitSize) + (persistentConfigSize / giDeviceUnitSize))));
            return false;
        }
    }

    return true;
}

bool PartitionSeniorManager::checkGhostPartition(QString &err)
{
    return PartitionOperateManager::instance()->checkGhostSpace(err);
}

SPartitionInfo PartitionSeniorManager::getEFIPartition()
{
    SPartitionInfo efiPartition;
    auto infos = getSystemPartitionInfo();
    for (SDeviceInfo dev : infos.keys()) {
        if (dev.isStartDevice || dev.isVentoyDevice) {
            continue;
        }
        for (SPartitionInfo in : infos.value(dev)) {
            if (in.usage == ESpaceUsage::USE) {
                if (in.isEFIPartition || in.mountPoint == "/boot/efi") {
                    efiPartition = in;
                }
            }
        }
    }
    return efiPartition;
}

SPartitionInfo PartitionSeniorManager::getBootPartition()
{
    SPartitionInfo bootPartition;
    auto infos = getSystemPartitionInfo();
    for (SDeviceInfo dev : infos.keys()) {
        for (SPartitionInfo in : infos.value(dev)) {
            if (in.usage == ESpaceUsage::USE) {
                if (in.mountPoint == "/boot") {
                    bootPartition = in;
                }
            }
        }
    }
    return bootPartition;
}

bool PartitionSeniorManager::existSWAPPartition(bool fromCurrentDevInfo)
{
    auto infos = fromCurrentDevInfo ? PartitionOperateManager::instance()->getCurrentDeviceInfo() : PartitionOperateManager::instance()->getAvailableDeviceInfo();
    for (SDeviceInfo dev : infos.keys()) {
        for (SPartitionInfo in : infos.value(dev)) {
            if (in.usage == ESpaceUsage::USE && gSwapFileSystems.contains(in.filesystem)) {
                return true;
            }
        }
    }
    //补充lvm情况
    auto lvms = (fromCurrentDevInfo ? PartitionOperateManager::instance()->getCurrentLVMInfo() : PartitionOperateManager::instance()->getAvailableLVMInfo());
    for (auto &lvmList: lvms.values()) {
        for (auto &lvm: lvmList) {
            if(lvm.usage == ESpaceUsage::USE && gSwapFileSystems.contains(lvm.filesystem)) {
                return true;
            }
        }
    }

    return false;
}

//辅助查找函数
bool PartitionSeniorManager::findUsagePartition(const QString &value, int &index, SPartitionInfo &partition, FindCond cond) {
    auto currDevice = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    for (auto dev : currDevice) {
        auto iter = dev.begin();
        for (int useIndex = 0; iter != dev.end(); iter++) {
            if (iter->usage == ESpaceUsage::USE) {
                QString compareStr;
                switch (cond) {
                case FindCond::ID:
                    compareStr = iter->id;
                    break;
                case FindCond::MOUNTPOINT:
                    compareStr = iter->mountPoint;
                    break;
                }

                if(compareStr == value) {
                    partition = *iter;
                    index = useIndex;
                    return true;
                }

                useIndex++;
            }
        }
    }

    return false;
}

bool PartitionSeniorManager::findFirstESP(SPartitionInfo &bootloader)
{
    SPartitionInfo espPartition;
    int espPartitionIndex = -1;
    //UEFI方式安装正常情况不关心引导分区是不是第一个分区
    //但是龙芯平台必须引导分区为第一个分区，可以配置龙芯平台的DI_FIRST_IS_BOOTLOADER_CONFIG配置
    //强制要求启动分区为第一个分区
    bool bEfiFirstPartition = CommonFunc::getSettingsBool("DI_FIRST_IS_BOOTLOADER_CONFIG");
    if (CommonFunc::isUEFI()) {
        QMap<SDeviceInfo, QList<SPartitionInfo>> currDevice = PartitionOperateManager::instance()->getCurrentDeviceInfo();
        // 在当前操作界面的设备列表中查找EFI分区
        for (const SDeviceInfo &dev : currDevice.keys()) {
            if (dev.isStartDevice) {
                continue;
            }
            QList<SPartitionInfo> devPartList = currDevice.value(dev);
            QList<SPartitionInfo>::iterator iter = devPartList.begin();
            for (int useIndex = 0; iter != devPartList.end(); ++iter) {
                if (iter->isEFIPartition && (iter->usage == ESpaceUsage::USE)) {
                        espPartition = *iter;
                        espPartitionIndex = useIndex;
                        break;
                } else if (iter->usage == ESpaceUsage::USE) {
                    useIndex++;
                }
            }
        }
    } else {
        SPartitionInfo bootPartition;
        int bootIndex = -1;
        findUsagePartition("/boot", bootIndex, bootPartition);
        SPartitionInfo rootPartition;
        int rootIndex = -1;
        findUsagePartition("/", rootIndex, rootPartition);
        if (!bootPartition.id.isEmpty()) {
            espPartition = bootPartition;
            espPartitionIndex = bootIndex;
        } else {
            espPartition = rootPartition;
            espPartitionIndex = rootIndex;
        }
    }
    bootloader = espPartition;
    if (bEfiFirstPartition) {
        //判断引导分区是否为第一个
        return espPartitionIndex == 0;
    } else {
        //判断是否存在引导分区
        return espPartitionIndex != -1;
    }
}

bool PartitionSeniorManager::isNewPrimaryPartition(const SDeviceInfo &dev) {
    if (CommonFunc::isUEFI()) {
        return true;
    }

    int partCnt = 0;
    auto reset = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    for (SPartitionInfo part : reset.value(dev)) {
        if (part.usage == ESpaceUsage::USE && (part.partitionType == EPartitionType::PartitionTypePrimary
                                               || part.partitionType == EPartitionType::PartitionTypeExtension)) {
            ++partCnt;
        }
    }

    const int mbrPartMaxCnt = 4;
    if (partCnt >= mbrPartMaxCnt) {
        return false;
    }

    return true;
}

bool PartitionSeniorManager::isNewPrimaryPartition(const SDeviceInfo &dev, const SPartitionInfo &free)
{
    // 判断当前空闲分区是否在扩展分区的内部
    auto exten = findExtensionPartition(PartitionOperateManager::instance()->getSystemPartitions(dev));
    if (!exten.id.isEmpty() && CommonFunc::isPartContains(exten, free)) {
        return false;
    }

    return isNewPrimaryPartition(dev);
}

bool PartitionSeniorManager::isNewLogicalPartition(const SDeviceInfo &dev, const SPartitionInfo &free)
{
    if (CommonFunc::isUEFI()) {
        return false;
    }

    SPartitionInfo extensionPart;
    auto reset = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    for (SPartitionInfo part : reset.value(dev)) {
        if (part.usage == ESpaceUsage::USE && part.partitionType == EPartitionType::PartitionTypeExtension) {
            extensionPart = part;
            break;
        }
    }

    if (extensionPart.id.isEmpty()) {
        // 如果需要创建扩展分区的时候，判断是否能新建主分区
        return isNewPrimaryPartition(dev);
    }

    // 扩展分区内部的空闲分区只能创建成逻辑分区
    if (CommonFunc::isPartContains(extensionPart, free)) {
        return true;
    }

    //qInfo() << "extensionPart: " << extensionPart.toString().toStdString().c_str();
    //qInfo() << "free: " << free.toString().toStdString().c_str();

    return CommonFunc::isPartContinuous(extensionPart, free);
}

bool PartitionSeniorManager::isNewStartLogicalPartition(const SDeviceInfo &dev, const SPartitionInfo &free)
{
    SPartitionInfo extenPart;
    auto reset = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    for (SPartitionInfo part : reset.value(dev)) {
        if (part.usage == ESpaceUsage::USE && part.partitionType == EPartitionType::PartitionTypeExtension) {
            extenPart = part;
        }
    }

    if (extenPart.id.isEmpty()) {
        return false;
    }

    return free.startPoint < extenPart.startPoint && CommonFunc::isPartContinuous(extenPart, free);
}

bool PartitionSeniorManager::isNewEndLogicalPartition(const SDeviceInfo &dev, const SPartitionInfo &free)
{
    SPartitionInfo extenPart;
    auto reset = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    for (int i = reset.value(dev).size() - 1; i >= 0; i--) {
        SPartitionInfo part = reset.value(dev).at(i);
        if (part.usage == ESpaceUsage::USE && part.partitionType == EPartitionType::PartitionTypeExtension) {
            extenPart = part;
        }
    }

    if (extenPart.id.isEmpty()) {
        return false;
    }

    return free.startPoint > extenPart.startPoint && CommonFunc::isPartContinuous(extenPart, free);
}

bool PartitionSeniorManager::isFormatDisk(const SDeviceInfo &devName)
{
    SDeviceInfo dev = findDev(devName);
    bool guideTypeEQ = CommonFunc::isUEFI() ? (dev.type == EDisklabelType::GPT) : (dev.type == EDisklabelType::MsDos);
    return !guideTypeEQ;
}

void PartitionSeniorManager::setFormatDisk(const SDeviceInfo &devName)
{
    // 删除磁盘下分区合成的vg
    //qInfo() << "setFormatDisk start...";
    QList<SVGInfo> vgs = getDevMergeVg(devName);
    for (SVGInfo v : vgs) {
        delVG(v);
    }

    // 删除磁盘下分区
    QMap<SDeviceInfo, QList<SPartitionInfo> > devInfos = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    SDeviceInfo dev = findDev(devName);
    for (SPartitionInfo in : devInfos.value(dev)) {
        if (in.usage == ESpaceUsage::USE && in.partitionType != EPartitionType::PartitionTypeExtension) {
            delPartition(dev, in);
        }
    }

    PartitionOperateManager::instance()->setFormatDisk(dev);
    //qInfo() << "setFormatDisk end!";
}

QList<SVGInfo> PartitionSeniorManager::getDevMergeVg(const SDeviceInfo &devName)
{
    QList<SVGInfo> reset;
    QList<SPartitionInfo> part = PartitionOperateManager::instance()->getCurrentDeviceInfo().value(findDev(devName.device));

    QList<SVGInfo> vgs = PartitionOperateManager::instance()->getCurrentVG();
    for (SVGInfo v : vgs) {
        for (SPartitionInfo p : part) {
            if (v.pvIds.contains(p.id)) {
                if (!reset.contains(v)) {
                    reset.append(v);
                }
            }
        }
    }

    return reset;
}

bool PartitionSeniorManager::isMultipleDiskToVg(const SDeviceInfo &devName, QMap<QString, QString> &text)
{
    return PartitionOperateManager::instance()->isMultipleDiskToVg(devName, text);
}

bool PartitionSeniorManager::isHaveCreatedSwap(QString &swapPartitionId)
{
    auto reset = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    for (SDeviceInfo dev : reset.keys()) {
        QList<SPartitionInfo> partList = reset.value(dev);
        for (SPartitionInfo par : partList) {
            // 检查是否有创建swap分区
            if (gSwapFileSystems.contains(par.filesystem)) {
                swapPartitionId = par.id;
                return true;
            }
        }
    }

    auto vgInfos = PartitionOperateManager::instance()->getCurrentLVMInfo();
    for (SVGInfo vg : vgInfos.keys()) {
        for(SLVMInfo& lv : vgInfos[vg]) {
            // 检查是否有创建swap分区
            if (gSwapFileSystems.contains(lv.filesystem)) {
                swapPartitionId = lv.id;
                return true;
            }
        }
    }

    return  false;
}

bool PartitionSeniorManager::checkNewRootPartitionIsBootloader(const SPartitionInfo &info)
{
    if (info.mountPoint != "/") {
        return false;
    }

    SPartitionInfo bootloder = info;
    if (!findFirstESP(bootloder)) {
        return true;
    }

    if (bootloder.mountPoint == "/") {
        return true;
    }

    return false;
}

bool PartitionSeniorManager::checkDialogNextEnable(const SPartitionInfo &info, QString &err)
{
    err = QString();
    const QString splitChar = "、";
    QStringList bootFileSystem = CommonFunc::getSettingsStringList("DI_BOOT_PARTITION_FS_CONFIG");

    // 文件系统的检查
    QString onlyEmptyMountPointTip = QString(::QObject::tr("When the file system is %1, the mount point can only be %2."));
    if (gSwapFileSystems.contains(info.filesystem) && CommonFunc::getEmptyMountPoint() != CommonFunc::getPartitionMountText(info.mountPoint)) {
        err = onlyEmptyMountPointTip.arg(info.filesystem, CommonFunc::getEmptyMountPoint());
        return false;
    }

    if (gLVM2PVFileSystem == info.filesystem && CommonFunc::getEmptyMountPoint() != CommonFunc::getPartitionMountText(info.mountPoint)) {
        err = onlyEmptyMountPointTip.arg(info.filesystem, CommonFunc::getEmptyMountPoint());
        return false;
    }

    QStringList efiSystem = CommonFunc::getSettingsStringList("DI_EFI_PARTITION_FS_CONFIG");
    if (efiSystem.contains(info.filesystem) && "/boot/efi" != info.mountPoint) {
        err = onlyEmptyMountPointTip.arg(info.filesystem, "/boot/efi");
        return false;
    }

    QString onlyBootEfiMountPointTip = QString(::QObject::tr("When the mount point is %1, the file system can only be %2."));
    if ("/boot/efi" == info.mountPoint && !efiSystem.contains(info.filesystem)) {
        err = onlyBootEfiMountPointTip.arg("/boot/efi", efiSystem.join(splitChar));
        return false;
    }

    // 引导分区的检查
    QString loaderTip = QString(::QObject::tr("The %1 is a boot partition, and its file system can only be %2"));
    if (!CommonFunc::isUEFI() && checkNewRootPartitionIsBootloader(info) && !bootFileSystem.contains(info.filesystem)) {
        err = loaderTip.arg(CommonFunc::getPartitionMountText(info.mountPoint), bootFileSystem.join(splitChar));
        return false;
    }

    QString mountTip = ::QObject::tr("When the mount point is %1, the file system can only be %2.");
    if ("/boot" == info.mountPoint && !bootFileSystem.contains(info.filesystem)) {
       err = mountTip.arg(info.mountPoint, bootFileSystem.join(splitChar));
       return false;
    }

    // 分区大小检查
    QString sizeTip = ::QObject::tr("The size of %1 partition should be more than %2");
    qint64 minSize = 0;
    double showSize = 0.0;
    if ("/" == info.mountPoint) {
        minSize = CommonFunc::getSettingsInt("DI_ROOTA_SIZE_CONFIG");
    } else if ("/persistent" == info.mountPoint) {
        minSize = CommonFunc::getSettingsInt("DI_PERSISTENT_SIZE_CONFIG");
    } else if ("/boot" == info.mountPoint) {
        minSize = CommonFunc::getSettingsInt("DI_BOOT_SIZE_CONFIG");
    } else if ("/boot/efi" == info.mountPoint) {
        minSize = CommonFunc::getSettingsInt("DI_EFI_SIZE_CONFIG");
    }  else if (gSwapFileSystems.contains(info.filesystem)) {
        minSize = CommonFunc::getSwapSize();
    } else {
        minSize = info.mountPoint.isEmpty() ? 1 : CommonFunc::getSettingsInt("DI_OTHER_SIZE_CONFIG");
    }

    showSize = minSize;
    QString showUnit = CommonFunc::showSizeUnit(showSize);
    if(info.size < minSize && qAbs(minSize - info.size) > gPartitionDelta) {
        err = sizeTip.arg(info.mountPoint, QString("%1%2").arg(QString::number(showSize), showUnit));
        return false;
    }

    return true;
}

bool PartitionSeniorManager::checkDialogNextEnable(const SLVMInfo &info, QString &err)
{
   SPartitionInfo partInfo = SPartitionInfo{};
   partInfo.id = info.id;
   partInfo.filesystem = info.filesystem;
   partInfo.mountPoint = info.mountPoint;
   partInfo.size = info.size;
   return checkDialogNextEnable(partInfo, err);
}

SPartitionInfo PartitionSeniorManager::mergePartitions(const SPartitionInfo &fir, const SPartitionInfo &sec)
{
    SPartitionInfo free;
    if ((fir.usage == ESpaceUsage::FREE) && (sec.usage == ESpaceUsage::FREE)) {
        free = fir;
        free.id = QUuid::createUuid().toString();
        free.usage = ESpaceUsage::FREE;
        free.partitionType = EPartitionType::PartitionTypePrimary;
        if (fir.startPoint > sec.startPoint) {
            free.startPoint = sec.startPoint;
        }
        if (fir.endPoint < sec.endPoint) {
            free.endPoint = sec.endPoint;
        }
        free.size += sec.size;
        free.label = "";
        free.mountPoint = "";
        free.filesystem = "";
        free.index = -1;
        free.isCrypt = false;
        free.cryptPassword = "";
    }

    return free;
}

bool PartitionSeniorManager::existLogicalPartition(const QList<SPartitionInfo> &partLists)
{
    for (SPartitionInfo part : partLists) {
        if (part.usage == ESpaceUsage::USE && part.partitionType == EPartitionType::PartitionTypeLogical) {
            return true;
        }
    }

    return false;
}

SPartitionInfo PartitionSeniorManager::expandExtensionPartition(const SDeviceInfo &dev, SPartitionInfo &part)
{
    SPartitionInfo reset = findExtensionPartition(PartitionOperateManager::instance()->getSystemPartitions(dev));

    if (!reset.id.isEmpty()) {
        if (part.endPoint > reset.endPoint) {
            // 从扩展分区的尾部开始增加逻辑分区
            reset.endPoint = part.endPoint;
        } else if (part.endPoint < reset.startPoint){
            // 从扩展分区的头部开始增加逻辑分区
            reset.startPoint = part.startPoint;
            part.startPoint += 1;
        }

        reset.byteSize = (reset.endPoint + 1 - reset.startPoint) * reset.sectorSize;
        reset.size = CommonFunc::bytesToM(reset.byteSize);
    }

    return reset;
}

SPartitionInfo PartitionSeniorManager::addExtensionPartition(const SDeviceInfo &dev, SPartitionInfo &part)
{
    SPartitionInfo reset = part;
    reset.id = QUuid::createUuid().toString();
    reset.usage = ESpaceUsage::USE;
    reset.mountPoint = "";
    reset.filesystem = "";
    reset.partitionType = EPartitionType::PartitionTypeExtension;
    reset.label = CommonFunc::getPartitionLabel(reset);
    reset.device = dev.device;
    reset.deviceSize = dev.size;
    reset.sectorSize = dev.sectors;
    reset.phySectorSize = dev.physectors;
    reset.startPoint = part.startPoint;
    reset.endPoint = part.endPoint;
    reset.byteSize = part.byteSize;
    reset.size = part.size;
    reset.isCrypt = false;
    reset.cryptPassword = "";

    part.startPoint += 1;
    return reset;
}

SPartitionInfo PartitionSeniorManager::findExtensionPartition(const QList<SPartitionInfo> &parts)
{
    SPartitionInfo reset;
    for (SPartitionInfo part : parts) {
        if (part.usage == ESpaceUsage::USE && part.partitionType == EPartitionType::PartitionTypeExtension) {
            reset = part;
        }
    }

    return reset;
}

QList<SPartitionInfo> PartitionSeniorManager::getOperatorPartitions(const SDeviceInfo &dev)
{
    QList<SPartitionInfo> reset;
    auto devLists = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    for (SPartitionInfo part : devLists.value(dev)) {
        // 前端操作的分区列表中需要过滤掉扩展分区
        if (part.usage == ESpaceUsage::USE && part.partitionType == EPartitionType::PartitionTypeExtension) {
            continue;
        }

        reset.append(part);
    }

    return reset;
}

bool PartitionSeniorManager::isStartLogicalPartition(const SDeviceInfo &dev, const SPartitionInfo &in)
{
    SPartitionInfo logicalPart;
    auto reset = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    for (SPartitionInfo part : reset.value(dev)) {
        if (part.usage == ESpaceUsage::USE && part.partitionType == EPartitionType::PartitionTypeLogical) {
            logicalPart = part;
            break;
        }
    }

    if (logicalPart.id.isEmpty()) {
        return true;
    }

    return in == logicalPart;
}

bool PartitionSeniorManager::isEndLogicalPartition(const SDeviceInfo &dev, const SPartitionInfo &in)
{
    SPartitionInfo logicalPart;
    auto reset = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    for (int i = reset.value(dev).size() - 1; i >= 0; i--) {
        SPartitionInfo part = reset.value(dev).at(i);
        if (part.usage == ESpaceUsage::USE && part.partitionType == EPartitionType::PartitionTypeLogical) {
                logicalPart = part;
                break;
        }
    }

    if (logicalPart.id.isEmpty()) {
        return true;
    }

    return in == logicalPart;
}

qint64 PartitionSeniorManager::getExtensionPartitonsSize(const QList<SPartitionInfo> &infos)
{
    qint64 reset = 0;
    for (SPartitionInfo in : infos) {
        if (in.partitionType == EPartitionType::PartitionTypeLogical) {
            reset += in.size;
        }
    }

    return reset;
}

SLVMInfo PartitionSeniorManager::getFreeLVMInfo(const SVGInfo &vg)
{
    auto devInfos = PartitionOperateManager::instance()->getCurrentLVMInfo().value(vg);
    for (SLVMInfo lv : devInfos) {
        if (lv.usage == ESpaceUsage::FREE) {
            return lv;
        }
    }

    return SLVMInfo();
}

SPartitionInfo PartitionSeniorManager::getFreePartInfo(const SDeviceInfo &dev, qint64 size)
{
    auto devinfos = PartitionOperateManager::instance()->getCurrentDeviceInfo().value(dev);
    for(auto& part : devinfos) {
        if (part.usage == ESpaceUsage::FREE && part.size >= size) {
            return part;
        }
    }

    return SPartitionInfo();
}

QString PartitionSeniorManager::getVGName()
{
    QStringList reset;
    for (auto vg : PartitionOperateManager::instance()->getCurrentVG()) {
        reset.append(vg.name);
    }

    QString vgName = "vg";
    QString str = "vg";

    do {
        vgName = QString("%1%2").arg(str, QString::number(QDateTime::currentSecsSinceEpoch()));
    } while (reset.contains(vgName));

    return vgName;
}

void PartitionSeniorManager::resetMountPoint(const QString &mountPoint)
{
    auto devinfos = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    for (SDeviceInfo dev : devinfos.keys()) {
        for(SPartitionInfo& part : devinfos[dev]) {
            if (part.mountPoint == mountPoint) {
                part.mountPoint = "";
                if ((part.label != gRootbLabel) && (part.usage != ESpaceUsage::FREE) && (part.partitionType != EPartitionType::PartitionTypeExtension)) {
                    part.label = CommonFunc::getPartitionLabel(part);
                }
            }
        }

        PartitionOperateManager::instance()->updateDeviceInfo(dev, devinfos[dev]);
    }

    auto vgInfos = PartitionOperateManager::instance()->getCurrentLVMInfo();
    for (SVGInfo vg : vgInfos.keys()) {
        for(SLVMInfo& lv : vgInfos[vg]) {
            if (lv.mountPoint == mountPoint) {
                lv.mountPoint = "";
                if ((lv.label != gRootbLabel) && (lv.usage != ESpaceUsage::FREE)) {
                    lv.label = CommonFunc::getPartitionLabel(lv);
                }
            }
        }

        PartitionOperateManager::instance()->updateLVMInfo(vg, vgInfos[vg]);
    }
}

bool PartitionSeniorManager::isPartitionCanEncrypt(const SPartitionInfo &partInfo)
{
    static const QStringList fsEnableList = {"ext3", "ext4"};
    if(partInfo.mountPoint == "/boot/efi" ||
        partInfo.mountPoint == "/boot" ||
        partInfo.mountPoint == "/" ||
        !fsEnableList.contains(partInfo.filesystem)) {
        return false;
    }

    return true;
}

bool PartitionSeniorManager::isPartitionCanEncrypt(const SLVMInfo &lvmInfo)
{
    Q_UNUSED(lvmInfo);
    return false;
}

bool PartitionSeniorManager::getCurrentEncryptStatus(const QString &partId)
{
    return m_encryptPart.first ? partId == m_encryptPart.second : true;
}

SDeviceInfo PartitionSeniorManager::findDev(const QString &devName)
{
    auto devInfos = PartitionOperateManager::instance()->getCurrentDevice();
    for (SDeviceInfo in : devInfos) {
        if (in.device == devName) {
            return in;
        }
    }

    return SDeviceInfo();
}

SDeviceInfo PartitionSeniorManager::findDev(const SDeviceInfo &dev)
{
    auto devInfos = PartitionOperateManager::instance()->getCurrentDevice();
    for (SDeviceInfo in : devInfos) {
        if (in.device == dev.device) {
            return in;
        }
    }

    return SDeviceInfo();
}


SDeviceInfo PartitionSeniorManager::findAvailableDev(const QString &devName)
{
    auto devInfos = PartitionOperateManager::instance()->getAvailableDevice();
    for (SDeviceInfo in : devInfos) {
        if (in.device == devName) {
            return in;
        }
    }

    return SDeviceInfo();
}

SPartitionInfo PartitionSeniorManager::find(const SDeviceInfo &dev, const SPartitionInfo &part)
{
    auto devInfos = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    for (SPartitionInfo in : devInfos.value(dev)) {
        if (in.id == part.id) {
            return in;
        }
    }

    return SPartitionInfo();
}

SPartitionInfo PartitionSeniorManager::find(const QString &id)
{
    auto devInfos = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    for (SDeviceInfo dev : devInfos.keys()) {
        for (SPartitionInfo in : devInfos.value(dev)) {
            if (in.id == id) {
                return in;
            }
        }
    }

    return SPartitionInfo();
}

SPartitionInfo PartitionSeniorManager::findAvailablePartition(const QString &id)
{
    auto devInfos = PartitionOperateManager::instance()->getAvailableDeviceInfo();
    for (SDeviceInfo dev : devInfos.keys()) {
        for (SPartitionInfo in : devInfos.value(dev)) {
            if (in.id == id) {
                return in;
            }
        }
    }

    return SPartitionInfo();
}

SVGInfo PartitionSeniorManager::findVG(const SVGInfo &vgName)
{
    auto devInfos = PartitionOperateManager::instance()->getCurrentVG();
    for (SVGInfo vg : devInfos) {
        if (vg.id == vgName.id) {
            return vg;
        }
    }

    return SVGInfo();
}

SVGInfo PartitionSeniorManager::findVG(const QString &vgName)
{
    auto devInfos = PartitionOperateManager::instance()->getCurrentVG();
    for (SVGInfo vg : devInfos) {
        if (vg.name == vgName) {
            return vg;
        }
    }

    return SVGInfo();
}

SVGInfo PartitionSeniorManager::findLvToVG(const QString &lvID)
{
    auto devInfos = PartitionOperateManager::instance()->getCurrentLVMInfo();
    for (SVGInfo vg : devInfos.keys()) {
        for (SLVMInfo lv : devInfos.value(vg)) {
            if (lv.id == lvID) {
                return vg;
            }
        }
    }

    return SVGInfo();
}

SLVMInfo PartitionSeniorManager::findLV(const QString &id)
{
    auto devInfos = PartitionOperateManager::instance()->getCurrentLVMInfo();
    for (SVGInfo vg : devInfos.keys()) {
        for (SLVMInfo lv : getSystemLVMInfo().value(vg)) {
            if (lv.id == id) {
                return lv;
            }
        }
    }

    return SLVMInfo();
}

void PartitionSeniorManager::updateEncryptStatus(const QString &partId, const QString &pwd, bool bAutoDecrypt, bool bEncrypt)
{
    //qInfo() << "update device crypt info begin:";
    auto devInfos = getSystemPartitionInfo();
    auto lvmInfos = getSystemLVMInfo();
    QString encryptPwd = bEncrypt ? pwd : "";

    for (auto &dev: devInfos.keys()) {
        for (auto &partInfo: devInfos[dev]) {
            if(partInfo.id == partId) {
                partInfo.isCrypt = bEncrypt;
                partInfo.cryptPassword = encryptPwd;
            } else if (isInCryptList(partInfo.id)) {
                partInfo.isCanCrypt = !bEncrypt;
            }
        }

        PartitionOperateManager::instance()->updateDeviceInfo(dev, devInfos[dev]);
    }

    for (auto &vg: lvmInfos.keys()) {
        for (auto &lvm: lvmInfos[vg]) {
            if(lvm.id == partId) {
                lvm.isCrypt = bEncrypt;
                lvm.cryptPassword = encryptPwd;
            } else if (isInCryptList(lvm.id)) {
                lvm.isCanCrypt = !bEncrypt;
            }
        }

        PartitionOperateManager::instance()->updateLVMInfo(vg, lvmInfos[vg]);
    }

    CommonFunc::setSettingsValue("DI_IS_AUTO_DECRYPT", bEncrypt ? bAutoDecrypt : false);
    CommonFunc::setSettingsValue("DI_CRYPT_PASSWORD", encryptPwd);
    m_encryptPart = qMakePair(bEncrypt, partId);
    //qInfo() << "update device crypt info end";
}

bool PartitionSeniorManager::isInCryptList(const QString &partId)
{
    return m_cryptCandidateList.contains(partId);
}

void PartitionSeniorManager::clearCryptList()
{
    m_encryptPart.first = false;
    m_encryptPart.second.clear();
    m_cryptCandidateList.clear();
}

quint64 PartitionSeniorManager::getMinGhostPartitionSize(const QString &partmt)
{
    return PartitionOperateManager::instance()->getGhostPartitionSize(partmt);
}
