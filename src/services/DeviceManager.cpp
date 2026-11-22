#include "DeviceManager.h"
#include "CommonFunc.h"
#include "globals.h"
#include "MutexObject.h"
#include "ThreadObject.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QThread>
#include <QTimer>
#include <QFuture>
#include <QtConcurrent>
#include <QReadLocker>
#include <QWriteLocker>

DeviceManager::DeviceManager()
{

}

DeviceManager *DeviceManager::m_instance = nullptr;
DeviceManager *DeviceManager::instance()
{
    if (m_instance == nullptr) {
        m_instance = new DeviceManager;
    }
    return m_instance;
}

void DeviceManager::scanDevices(bool start)
{
    QString err;
    m_deviceUpdated = false;
    QFuture<void> res = QtConcurrent::run([this, &err]() {
        this->scanDeviceInfos(err);
    });
}

void DeviceManager::scanUSBDevices(bool start)
{
    QString err;
    m_enableScanUSBDevice = start;
    
    QFuture<void> res = QtConcurrent::run([this, &err]() {
        this->scanUSBDeviceInfos(err);
    });

}

QMap<SDeviceInfo, QList<SPartitionInfo>> DeviceManager::getDeviceInfos()
{
    while (!m_deviceUpdated) {
        QThread::msleep(100);
    }
    
    QReadLocker locker(&m_lock);
    return m_devicesInfos;
}

QMap<SVGInfo, QList<SLVMInfo>> DeviceManager::getLVMInfos()
{
    QReadLocker locker(&m_lock);
    return m_lvmInfos;
}


QList<SDeviceInfo> DeviceManager::getUSBDeviceInfos()
{
    QReadLocker locker(&m_lock);
    return m_usbDeviceInfos;
}

void DeviceManager::quitUSBScan()
{
    m_enableScanUSBDevice = false;
}

bool DeviceManager::getDeviceInfoFromJson(QJsonObject &deviceinfoobj, QString &err)
{
    QString deviceinfostring = "";
    if (!CommonFunc::command("/bin/bash", {"-c", "lsblk -JOb"}, deviceinfostring)) {
        err = QString("Command: lsblk -JOb cmd error failed. %1").arg(deviceinfostring);
        return false;
    }

    QJsonParseError jsonerror;
    deviceinfoobj = QJsonDocument::fromJson(deviceinfostring.toStdString().data(), &jsonerror).object();
    if (jsonerror.error != QJsonParseError::NoError) {
        err = QString("read device info json failed. %1").arg(jsonerror.errorString());
        return false;
    }

    return true;
}

bool DeviceManager::getVGnamesInfo(QMap<QString, SVGInfo> &vgNames, QString &err)
{
    QString vgInfoString = "";
    QString cmd = "vgdisplay | grep \"VG Name\" | awk -F \" \" '{print $3}'";
    if (!CommonFunc::command("/bin/bash", {"-c", cmd}, vgInfoString)) {
        err = QString("Command: %1 failed. %2").arg(cmd).arg(vgInfoString);
        return false;
    }
    QStringList vgInfoList = CommonFunc::qstringSplitToQStringList(vgInfoString, "\n");

    for (QString vgInfoItem : vgInfoList) {
        cmd = QString("pvdisplay -C | grep %1 | awk -F \" \" '{print $1}'").arg(vgInfoItem);
        QString pvInfoString = "";
        if (!CommonFunc::command("/bin/bash", {"-c", cmd}, pvInfoString)) {
            err = QString("Command: %1 failed. %2").arg(cmd).arg(pvInfoString);
            return false;
        }
        vgNames[vgInfoItem].name = vgInfoItem;
        vgNames[vgInfoItem].isCryptHeader = vgInfoItem.contains("usec-vg");
        QStringList pvInfoList = CommonFunc::qstringSplitToQStringList(pvInfoString, "\n");
        for (QString pvInfoItem : pvInfoList) {
            vgNames[vgInfoItem].pvIds.append(pvInfoItem);
        }
    }
    return true;
}

void DeviceManager::updatePartInfoByVG(QMap<QString, SVGInfo> &vgNames, SPartitionInfo &partitionInfo)
{
    // 更新分区信息，判断该分区是否在vg中
    QStringList vgNamesStr = vgNames.keys();
    if (std::find_if(vgNamesStr.begin(), vgNamesStr.end(), [&](const QString &vgNameStrItem) {
        return vgNames[vgNameStrItem].pvIds.contains(partitionInfo.path);
    }) != vgNamesStr.end()) {
        partitionInfo.isMergeVG = true;
    } else {
        qInfo() << "part: " << partitionInfo.path << " not merge in vg, part str is: "<< partitionInfo.toString();
    }

    if (std::find_if(vgNamesStr.begin(), vgNamesStr.end(), [&](const QString &vgNameStrItem) {
        return vgNames[vgNameStrItem].pvIds.contains(partitionInfo.path) && vgNameStrItem.contains("usec-vg-");
    }) != vgNamesStr.end()) {
        partitionInfo.isCryptHeader = true;
    } else {
        qInfo() << "part: " << partitionInfo.path << " is not crypt header, part str is: "<< partitionInfo.toString();
    }
}

bool DeviceManager::updatePartInfosByFdisk(SDeviceInfo &deviceinfo, QList<SPartitionInfo> &partitioninfos, QString &err)
{
    QString devicepartinfostring = "";
    QString cmd = QString("fdisk -l -o Device,Start,End,Type | grep %1").arg(deviceinfo.device);
    if (!CommonFunc::command("/bin/bash", {"-c", cmd}, devicepartinfostring)) {
        err = QString("Command: %1 failed. %2").arg(cmd).arg(devicepartinfostring);
        return false;
    }
    QStringList partinfostringlist = CommonFunc::qstringSplitToQStringList(devicepartinfostring, "\n");

    // 更新SPartitionInfo列表中的信息
    for (int i = 0; i < partitioninfos.size(); i++) {
        for (QString partinfoitem : partinfostringlist) {
            if (partinfoitem.contains(partitioninfos.at(i).path)) {
                QStringList partinfolistitem = CommonFunc::qstringSplitToQStringList(partinfoitem, " ");
                partinfoitem = partinfolistitem.join(" ");

                int pathIndex = partinfoitem.indexOf(" ");
                int startPointIndex = partinfoitem.indexOf(" ", pathIndex + 1);
                if (startPointIndex != -1) {
                    bool toULongLongOK = false;
                    QString startPointStr = partinfoitem.left(startPointIndex);
                    startPointStr = startPointStr.right(startPointStr.length() - pathIndex - 1);
                    partitioninfos[i].startPoint = startPointStr.toULongLong(&toULongLongOK);
                    if (!toULongLongOK) {
                        qWarning() << QString("set startPoint failed: %1 to toULongLong failed.").arg(startPointStr);
                    }

                    int endPointIndex = partinfoitem.indexOf(" ", startPointIndex + startPointStr.length() + 1);
                    if (endPointIndex != -1) {
                        QString endPointStr = partinfoitem.left(endPointIndex);
                        endPointStr = endPointStr.right(endPointStr.length() - startPointIndex - 1);
                        partitioninfos[i].endPoint = endPointStr.toULongLong(&toULongLongOK);
                        if (!toULongLongOK) {
                            qWarning() << QString("set endPoint failed: %1 to toULongLong failed.").arg(endPointStr);
                        }
                    }
                }

                // 设置是否是EFI分区
                if (partinfoitem.contains("EFI")) {
                    partitioninfos[i].isEFIPartition = true;
                }

                break;
            }
        }
    }

    return true;
}

bool DeviceManager::updatePartType(const SDeviceInfo &deviceinfo, QList<SPartitionInfo> &partitioninfos, QString &err)
{
    QString devicepartinfostring = "";
    QString cmd = QString("parted %1 print | grep -E \"primary|extended|logical\"").arg(deviceinfo.device);
    if (!CommonFunc::command("/bin/bash", {"-c", cmd}, devicepartinfostring)) {
        err = QString("Command: %1 failed. %2").arg(cmd).arg(devicepartinfostring);
        return false;
    }

    QStringList partinfostringlist = CommonFunc::qstringSplitToQStringList(devicepartinfostring, "\n");

    // 将解析的分区type信息设置到SPartitionInfo列表中
    for (int i = 0; i < partitioninfos.size(); i++) {
        for (QString partinfoitem : partinfostringlist) {
            QStringList partinfolistitem = CommonFunc::qstringSplitToQStringList(partinfoitem, " ");
            partinfoitem = partinfolistitem.join(" ");

            int pathIndex = partinfoitem.indexOf(" ");
            QString partIndexStr = partinfoitem.left(pathIndex);
            bool toIntOK = false;
            int partIndex = partIndexStr.toInt(&toIntOK);
            if (toIntOK && (partIndex == partitioninfos.at(i).index) &&
                (partitioninfos.at(i).usage != ESpaceUsage::FREE)) {
                if (partinfoitem.contains(gPrimary)) {
                    partitioninfos[i].partitionType = EPartitionType::PartitionTypePrimary;
                } else if (partinfoitem.contains(gExtended)) {
                    partitioninfos[i].partitionType = EPartitionType::PartitionTypeExtension;
                    // 计算扩展分区的大小
                    qint64 onemsectors = CommonFunc::mToSectors(1, deviceinfo.sectors);
                    qint64 subsectors = partitioninfos.at(i).endPoint - partitioninfos.at(i).startPoint - 1;
                    partitioninfos[i].size = subsectors / onemsectors;
                } else if (partinfoitem.contains(gLogical)) {
                    partitioninfos[i].partitionType = EPartitionType::PartitionTypeLogical;
                }
            }
        }
    }

    return true;
}

void DeviceManager::scanDeviceInfos(QString &err)
{
    qInfo() << Q_FUNC_INFO << "scan device begin";
    // 使用lsblk命令获取设备信息的json格式返回
    QJsonObject deviceInfoObj;
    if (!getDeviceInfoFromJson(deviceInfoObj, err)) {
        qWarning() << err;
        m_deviceUpdated = true;
        return;
    }

    QMap<QString, SVGInfo> vgnames;
    QMap<QString, SLVMInfo> lvnames;
    if (!getVGnamesInfo(vgnames, err)) {
        err = "getVGnames failed vgnames is empty";
        qWarning() << err;
        m_deviceUpdated = true;
        return;
    }

    QMap<SDeviceInfo, QList<SPartitionInfo>> devicesInfos;
    QJsonArray deviceArray = deviceInfoObj.value("blockdevices").toArray();
    if (filterDevice(deviceArray) == 0) {
        err = "blockdevices is empty";
        qWarning() << err;
        m_deviceUpdated = true;
        return;
    }

    QString startDev;
    QString startDevPart;
    bool findStartDev = false;
    findStartDevice(deviceArray, startDev, startDevPart, findStartDev);

    qInfo() << "startDev is : " << startDev << " startDevPart is : " << startDevPart;
    // 保存启动盘设备的路径
    if (findStartDev) {
        CommonFunc::setSettingsValue("DI_START_DEV", startDev);
    } else {
        CommonFunc::setSettingsValue("DI_START_DEV", "");
    }

    for (int i = 0; i < deviceArray.size(); i++) {
        QJsonObject deviceObj = deviceArray.at(i).toObject();
        //过滤掉只读设备
        if (deviceObj.value("ro").toBool()) {
            continue;
        }

        SDeviceInfo deviceInfo = newDeviceByJobj(deviceObj);
        // 标记启动盘设备
        if ((!startDev.isEmpty()) && (!deviceInfo.device.compare(startDev))) {
            deviceInfo.isStartDevice = true;
        }

        QList<SPartitionInfo> partinfoitems;
        QJsonArray devicePartArray = deviceObj.value("children").toArray();
        for (int j = 0; j < devicePartArray.size(); j++) {
            QJsonObject devicePartObj = devicePartArray.at(j).toObject();
            SPartitionInfo partInfoItem = newPartitionByJobj(deviceInfo, devicePartObj);

            if (partInfoItem.label == "Ventoy") {
                deviceInfo.isVentoyDevice = true;
            }

            // 标记启动盘设备分区
            if ((!startDevPart.isEmpty()) && (!partInfoItem.path.compare(startDevPart))) {
                partInfoItem.isStartDevicePart = true;
            }

            // 存在加密分区则当前设备是加密设备
            if (partInfoItem.isCrypt) {
                deviceInfo.isCrypt = partInfoItem.isCrypt;
                deviceInfo.decryptStatus = partInfoItem.decryptStatus;
                deviceInfo.cryptPartPath = partInfoItem.isCrypt ? partInfoItem.path : "";
            }

            // 更新vgnames和lvnames
            updateVGNames(devicePartObj, vgnames, lvnames);
            updatePartInfoByVG(vgnames, partInfoItem);

            partinfoitems.append(partInfoItem);
        }

        // 使用fdisk命令更新每个设备的分区信息
        if (!updatePartInfosByFdisk(deviceInfo, partinfoitems, err)) {
            qWarning() << err;
            continue;
        }

        // 使用parted命令获取分区的type信息
        if (!updatePartType(deviceInfo, partinfoitems, err)) {
            qWarning() << err;
            continue;
        }

        // 获取空闲空间的信息
        std::sort(partinfoitems.begin(), partinfoitems.end());
        updateFreeSpace(deviceInfo, partinfoitems);

        // 过滤掉小于1M分区
        // removePartLessThanOneM(partinfoitems);

        devicesInfos.insert(deviceInfo, partinfoitems);
    }

    // 根据vgnames和lvnames更新m_lvmInfos
    scanLvmInfos(devicesInfos, vgnames, lvnames);

#ifdef QT_DEBUG
    printDevInfos(devicesInfos, m_lvmInfos);
#endif // DEBUG

    QWriteLocker locker(&m_lock);
    m_devicesInfos = devicesInfos;
    m_deviceUpdated = true;
    qInfo() << Q_FUNC_INFO << "end";
}

void DeviceManager::updateUSBDeviceInfo(QList<SDeviceInfo> &deviceInfo)
{
    QWriteLocker locker(&m_lock);
    bool isChange = false;
    if (m_usbDeviceInfos.size() == deviceInfo.size()) {
        for (SDeviceInfo devItem : deviceInfo) {
            int infoIndex = m_usbDeviceInfos.indexOf(devItem);
            if (infoIndex == -1) {
                isChange = true;
                break;
            } else {
                if (m_usbDeviceInfos.at(infoIndex).mountPoint.compare(devItem.mountPoint)) {
                    isChange = true;
                    break;
                }
            }
        }
    } else {
        isChange = true;
    }

    if (isChange) {
        m_usbDeviceInfos = deviceInfo;

        if (m_enableScanUSBDevice) {
            Q_EMIT usbDeviceChanged("");
        }
    }
}

void DeviceManager::scanUSBDeviceInfos(QString &err)
{
    while (m_enableScanUSBDevice) {
        QList<SDeviceInfo> usbDeviceInfo;

        // 使用lsblk命令获取设备信息的json格式返回
        QJsonObject deviceInfoObj;
        if (!getDeviceInfoFromJson(deviceInfoObj, err)) {
            return;
        }

        QMap<SDeviceInfo, QList<SPartitionInfo>> devicesInfos;
        QJsonArray deviceArray = deviceInfoObj.value("blockdevices").toArray();
        if (DeviceManager::instance()->filterDevice(deviceArray) == 0) {
            err = "blockdevices is empty";
            return;
        }

        QString startDev;
        QString startDevPart;
        bool findStartDev = false;
        DeviceManager::instance()->findStartDevice(deviceArray, startDev, startDevPart, findStartDev);
        qInfo() << "startDev is : " << startDev << " startDevPart is : " << startDevPart;

        for (int i = 0; i < deviceArray.size(); i++) {
            QJsonObject deviceObj = deviceArray.at(i).toObject();
            //过滤掉只读设备
            if (deviceObj.value("ro").toBool()) {
                continue;
            }

            // 排除非usb设备
            if (deviceObj.value("tran").toString().compare("usb")) {
                continue;
            }

            if (!deviceObj.value("fstype").toString().isEmpty()) {
                // 设备根有文件系统则作为一个存储设备处理
                // 过滤掉挂载位置不可写的设备
                QString devPath = deviceObj.value("path").toString();
                QString devMountPoint = deviceObj.value("mountpoint").toString();
                if ((!devMountPoint.isEmpty()) && (!CommonFunc::mountPointWriteable(devMountPoint))) {
                    continue;
                }

                SDeviceInfo deviceInfo;
                deviceInfo.name       = deviceObj.value("label").toString();
                deviceInfo.device     = devPath;
                deviceInfo.filesystem = deviceObj.value("fstype").toString();
                deviceInfo.mountPoint = devMountPoint;
                deviceInfo.byteSize   = deviceObj.value("size").toVariant().toLongLong();
                deviceInfo.size       = CommonFunc::bytesToM(deviceInfo.byteSize);
                deviceInfo.isStartDevice = (startDev == devPath);

                usbDeviceInfo.append(deviceInfo);
            }

            QJsonArray devicePartArray = deviceObj.value("children").toArray();
            for (int j = 0; j < devicePartArray.size(); j++) {
                QJsonObject devicePartObj = devicePartArray.at(j).toObject();

                QString devPartPath = devicePartObj.value("path").toString();
                // 过滤掉挂载位置不可写且不是启动盘的设备
                QString devicePartMountPoint = devicePartObj.value("mountpoint").toString();
                if ((!devicePartMountPoint.isEmpty()) && (!CommonFunc::mountPointWriteable(devicePartMountPoint)) && (startDevPart != devPartPath)) {
                    continue;
                }

                if (!devicePartObj.value("fstype").toString().isEmpty()) {
                    // 分区有文件系统则作为一个存储设备处理
                    SDeviceInfo deviceInfo;
                    deviceInfo.name       = devicePartObj.value("label").toString();
                    deviceInfo.device     = devPartPath;
                    deviceInfo.filesystem = devicePartObj.value("fstype").toString();
                    deviceInfo.mountPoint = devicePartMountPoint;
                    deviceInfo.byteSize   = devicePartObj.value("size").toVariant().toLongLong();
                    deviceInfo.size       = CommonFunc::bytesToM(deviceInfo.byteSize);
                    deviceInfo.isStartDevice = (startDevPart == devPartPath);

                    usbDeviceInfo.append(deviceInfo);
                }
            }
        }

        usbDeviceInfo = mountUSBDevices(usbDeviceInfo);
        updateUSBDeviceInfo(usbDeviceInfo);
        QThread::msleep(1000);
    }
}

void DeviceManager::scanLvmInfos(QMap<SDeviceInfo, QList<SPartitionInfo>> devs, QMap<QString, SVGInfo> &vgnames, QMap<QString, SLVMInfo> &lvnames)
{
    QWriteLocker locker(&m_lock);
    m_lvmInfos.clear();
    // 更新每个vg下的lvm分区信息，和空闲空间信息
    for (QString vgname : vgnames.keys()) {
        qint64 lvTotalByteSize = 0;
        vgnames[vgname].id = QUuid::createUuid().toString();
        // 更新vg的pvids信息
        QMap<SDeviceInfo, QList<SPartitionInfo>>::iterator deviceiter;
        for (deviceiter = devs.begin(); deviceiter != devs.end(); ++deviceiter) {
            for (SPartitionInfo partinfo : deviceiter.value()) {
                if (vgnames[vgname].pvIds.indexOf(partinfo.path) != -1) {
                    vgnames[vgname].pvIds.removeOne(partinfo.path);
                    vgnames[vgname].pvIds.append(partinfo.id);
                }
            }
        }

        // 找到vg对应的lvm分区信息
        for (QString lvname : lvnames.keys()) {
            if (lvname.contains(vgname)) {
                lvnames[lvname].id     = QUuid::createUuid().toString();
                lvnames[lvname].vgName = vgname;
                lvnames[lvname].isCryptHeader = vgname.contains("usec-vg");
                lvTotalByteSize += lvnames[lvname].byteSize;
                m_lvmInfos[vgnames[vgname]].append(lvnames[lvname]);
            }
        }

        // 获取vg中的pe的size，将其过滤掉不作为空闲空间使用
        qint64 peSize = 1;
        QString err;
        if (!CommonFunc::getLVMPESize(vgname, peSize, err)) {
            peSize = 1;
        }

        // 添加vg空闲空间信息, 空间大小以M为单位
        qint64 vgFreeByteSize = vgnames[vgname].byteSize - lvTotalByteSize;
        qint64 vgFreeSize = CommonFunc::bytesToM(vgFreeByteSize);
        if (vgFreeSize > peSize) {
            SLVMInfo freespaceinfo;
            freespaceinfo.id         = QUuid::createUuid().toString();
            freespaceinfo.size       = vgFreeSize;
            freespaceinfo.byteSize   = vgFreeByteSize;
            freespaceinfo.label      = "";
            freespaceinfo.usage      = ESpaceUsage::FREE;
            freespaceinfo.vgName     = vgname;
            freespaceinfo.filesystem = "";
            freespaceinfo.mountPoint = "";
            m_lvmInfos[vgnames[vgname]].append(freespaceinfo);
        }
    }
}

void DeviceManager::updateVGNames(const QJsonObject &partinfoobj, QMap<QString, SVGInfo> &vgnames, QMap<QString, SLVMInfo> &lvnames)
{
    QJsonObject partInfoObjNew;
    if (!partinfoobj.value("fstype").toString().compare("crypto_LUKS")) {
        QJsonArray cryptPartInfoArray = partinfoobj.value("children").toArray();
        if (cryptPartInfoArray.size() > 0) {
            partInfoObjNew = cryptPartInfoArray.at(0).toObject();
        } else {
            return;
        }
    } else {
        partInfoObjNew = partinfoobj;
    }

    if (partInfoObjNew.value("fstype").toString().compare("LVM2_member")) {
        return;
    }

    QList<QString> lvmNamesByLvDisplay;
    for (QString vgname : vgnames.keys()) {
        if (vgnames[vgname].pvIds.indexOf(partInfoObjNew.value("path").toString()) != -1) {
            vgnames[vgname].byteSize += partInfoObjNew.value("size").toVariant().toLongLong();
            vgnames[vgname].size = CommonFunc::bytesToM(vgnames[vgname].byteSize);
        }

        QString cmd = QString("lvdisplay -C | grep %1 | awk '{print $1}'").arg(vgname);
        QString err = "";
        QStringList resultList = {};
        if (CommonFunc::getCmdListReturn(cmd, resultList, err)) {
            if (resultList.size() > 0) {
                lvmNamesByLvDisplay.append(resultList);
            }
        }
    }

    // 添加lvm分区信息到lvnames
    QJsonArray lvmpartarray = partInfoObjNew.value("children").toArray();
    for (int h = 0; h < lvmpartarray.size(); h++) {
        QJsonObject lvmpartobj = lvmpartarray.at(h).toObject();
        QString lvmpath = lvmpartobj.value("name").toString();
        QString lvmName = lvmpath;
        lvmpath.replace("--", "-");
        auto findItem = std::find_if(lvmNamesByLvDisplay.begin(), lvmNamesByLvDisplay.end(), [=](const QString &name) {
            return lvmpath.contains(name);
        });
        if (findItem != lvmNamesByLvDisplay.end()) {
            lvmName = *findItem;
        }

        lvnames[lvmpath].name       = lvmpath.contains("usec-vg") ? lvmName : lvmpath.mid(lvmpath.lastIndexOf('-') + 1);
        lvnames[lvmpath].label      = lvmpartobj.value("label").toString();
        lvnames[lvmpath].filesystem = lvmpartobj.value("fstype").toString();
        lvnames[lvmpath].mountPoint = lvmpartobj.value("mountpoint").toString();
        lvnames[lvmpath].path       = lvmpartobj.value("path").toString();
        lvnames[lvmpath].byteSize   = lvmpartobj.value("size").toVariant().toLongLong();
        lvnames[lvmpath].size       = CommonFunc::bytesToM(lvnames[lvmpath].byteSize);
        lvnames[lvmpath].usage      = ESpaceUsage::USE;
    }
}

void DeviceManager::updateFreeSpace(SDeviceInfo &deviceinfo, QList<SPartitionInfo> &partitioninfos)
{
    qint64 totaldevicesectors = deviceinfo.byteSize / deviceinfo.sectors;
    qint64 subsectors = 0;
    qint64 tmpstartpoint = 0;                // 起始位置

    // 处理磁盘起点到最后一个分区之间的空闲分区
    for (int i = 0; i < partitioninfos.size(); i++) {
        qint64 tmpendpoint = partitioninfos.at(i).startPoint - 1;
        subsectors = tmpendpoint - tmpstartpoint - 1;
        if (subsectors >= 1) {
            SPartitionInfo partinfoitem = newFreePartition(deviceinfo, subsectors * deviceinfo.sectors, tmpstartpoint, tmpendpoint);
            partitioninfos.insert(i, partinfoitem);
        }
        tmpstartpoint = partitioninfos.at(i).endPoint + 1;
    }

    // 处理最后一个分区之后的空闲分区
    subsectors = totaldevicesectors - tmpstartpoint;
    if (subsectors >= 1) {
        SPartitionInfo partinfoitem = newFreePartition(deviceinfo, subsectors * deviceinfo.sectors, tmpstartpoint, totaldevicesectors);
        partitioninfos.append(partinfoitem);
    }

    // 处理扩展分区中的空闲空间大小，单位bytes
    qint64 extensionPartByteSize = 0;
    qint64 extensionPartStartPoint = 0;   // 扩展分区的起始位置
    qint64 lastLogicalPartEndPoint = 0;   // 最后一个逻辑分区的终点位置
    int extensionPartIndex = 0; //记录扩展分区在列表中的索引
    for (SPartitionInfo info : partitioninfos) {
        if (ESpaceUsage::USE == info.usage) {
            if (EPartitionType::PartitionTypeLogical == info.partitionType) {
                lastLogicalPartEndPoint = info.endPoint + 1;
                extensionPartIndex = partitioninfos.indexOf(info);
            } else if (EPartitionType::PartitionTypeExtension == info.partitionType) {
                extensionPartByteSize = info.byteSize;
                extensionPartStartPoint = info.startPoint;   // 扩展分区的起始位置
                extensionPartIndex = partitioninfos.indexOf(info);
            }
        }
    }

    if (extensionPartByteSize > 0) {
        // 计算扩展分区总扇区数
        qint64 extensionPartTotalSectors = extensionPartByteSize / deviceinfo.sectors;
        // 计算扩展分区中空闲空间的起点
        tmpstartpoint = (lastLogicalPartEndPoint > 0) ? (lastLogicalPartEndPoint + 1) : (extensionPartStartPoint + 1);

        // 计算扩展分区中空闲空间总扇区数
        subsectors = totaldevicesectors - tmpstartpoint;
        // 过滤掉小于1M的空闲空间
        if (subsectors >= 1) {
            // 创建一个空闲空间分区
            SPartitionInfo partinfoitem = newFreePartition(deviceinfo, subsectors * deviceinfo.sectors, tmpstartpoint, extensionPartTotalSectors);
            partinfoitem.partitionType = EPartitionType::PartitionTypeLogical;

            //将扩展分区的空闲加入分区列表
            int extensionPartNextIndex = extensionPartIndex + 1;
            if (extensionPartIndex < partitioninfos.size()) {
                partitioninfos.insert(extensionPartNextIndex, partinfoitem);
            } else {
                partitioninfos.append(partinfoitem);
            }
            qWarning() << "extensionFreeSize is : " << partinfoitem.size;
        }
    }

    // 合并所有相邻的空闲空间
    QList<SPartitionInfo> newPartitionInfos;
    SPartitionInfo freePartInfo = SPartitionInfo();
    freePartInfo.id = "";
    for (SPartitionInfo info : partitioninfos) {
        if (info.usage == ESpaceUsage::FREE) {
            if (freePartInfo.id.isEmpty()) {
                freePartInfo = info;
            } else {
                freePartInfo.endPoint = info.endPoint;
                qint64 oldFreePartSectors = freePartInfo.endPoint + 1 - freePartInfo.startPoint - 1;
                freePartInfo.byteSize = oldFreePartSectors * deviceinfo.sectors;
                freePartInfo.size = CommonFunc::bytesToM(freePartInfo.byteSize);
            }
        } else {
            if ((!freePartInfo.id.isEmpty()) && (info.partitionType != EPartitionType::PartitionTypeExtension)) {
                newPartitionInfos.append(freePartInfo);
                freePartInfo = SPartitionInfo();
                freePartInfo.id = "";
            }
            newPartitionInfos.append(info);
        }
    }
    if (!freePartInfo.id.isEmpty()) {
        newPartitionInfos.append(freePartInfo);
        freePartInfo = SPartitionInfo();
    }

    //根据引导模式调整第一个空闲空间的起点位置和最后一个空闲空间的终点位置
    // efi引导模式下空磁盘末尾需要预留33个扇区的空间存放gpt分区表信息
    if (CommonFunc::isUEFI()) {
        if (newPartitionInfos.at(0).usage == ESpaceUsage::FREE) {
            newPartitionInfos[0].startPoint = 34;

            // 调整后的起点大于终点，则舍弃这个分区，调整第二个分区的起点，这时一定存在第二个分区
            if (newPartitionInfos.at(0).startPoint >= newPartitionInfos.at(0).endPoint) {
                newPartitionInfos.removeAt(0);
                newPartitionInfos[0].startPoint = 34;
            }

            // 更新分区的大小信息
            newPartitionInfos[0].byteSize = (newPartitionInfos[0].endPoint + 1 - newPartitionInfos[0].startPoint) * deviceinfo.sectors;
            newPartitionInfos[0].size = CommonFunc::bytesToM(newPartitionInfos[0].byteSize);
        }
        int lastPartIndex = newPartitionInfos.size() - 1;
        if (newPartitionInfos.at(lastPartIndex).usage == ESpaceUsage::FREE) {
            newPartitionInfos[lastPartIndex].endPoint = totaldevicesectors - 34;

            // 调整后的终点小于起点，则舍弃这个分区，调整前个分区的终点，这时一定存在前一个分区
            if (newPartitionInfos.at(lastPartIndex).endPoint <= newPartitionInfos.at(lastPartIndex).startPoint) {
                newPartitionInfos.removeAt(lastPartIndex);
                lastPartIndex = newPartitionInfos.size() - 1;
                newPartitionInfos[lastPartIndex].endPoint = totaldevicesectors - 34;
            }

            // 更新分区的大小信息
            newPartitionInfos[lastPartIndex].byteSize = (newPartitionInfos[lastPartIndex].endPoint + 1 - newPartitionInfos[lastPartIndex].startPoint) * deviceinfo.sectors;
            newPartitionInfos[lastPartIndex].size = CommonFunc::bytesToM(newPartitionInfos[lastPartIndex].byteSize);
        }
    } else {
        // lagecy引导模式下空磁盘末尾需要预留1个扇区的空间存放gpt分区表信息
        if (newPartitionInfos.at(0).usage == ESpaceUsage::FREE) {
            newPartitionInfos[0].startPoint = 3;
            // 更新分区的大小信息
            newPartitionInfos[0].byteSize = (newPartitionInfos[0].endPoint + 1 - newPartitionInfos[0].startPoint) * deviceinfo.sectors;
            newPartitionInfos[0].size = CommonFunc::bytesToM(newPartitionInfos[0].byteSize);
        }

        int lastPartIndex = newPartitionInfos.size() - 1;
        if (newPartitionInfos.at(lastPartIndex).usage == ESpaceUsage::FREE) {
            newPartitionInfos[lastPartIndex].endPoint = totaldevicesectors - 3;
            // 更新分区的大小信息
            newPartitionInfos[lastPartIndex].byteSize = (newPartitionInfos[lastPartIndex].endPoint + 1 - newPartitionInfos[lastPartIndex].startPoint) * deviceinfo.sectors;
            newPartitionInfos[lastPartIndex].size = CommonFunc::bytesToM(newPartitionInfos[lastPartIndex].byteSize);
        }
    }

    partitioninfos.clear();
    partitioninfos = newPartitionInfos;

    //判断分区大小是free类型就累加给设备信息的freesize字段
    for (SPartitionInfo info : partitioninfos) {
        if (ESpaceUsage::FREE == info.usage) {
            deviceinfo.freeByteSize += info.byteSize;
            deviceinfo.freesize = CommonFunc::bytesToM(deviceinfo.freeByteSize);
        }
    }
}

SDeviceInfo DeviceManager::newDeviceByJobj(const QJsonObject &deviceinfoobj)
{
    SDeviceInfo deviceinfo;
    deviceinfo.ptuuid        = deviceinfoobj.value("ptuuid").toString();
    deviceinfo.name          = deviceinfoobj.value("model").toString(); //暂时使用model填充，设备名称可以从device中获取到
    deviceinfo.device        = deviceinfoobj.value("path").toString();
    deviceinfo.sectors       = deviceinfoobj.value("log-sec").toVariant().toLongLong();
    deviceinfo.physectors    = deviceinfoobj.value("phy-sec").toVariant().toLongLong();
    deviceinfo.filesystem    = deviceinfoobj.value("fstype").toString();
    deviceinfo.mountPoint    = deviceinfoobj.value("mountpoint").toString();
    deviceinfo.transporttype = deviceinfoobj.value("tran").toString();
    deviceinfo.rota          = deviceinfoobj.value("rota").toBool();
    deviceinfo.type          = CommonFunc::qstringToEnumDisklabel(deviceinfoobj.value("pttype").toString());
    deviceinfo.byteSize      = deviceinfoobj.value("size").toVariant().toLongLong();
    deviceinfo.size          = CommonFunc::bytesToM(deviceinfo.byteSize);
    if (!deviceinfoobj.value("fsavail").toString().isEmpty()) {
        deviceinfo.availSize = CommonFunc::bytesToM(deviceinfoobj.value("fsavail").toVariant().toLongLong());
    } else {
        deviceinfo.availSize = 0;
    }
    deviceinfo.freesize      = 0;
    return deviceinfo;
}

SPartitionInfo DeviceManager::newPartitionByJobj(const SDeviceInfo &deviceinfo, const QJsonObject &partitioninfoobj)
{
    SPartitionInfo partinfoitem;
    partinfoitem.byteSize      = partitioninfoobj.value("size").toVariant().toLongLong();
    partinfoitem.size          = CommonFunc::bytesToM(partinfoitem.byteSize);
    partinfoitem.id            = QUuid::createUuid().toString();
    partinfoitem.partUuid      = partitioninfoobj.value("partuuid").toString();
    partinfoitem.label         = partitioninfoobj.value("label").toString();
    partinfoitem.usage         = ESpaceUsage::USE;
    partinfoitem.partitionType = EPartitionType::PartitionTypePrimary;
    partinfoitem.device        = deviceinfo.device;
    partinfoitem.index         = partitioninfoobj.value("path").toString().replace(deviceinfo.device, "").replace(QRegularExpression("[A-Za-z]"),"").toInt();
    partinfoitem.path          = partitioninfoobj.value("path").toString();
    partinfoitem.filesystem    = partitioninfoobj.value("fstype").toString();
    partinfoitem.mountPoint    = partitioninfoobj.value("mountpoint").toString();
    partinfoitem.startPoint    = 0;
    partinfoitem.endPoint      = 0;
    partinfoitem.sectorSize    = partitioninfoobj.value("log-sec").toVariant().toLongLong();
    partinfoitem.phySectorSize = partitioninfoobj.value("phy-sec").toVariant().toLongLong();
    partinfoitem.isStartPoint  = true;
    partinfoitem.deviceSize    = deviceinfo.size;
    partinfoitem.isEFIPartition = (partinfoitem.filesystem == "vfat" && partinfoitem.label == "EFI");// 文件系统是vfat 并且label是EFI的情况下，是efi分区
    // lsblk -O includes PARTTYPE. Detect an existing BIOS boot partition by
    // its GPT type, independently of filesystem labels or partition names.
    partinfoitem.isBiosBoot = deviceinfo.type == EDisklabelType::GPT
            && partitioninfoobj.value("parttype").toString().compare(
                "21686148-6449-6e6f-744e-656564454649", Qt::CaseInsensitive) == 0;
    if (partinfoitem.isBiosBoot && partinfoitem.label.isEmpty())
        partinfoitem.label = "BIOS-BOOT";
    partinfoitem.isMergeVG      = false;
    partinfoitem.isCrypt        = partinfoitem.filesystem == "crypto_LUKS" ? true : false;
    partinfoitem.decryptStatus  = (partinfoitem.isCrypt && partitioninfoobj.value("children").toArray().size() > 0) ? true : false;
    if (!partitioninfoobj.value("fsavail").toString().isEmpty()) {
        partinfoitem.availSize = CommonFunc::bytesToM(partitioninfoobj.value("fsavail").toVariant().toLongLong());
    } else {
        partinfoitem.availSize = 0;
    }
    return partinfoitem;
}

SPartitionInfo DeviceManager::newFreePartition(const SDeviceInfo &deviceinfo, qint64 byteSize, qint64 startpoint, qint64 endpoint)
{
    SPartitionInfo partinfoitem;
    partinfoitem.byteSize      = byteSize;
    partinfoitem.size          = CommonFunc::bytesToM(byteSize);
    partinfoitem.id            = QUuid::createUuid().toString();
    partinfoitem.label         = "";
    partinfoitem.usage         = ESpaceUsage::FREE;
    partinfoitem.partitionType = EPartitionType::PartitionTypePrimary;
    partinfoitem.device        = deviceinfo.device;
    partinfoitem.path          = "";
    partinfoitem.filesystem    = "";
    partinfoitem.mountPoint    = "";
    partinfoitem.startPoint    = startpoint;
    partinfoitem.endPoint      = endpoint;
    partinfoitem.sectorSize    = deviceinfo.sectors;
    partinfoitem.phySectorSize = deviceinfo.physectors;
    partinfoitem.index         = -1;
    partinfoitem.isStartPoint  = true;
    partinfoitem.deviceSize    = deviceinfo.size;
    partinfoitem.isEFIPartition = false;
    partinfoitem.isMergeVG      = false;
    return partinfoitem;
}

void DeviceManager::removePartLessThanOneM(QList<SPartitionInfo> &partitioninfos)
{
    QList<SPartitionInfo>::iterator partinfoiter = partitioninfos.begin();
    for (;partinfoiter != partitioninfos.end();) {
        if (partinfoiter->size <= 1) {
            partinfoiter = partitioninfos.erase(partinfoiter);
        } else {
            ++partinfoiter;
        }
    }
}

int DeviceManager::filterDevice(QJsonArray &devicearray)
{
    QString types = CommonFunc::getSettingsString("DI_DEVICETYPES_CONFIG");
    QStringList devicetypes = CommonFunc::qstringSplitToQStringList(types, ";");
    QJsonArray::iterator iter = devicearray.begin();
    for (;iter != devicearray.end();) {
        QJsonObject deviceobj = iter->toObject();
        qint64 devicesize = CommonFunc::bytesToM(deviceobj.value("size").toVariant().toLongLong());
        QString devicetype = deviceobj.value("type").toString();

        if ((!devicetypes.contains(devicetype))// 过滤掉不在期望类型中的设备
            || (devicesize <= 1)) {// 过滤掉于1M的设备
            iter = devicearray.erase(iter);
            continue;
        } else {
            ++iter;
        }
    }

    return devicearray.size();
}

QList<SDeviceInfo> DeviceManager::mountUSBDevices(const QList<SDeviceInfo> &infoList)
{
    QList<SDeviceInfo> newInfoList;
    QString cmd = "";
    QString err = "";
    for (SDeviceInfo deviceItem : infoList) {
        SDeviceInfo newDeviceItem = deviceItem;
        // 如果是启动盘设备，需要mount --bind到其他目录，如果是ventoy启动盘，需要dmsetup新建设备节点，再mount
        if (newDeviceItem.isStartDevice) {
            // 检测启动盘是否存在可读写的挂载点
            cmd = QString("lsblk %1 -no MOUNTPOINTS").arg(newDeviceItem.device);
            QStringList mountPointList;
            if (!CommonFunc::getCmdListReturn(cmd, mountPointList, err)) {
                qWarning() << err;
                continue;
            }

            bool isMountPointWriteable = false;
            for (QString  pathItem : mountPointList) {
                if (CommonFunc::mountPointWriteable(pathItem)) {
                    isMountPointWriteable = true;
                    newDeviceItem.mountPoint = pathItem;
                    break;
                }
            }

            if (!isMountPointWriteable) {
                if (newDeviceItem.name == "Ventoy") {
                    // 获取ventoy启动盘的扇区数
                    cmd = QString("fdisk -l | grep %1 | awk '{print $5}'").arg(newDeviceItem.device);
                    QString result = "";
                    if (!CommonFunc::command("/bin/bash", {"-c", cmd}, result)) {
                        qWarning() << result;
                        continue;
                    }

                    result.replace("\n", "");
                    result.replace(" ", "");

                    // 构建dm映射表文件
                    QFile tabFile("/tmp/VentoyPartMount");
                    if (tabFile.open(QIODevice::ReadWrite)) {
                        QString fileInfoStr = QString("0 %1 linear %2 0").arg(result).arg(newDeviceItem.device);
                        qWarning() << "fileInfoStr is : " << fileInfoStr << fileInfoStr.toStdString().c_str();
                        tabFile.write(fileInfoStr.toStdString().c_str());
                        tabFile.close();
                    }

                    // 创建dm设备节点
                    cmd = "dmsetup create Ventoy1 /tmp/VentoyPartMount";
                    if (!CommonFunc::command("/bin/bash", {"-c", cmd}, err)) {
                        qWarning() << err;
                        continue;
                    }

                    if (!mountPath("/dev/mapper/Ventoy1", "/media/Ventoy1", err)) {
                        qWarning() << err;
                        continue;
                    }

                    newDeviceItem.mountPoint = "/media/Ventoy1";
                } else {
                    QString startDevMountPoint = "/media" + newDeviceItem.device;
                    //  挂载前先创建文件路径
                    if (!QDir().exists(startDevMountPoint)) {
                        if (!QDir().mkpath(startDevMountPoint)) {
                            qWarning() << QString("mkpath %1 failed").arg(startDevMountPoint);
                            continue;
                        }
                    } else {
                        qInfo() << QString("mountPoint %1 is exists not need to create !").arg(startDevMountPoint);
                    }

                    cmd = QString("mount --bind %1 %2").arg(newDeviceItem.mountPoint, startDevMountPoint);
                    if (!CommonFunc::command("/bin/bash", {"-c", cmd}, err)) {
                        qWarning() << err;
                        continue;
                    }
                    newDeviceItem.mountPoint = startDevMountPoint;
                }
            }
        }

        // 处理没有挂载的路径
        if (newDeviceItem.mountPoint.isEmpty()) {
            newDeviceItem.mountPoint = "/media" + newDeviceItem.device;
            if (!mountPath(newDeviceItem.device, newDeviceItem.mountPoint, err)) {
                qWarning() << err;
                continue;
            }
        }

        // 检查是否有可用空间
        newDeviceItem.availSize = CommonFunc::bytesToM(CommonFunc::availSizeFromMountPoint(newDeviceItem.mountPoint, err));
        if (newDeviceItem.availSize == 0) {
            qWarning() << err;
            if ((!newDeviceItem.isStartDevice) && (!CommonFunc::umount(newDeviceItem.mountPoint, err))) {
                qWarning() << err;
            }
            continue;
        }

        newInfoList.append(newDeviceItem);
    }
    return newInfoList;
}

// 挂载一个路径
bool DeviceManager::mountPath(const QString &devPath, const QString &mountPoint, QString &err)
{
    if (CommonFunc::isMounted(devPath, err)) {
        return true;
    }

    return CommonFunc::mount(devPath, mountPoint, "", err);
}

void DeviceManager::findStartDevice(const QJsonArray &devicearray, QString &startDev, QString &startDevPart, bool &findStart)
{
    for (int i = 0; i < devicearray.size(); i++) {
        QJsonObject deviceobj = devicearray.at(i).toObject();
        QStringList mountPointList;
        QString mountPointStr = deviceobj.value("mountpoint").toString();
        mountPointList.append(mountPointStr);
        QJsonArray mountPointArray = deviceobj.value("mountpoints").toArray();
        for (const QJsonValue& mountPointItem : mountPointArray) {
            QString mountPointItemStr = mountPointItem.toString();
            if (!mountPointList.contains(mountPointItemStr)) {
                mountPointList.append(mountPointItemStr);
            }
        }
        QString typeStr = deviceobj.value("type").toString();
        QString pathStr = deviceobj.value("path").toString();
        QString liveDirEvn = CommonFunc::getSystemEnv("DI_LIVE_DIR_ENV");
        liveDirEvn = liveDirEvn.left(liveDirEvn.lastIndexOf("/"));
        QString filesystemSquashfsPath = QString("%1/live/filesystem.squashfs").arg(liveDirEvn);
        if (!findStart) {
            findStart = mountPointList.contains(liveDirEvn) ? QFile::exists(filesystemSquashfsPath) : false;
            QJsonArray devicepartarray = deviceobj.value("children").toArray();
            findStartDevice(devicepartarray, startDev, startDevPart, findStart);
        }

        if (findStart && (!typeStr.compare("disk")) && startDev.isEmpty()) {
            startDev = pathStr;
        } else if (findStart && (!typeStr.compare("part")) && startDevPart.isEmpty()) {
            startDevPart = pathStr;
        }
    }
}


void DeviceManager::printDevInfos(QMap<SDeviceInfo, QList<SPartitionInfo>> &devicesInfos, QMap<SVGInfo, QList<SLVMInfo>> &lvmInfos)
{
    for (SDeviceInfo dev : devicesInfos.keys()) {
        qInfo() << "device info:" << dev.toString().toStdString().c_str();
        for (SPartitionInfo in : devicesInfos.value(dev)) {
            qInfo() << "partition info:" << in.toString().toStdString().c_str();
        }
    }

    for (SVGInfo dev : lvmInfos.keys()) {
        qInfo() << "vg info:" << dev.toString().toStdString().c_str();
        for (SLVMInfo in : m_lvmInfos.value(dev)) {
            qInfo() << "lv info:" << in.toString().toStdString().c_str();
        }
    }
}
