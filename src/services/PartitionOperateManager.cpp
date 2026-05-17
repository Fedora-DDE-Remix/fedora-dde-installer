#include "PartitionOperateManager.h"
#include "CommonFunc.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

const quint64 BACKUP_RESERVESIZE = 500 * 1024 * 1024; //500Mib

PartitionOperateManager *PartitionOperateManager::m_instance = Q_NULLPTR;
PartitionOperateManager *PartitionOperateManager::instance()
{
    if(m_instance == nullptr)
        m_instance = new PartitionOperateManager();
    return m_instance;
}

void PartitionOperateManager::setDeviceInfo(const QMap<SDeviceInfo, QList<SPartitionInfo> > &infos)
{
    m_availableDeviceInfo.clear();
    m_currentDeviceInfo.clear();

    for (SDeviceInfo in : infos.keys()) {
        // 过滤启动盘
        if (in.isStartDevice) {
            continue;
        }

        m_availableDeviceInfo[in] = infos.value(in);
        m_currentDeviceInfo[in] = infos.value(in);
    }
}

void PartitionOperateManager::updateDeviceInfo(const SDeviceInfo &dev, const SPartitionInfo &info)
{
    m_currentDeviceInfo[dev].removeAll(info);
    m_currentDeviceInfo[dev].append(info);
    std::sort(m_currentDeviceInfo[dev].begin(), m_currentDeviceInfo[dev].end());
}

void PartitionOperateManager::updateDeviceInfo(const SDeviceInfo &dev, const QList<SPartitionInfo> &infos)
{
    m_currentDeviceInfo.remove(dev);
    m_currentDeviceInfo[dev] = infos;
}

void PartitionOperateManager::updateLVMInfo(const SVGInfo &dev, const QList<SLVMInfo> &infos)
{
    m_currentLVMInfo.remove(dev);
    m_currentLVMInfo[dev] = infos;
}

void PartitionOperateManager::deleteLVMInfo(const SVGInfo &vg)
{
    m_currentLVMInfo.remove(vg);
}

QMap<SDeviceInfo, QList<SPartitionInfo> > PartitionOperateManager::getCurrentDeviceInfo()
{
    return m_currentDeviceInfo;
}

QMap<SVGInfo, QList<SLVMInfo> > PartitionOperateManager::getCurrentLVMInfo()
{
    return m_currentLVMInfo;
}

QMap<SDeviceInfo, QList<SPartitionInfo> > PartitionOperateManager::getAvailableDeviceInfo()
{
    return m_availableDeviceInfo;
}

QMap<SVGInfo, QList<SLVMInfo> > PartitionOperateManager::getAvailableLVMInfo()
{
    return m_availableLVMInfo;
}

void PartitionOperateManager::setLVMInfo(const QMap<SVGInfo, QList<SLVMInfo> > &infos)
{
    m_availableLVMInfo = infos;
    m_currentLVMInfo = infos;

    qInfo() << "set m_availableLVMInfo and m_currentLVMInfo infos:";
    this->printPartitions(infos);
}

void PartitionOperateManager::setPartitionMode(int mode)
{
    CommonFunc::setSettingsValue("DI_PARTITION_TYPE", QString::number(mode));
}

QList<SPartitionInfo> PartitionOperateManager::getSystemPartitions(const SDeviceInfo &in)
{
    return m_currentDeviceInfo.value(in);
}

void PartitionOperateManager::updateDeviceInfo(const SPartitionInfo &info)
{
    for (SDeviceInfo dev : m_currentDeviceInfo.keys()) {
        auto parts = m_currentDeviceInfo.value(dev);
        if (parts.contains(info)) {
            this->updateDeviceInfo(dev, info);
        }
    }
}

QList<SDeviceInfo> PartitionOperateManager::getCurrentDevice()
{
    return m_currentDeviceInfo.keys();
}

QList<SDeviceInfo> PartitionOperateManager::getAvailableDevice()
{
    return m_availableDeviceInfo.keys();
}


QList<SLVMInfo> PartitionOperateManager::getSystemLVMPartitions(const SVGInfo &in)
{
    return m_currentLVMInfo.value(in);
}

void PartitionOperateManager::updateLVMInfo(const SVGInfo &vg, const SLVMInfo &info)
{
    int pos = m_currentLVMInfo.value(vg).indexOf(info);
    if (pos != -1) {
        m_currentLVMInfo[vg][pos] = info;
    }
}

QList<SVGInfo> PartitionOperateManager::getCurrentVG()
{
    return m_currentLVMInfo.keys();
}

bool PartitionOperateManager::checkBackupSize()
{
    qInfo() << "check ghost backup size";
    QList<GhostCheckInfo> ghostCheckList;
    mergePartitionInfo(ghostCheckList);

    QString err;
    quint64 ghostRootSize = getGhostRootSize(ghostCheckList, err);
    if(ghostRootSize == 0) {
        qWarning() << "clone image is error, root size is empty." << err;
        return false;
    }

    //初始化备份需要+/var+/opt+/boot目录的大小
    ghostRootSize += m_uimgDirSize["/var"];
    //ghostRootSize += m_uimgDirSize["/opt"];
    ghostRootSize += m_uimgDirSize["/boot"];

    //check backup size
    auto backupPart = std::find_if(ghostCheckList.begin(), ghostCheckList.end(), [&](const GhostCheckInfo &partInfo){
        return partInfo.mountPoint == "/recovery";});
    if(backupPart == ghostCheckList.end()) {
        qWarning() << "not find backup partition";
        return false;
    }

    quint64 currentBackupSize = backupPart->size * 1024 * 1024;
    qInfo() << "current backup size:" << currentBackupSize << ", ghost root size:" << ghostRootSize;
    if((currentBackupSize - BACKUP_RESERVESIZE) < ghostRootSize) {
        qWarning() << "backup size is not enough:" << currentBackupSize << ghostRootSize;
        return false;
    }

    return true;
}

QString PartitionOperateManager::getPartedOperatorInfo()
{
    this->setOthenOperateInfo();

    QJsonArray jsonArray;

    // 遍历真实VG的信息，获取VG的删除配置
    for (SVGInfo in : m_availableLVMInfo.keys()) {
        delVGConf(in, m_currentLVMInfo.keys(), jsonArray);
    }

    // 获取对磁盘的操作
    for (SDeviceInfo in : m_currentDeviceInfo.keys()) {
        if (in.isFormat) {
            formatDiskConf(in, jsonArray);
        }
    }

    // 遍历真实的磁盘分区信息，获取磁盘删除和编辑的配置
    for (SDeviceInfo in : m_availableDeviceInfo.keys()) {
        QList<SPartitionInfo> availables = m_availableDeviceInfo.value(in);
        QList<SPartitionInfo> currents = m_currentDeviceInfo.value(in);

        // 处理没有格式化的磁盘的删除和编辑
        if (!m_currentDeviceInfo.key(currents).isFormat) {
            // 逻辑分区删除的时候分区号会自动被调整，所以需要在大到小进行删除
            std::sort(availables.begin(), availables.end(),
                  [=](const SPartitionInfo &fir, const SPartitionInfo &sec) {return fir.index > sec.index;});

            for (SPartitionInfo partIn : availables) {
                if (partIn.usage == ESpaceUsage::USE) {
                    delPartConf(partIn, currents, jsonArray);           // 先处理其他分区的删除
                }
            }

            for (SPartitionInfo partIn : availables) {
                if (partIn.usage == ESpaceUsage::USE) {
                    // 屏蔽扩展分区的编辑处理，扩展分区只处理新建
                    if (partIn.partitionType != EPartitionType::PartitionTypeExtension) {
                        editPartConf(partIn, currents, jsonArray);          // 再处理分区的编辑
                    }
                }
            }

            for (SPartitionInfo partIn : availables) {
                if (partIn.usage == ESpaceUsage::USE) {
                    editExtensinoPartConf(partIn, currents, jsonArray);  // 最后处理扩展分区的调整
                }
            }
        }
    }

    // 遍历界面上的磁盘分区信息，获取磁盘上分区新建动作
    for (SDeviceInfo in : m_currentDeviceInfo.keys()) {
        QList<SPartitionInfo> availables = m_availableDeviceInfo.value(in);
        QList<SPartitionInfo> currents = m_currentDeviceInfo.value(in);

        for (SPartitionInfo partIn : currents) {
            if (partIn.usage == ESpaceUsage::USE) {
                newPartConf(partIn, availables, jsonArray);
            }
        }
    }

    // 遍历界面操作的VG信息，获取VG创建配置
    for (SVGInfo in : m_currentLVMInfo.keys()) {
        newVGConf(in, m_availableLVMInfo.keys(), jsonArray);
    }

    // 遍历真实LVM的信息，获取LVM的编辑和删除配置
    for (SVGInfo in : m_availableLVMInfo.keys()) {
        QList<SLVMInfo> availables = m_availableLVMInfo.value(in);
        QList<SLVMInfo> currents = m_currentLVMInfo.value(in);
        for (SLVMInfo lvm : availables) {
            if (lvm.usage == ESpaceUsage::USE) {
                delLVMConf(lvm, currents, jsonArray);
                editLVMConf(lvm, currents, jsonArray);
            }
        }
    }

    // 遍历界面操作的LVM信息，获取LVM创建配置
    for (SVGInfo in : m_currentLVMInfo.keys()) {
        QList<SLVMInfo> availables = m_availableLVMInfo.value(in);
        QList<SLVMInfo> currents = m_currentLVMInfo.value(in);
        for (SLVMInfo lvm : currents) {
            if (lvm.usage == ESpaceUsage::USE) {
                newLVMConf(lvm, availables, jsonArray);
            }
        }
    }

    QStringList deviceList;
    for (const QJsonValue &policyItem : jsonArray) {
        QJsonObject policyObjItem = policyItem.toObject();
        if ((policyObjItem.value("type").toString() == "partition") && (policyObjItem.value("operate").toString() != "normal")) {
            if (!deviceList.contains(policyObjItem.value("device").toString())) {
                deviceList.append(policyObjItem.value("device").toString());
            }
        }
    }
    CommonFunc::setSettingsValue("DI_DEVICE_LIST", deviceList.join(";"));

    m_operateJsonArray = jsonArray;

    return m_operateInfo.join("\n\n");
}

bool PartitionOperateManager::isMultipleDiskToVg(const SDeviceInfo &dev, QMap<QString, QString> &text)
{
    QList<QString> multipleDiskPvs;
    for (SVGInfo vg : m_availableLVMInfo.keys()) {
        QSet<QString> devDatas;  // 利用Set不能重复的特性来检测vg是否由多个盘合成
        for (QString id : vg.pvIds) {
            for (QList<SPartitionInfo> partList : m_availableDeviceInfo.values()) {
                for (SPartitionInfo part : partList) {
                    if (part.id == id) {
                        devDatas.insert(part.device);
                    }
                }
            }
        }

        // 将多盘磁盘和成的vg对应的pv保存到multipleDiskPvs中
        if (devDatas.size() > 1) {
            multipleDiskPvs.append(vg.pvIds);
        }
    }

    for (SPartitionInfo part : m_availableDeviceInfo.value(dev)) {
        // 检查传入的磁盘中对应的分区是否有在multipleDiskPvs中
        if (multipleDiskPvs.contains(part.id)) {
            text[part.path] = findVG(part.id).name;
            return true;
        }
    }

    return false;
}

void PartitionOperateManager::setOthenOperateInfo()
{
    m_operateInfo.clear();

    // 设置一些磁盘的其他操作信息
}

void PartitionOperateManager::newPartConf(const SPartitionInfo &in, const QList<SPartitionInfo> &infos, QJsonArray &jsons)
{
    if (infos.indexOf(in) == -1) {
        QJsonObject json = getOperateJsonObject(gNewOption, in);
        jsons.append(json);
        if (!in.isCryptHeader) {
            this->setOperateInfo(gNewOption, in);
        }
    }
}

void PartitionOperateManager::delPartConf(const SPartitionInfo &in, const QList<SPartitionInfo> &infos, QJsonArray &jsons)
{
    if (in.partitionType != EPartitionType::PartitionTypeExtension) {
        if (infos.indexOf(in) == -1) {
            QJsonObject json = getOperateJsonObject(gDeleteOption, in);
            jsons.append(json);
            if (!in.isCryptHeader) {
                this->setOperateInfo(gDeleteOption, in);
            }
        }
    }
}

void PartitionOperateManager::editPartConf(const SPartitionInfo &in, const QList<SPartitionInfo> &infos, QJsonArray &jsons)
{
    if (in.partitionType != EPartitionType::PartitionTypeExtension) {
        for (SPartitionInfo it : infos) {
            if (it == in && ((in.mountPoint != it.mountPoint) || (in.filesystem != it.filesystem) || (in.isCrypt != it.isCrypt)) ) {
                QJsonObject json = getOperateJsonObject(gEditOption, it);
                jsons.append(json);
                this->setOperateInfo(gEditOption, it);
            } else if ((it == in) && (in.mountPoint == it.mountPoint) && (in.filesystem == it.filesystem) && (in.isCrypt == it.isCrypt)) {
                QJsonObject json = getOperateJsonObject(gNormalOption, it);
                jsons.append(json);
            }
        }
    }
}

void PartitionOperateManager::editExtensinoPartConf(const SPartitionInfo &in, const QList<SPartitionInfo> &infos, QJsonArray &jsons)
{
    if (in.partitionType == EPartitionType::PartitionTypeExtension) {
        int pos = infos.indexOf(in);
        if (pos == -1) {
            QJsonObject json = getOperateJsonObject(gDeleteOption, in);
            jsons.append(json);
            this->setOperateInfo(gDeleteOption, in);
        } else {
            SPartitionInfo newInfo = infos.at(pos);
            if (newInfo == in && ((in.startPoint != newInfo.startPoint) || (in.endPoint != newInfo.endPoint)) ) {
                QJsonObject json = getOperateJsonObject(gEditOption, newInfo);
                jsons.append(json);
                this->setOperateInfo(gEditOption, newInfo);
            }
        }
    }
}

void PartitionOperateManager::newLVMConf(const SLVMInfo &in, const QList<SLVMInfo> &infos, QJsonArray &jsons)
{
    if (infos.indexOf(in) == -1) {
        QJsonObject json = getOperateJsonObject(gNewOption, in);
        jsons.append(json);
        if (!in.isCryptHeader) {
            this->setOperateInfo(gNewOption, in);
        }
    }
}

void PartitionOperateManager::delLVMConf(const SLVMInfo &in, const QList<SLVMInfo> &infos, QJsonArray &jsons)
{
    if (infos.indexOf(in) == -1) {
        QJsonObject json = getOperateJsonObject(gDeleteOption, in);
        jsons.append(json);
        if (!in.isCryptHeader) {
            this->setOperateInfo(gDeleteOption, in);
        }
    }
}

void PartitionOperateManager::editLVMConf(const SLVMInfo &in, const QList<SLVMInfo> &infos, QJsonArray &jsons)
{
    for (SLVMInfo it : infos) {
        if (it == in && ((in.mountPoint != it.mountPoint) || (in.filesystem != it.filesystem) || (in.isCrypt != it.isCrypt)) ) {
            QJsonObject json = getOperateJsonObject(gEditOption, it);
            jsons.append(json);
            this->setOperateInfo(gEditOption, it);
        }
    }
}

void PartitionOperateManager::newVGConf(const SVGInfo &in, const QList<SVGInfo> &infos, QJsonArray &jsons)
{
    if (infos.indexOf(in) == -1) {
        QJsonObject json = getOperateJsonObject(gNewOption, in);
        jsons.append(json);
        if (!in.isCryptHeader) {
            this->setOperateInfo(gNewOption, in);
        }
    }
}

void PartitionOperateManager::delVGConf(const SVGInfo &in, const QList<SVGInfo> &infos, QJsonArray &jsons)
{
    if (infos.indexOf(in) == -1) {
        QJsonObject json = getOperateJsonObject(gDeleteOption, in);
        jsons.append(json);
        if (!in.isCryptHeader) {
            this->setOperateInfo(gDeleteOption, in);
        }
    }
}

void PartitionOperateManager::formatDiskConf(const SDeviceInfo &in, QJsonArray &jsons)
{
    jsons.append(getOperateJsonObject(gEditOption, in));
    this->setOperateInfo(gEditOption, in);
}

void PartitionOperateManager::formatLVMConf(const SDeviceInfo &in, QJsonArray &jsons)
{
    for (SVGInfo vg : m_availableLVMInfo.keys()) {
        for (SPartitionInfo part : m_availableDeviceInfo[in]) {
            if (vg.pvIds.contains(part.id)) {
                delVGConf(vg, QList<SVGInfo>(), jsons);
                break;
            }
        }
    }
}

void PartitionOperateManager::setOperateInfo(const QString &op, const SPartitionInfo &in)
{
    QString partLabel = in.label.isEmpty() ? in.path : in.label;
    QString partFileSystem = in.filesystem;
    if (partFileSystem.isEmpty()) {
        if (in.partitionType == EPartitionType::PartitionTypeExtension) {
            partLabel = "";
            partFileSystem = CommonFunc::enumTypeToString(in.partitionType);
        }
    } else if (partFileSystem.contains("lvm")) {
        partLabel = "";
    }

    QString partType = "";
    if (in.partitionType == EPartitionType::PartitionTypePrimary) {
        partType = ::QObject::tr("primary partition");
    } else if (in.partitionType == EPartitionType::PartitionTypeLogical) {
        partType = ::QObject::tr("logical partition");
    } else if (in.partitionType == EPartitionType::PartitionTypeExtension) {
        partType = ::QObject::tr("extended partition");
    }

    EPartitionOperate partitionOperate = CommonFunc::qstringToEnumOperate(op);
    if ( partitionOperate == EPartitionOperate::PartitionCreate ) {
        if ( in.mountPoint.isEmpty() ) {
            m_operateInfo.append(::QObject::tr("Create %1 %2 on %3, type: %4")
                                         .arg(partType, partLabel, in.device, partFileSystem));
        } else {
            m_operateInfo.append(::QObject::tr("Create %1 %2 as %3 (mountpoint) on %4, type: %5")
                                         .arg(partType, partLabel, in.mountPoint, in.device, partFileSystem));
        }
    } else if ( partitionOperate == EPartitionOperate::PartitionDelete ) {
        m_operateInfo.append(::QObject::tr("Delete %1 partition").arg(partLabel));
    } else if ( partitionOperate == EPartitionOperate::PartitionEdit ) {
        if ( in.mountPoint.isEmpty() ) {
            m_operateInfo.append(::QObject::tr("Use %1 partition on %2, type: %3")
                                         .arg(partLabel, in.device, partFileSystem));
        } else {
            m_operateInfo.append(::QObject::tr("Use %1 partition as %2 (mountpoint) on %3, type: %4")
                                         .arg(partLabel, in.mountPoint, in.device, partFileSystem));
        }
    }
}

void PartitionOperateManager::setOperateInfo(const QString &op, const SDeviceInfo &in)
{
    m_operateInfo.append(::QObject::tr("Format %1 and create %2 partition table").arg(in.device, CommonFunc::diskLabelToString(in.type)));
}

void PartitionOperateManager::setOperateInfo(const QString &op, const SVGInfo &in)
{
    EPartitionOperate partitionOperate = CommonFunc::qstringToEnumOperate(op);
    if ( partitionOperate == EPartitionOperate::PartitionCreate ) {
        m_operateInfo.append(::QObject::tr("Create VG %1").arg(in.name));
    } else if ( partitionOperate == EPartitionOperate::PartitionDelete ) {
        m_operateInfo.append(::QObject::tr("Delete VG %1").arg(in.name));
    } else if ( partitionOperate == EPartitionOperate::PartitionEdit ) {
        m_operateInfo.append(::QObject::tr("Edit VG %1").arg(in.name));
    }
}

void PartitionOperateManager::setOperateInfo(const QString &op, const SLVMInfo &in)
{
    EPartitionOperate partitionOperate = CommonFunc::qstringToEnumOperate(op);
    if ( partitionOperate == EPartitionOperate::PartitionCreate ) {
        m_operateInfo.append(::QObject::tr("Create LV %1 on %2").arg(in.name, in.vgName));
    } else if ( partitionOperate == EPartitionOperate::PartitionDelete ) {
        m_operateInfo.append(::QObject::tr("Delete LV %1 on %2").arg(in.name, in.vgName));
    } else if ( partitionOperate == EPartitionOperate::PartitionEdit ) {
        m_operateInfo.append(::QObject::tr("Edit LV %1 on %2").arg(in.name, in.vgName));
    }
}

SVGInfo PartitionOperateManager::findVG(const QString &pvID)
{
    for (SVGInfo vg : m_availableLVMInfo.keys()) {
        if (vg.pvIds.contains(pvID)) {
            return vg;
        }
    }

    return SVGInfo();
}

QJsonObject PartitionOperateManager::getOperateJsonObject(const QString &oper, const SDeviceInfo &in)
{
    QJsonObject json;
    json.insert("type", "disk");
    json.insert("operate", oper);
    json.insert("device", in.device);
    json.insert("diskType", CommonFunc::diskLabelToString(in.type));
    json.insert("ptuuid", in.ptuuid);

    return json;
}

QJsonObject PartitionOperateManager::getOperateJsonObject(const QString &oper, const SVGInfo &in)
{
    QJsonObject json;
    json.insert("type", "VG");
    json.insert("operate", oper);
    json.insert("id", in.id);
    json.insert("size", in.size);
    json.insert("name", in.name);
    json.insert("pv", QJsonArray::fromStringList(in.pvIds));
    json.insert("isCryptHeader", in.isCryptHeader);

    return json;
}

QJsonObject PartitionOperateManager::getOperateJsonObject(const QString &oper, const SLVMInfo &in)
{
    QJsonObject json;
    json.insert("type", "LVM");
    json.insert("operate", oper);
    json.insert("id", in.id);
    json.insert("size", in.size);
    json.insert("name", in.name);
    json.insert("label", in.label);
    json.insert("path", in.path);
    json.insert("vg", in.vgName);
    json.insert("filesystem", in.filesystem);
    json.insert("mountPoint", in.mountPoint);
    json.insert("isneedformat", in.isNeedFormat);
    json.insert("isCrypt", in.isCrypt);
    json.insert("cryptPassword", in.cryptPassword);
    json.insert("isCryptHeader", in.isCryptHeader);

    return json;
}


QJsonObject PartitionOperateManager::getOperateJsonObject(const QString &oper, const SPartitionInfo &in)
{
    QJsonObject json;
    json.insert("id", in.id);
    json.insert("type", "partition");
    json.insert("operate", oper);
    json.insert("device", in.device);
    json.insert("filesystem", in.filesystem);
    json.insert("mountPoint", in.mountPoint);
    json.insert("label", in.label);
    json.insert("startPoint", in.startPoint);
    json.insert("endPoint", in.endPoint);
    json.insert("size", in.size);
    json.insert("index", in.index);
    json.insert("isstartpoint", in.isStartPoint);
    json.insert("partType", CommonFunc::enumTypeToString(in.partitionType));
    json.insert("sectorSize", in.sectorSize);
    json.insert("deviceSize", in.deviceSize);
    json.insert("path", in.path);
    json.insert("isneedformat", in.isNeedFormat);
    json.insert("isCrypt", in.isCrypt);
    json.insert("cryptPassword", in.cryptPassword);
    json.insert("isCryptHeader", in.isCryptHeader);
    json.insert("isNeedCryptHeader", in.isNeedCryptHeader);
    json.insert("partUuid", in.partUuid);

    return json;
}

bool PartitionOperateManager::writeConf(QString &err)
{
    // 获取分区操作信息
    if (m_operateJsonArray.isEmpty()) {
        getPartedOperatorInfo();
    }

    // 写入分区的配置
    QJsonDocument jsonDoc;
    jsonDoc.setArray(m_operateJsonArray);

    QString str = CommonFunc::getSettingsString("DI_PARTITION_CONFIG");
    QFile file(str);
    file.remove();
    if (!file.open(QIODevice::ReadWrite)) {
        err = QString("%1 File open error. err:<%2>").arg(str, file.errorString());
        return false;
    }

    file.write(jsonDoc.toJson());
    file.close();

    printPartitions(m_currentDeviceInfo);
    printPartitions(m_currentLVMInfo);
    qInfo() << qPrintable(m_operateInfo.join("\n\n"));

    return true;
}

void PartitionOperateManager::printPartitions(const SVGInfo &vg)
{
    qInfo() << "vg info: " << vg.toString().toStdString().c_str();
    qInfo() << "lv cnt: " << m_currentLVMInfo.value(vg).size();
    for (SLVMInfo in : m_currentLVMInfo.value(vg)) {
        qInfo() << "lv info:" << in.toString().toStdString().c_str();
    }
}

void PartitionOperateManager::printPartitions(const QMap<SVGInfo, QList<SLVMInfo>> &devsInfo)
{
    for (SVGInfo dev : devsInfo.keys()) {
        qInfo() << "vg info: " << dev.toString().toStdString().c_str();
        qInfo() << "lv cnt: " << devsInfo.value(dev).size();
        for (SLVMInfo in : devsInfo.value(dev)) {
            qInfo() << "lv info:" << in.toString().toStdString().c_str();
        }
    }
}

void PartitionOperateManager::printPartitions(const SDeviceInfo &dev)
{
    qInfo() << "device info:" << dev.toString().toStdString().c_str();
    int cnt = 0;
    for (SPartitionInfo in : m_currentDeviceInfo.value(dev)) {
        qInfo() << QString("part%1 info:").arg(cnt++) << in.toString().toStdString().c_str();
    }
}

void PartitionOperateManager::printPartitions(const QMap<SDeviceInfo, QList<SPartitionInfo>> &devsInfo)
{
    qInfo() << "device info cnt: " << devsInfo.size();
    for (SDeviceInfo dev : devsInfo.keys()) {
        qInfo() << "device info: " << dev.toString().toStdString().c_str() << " part cnt: " << devsInfo.value(dev).size();
        for (SPartitionInfo in : devsInfo.value(dev)) {
            qInfo() << "part info:" << in.toString().toStdString().c_str();
        }
    }
}

void PartitionOperateManager::setFormatDisk(const SDeviceInfo &dev)
{
    if (!dev.device.isEmpty()) {
        SDeviceInfo curDev = dev;
        curDev.isFormat = true;
        curDev.type = CommonFunc::isUEFI() ? EDisklabelType::GPT : EDisklabelType::MsDos;
        QList<SPartitionInfo> parts = m_currentDeviceInfo.value(curDev);
        m_currentDeviceInfo.remove(curDev);
        m_currentDeviceInfo[curDev] = parts;
    }
}

void PartitionOperateManager::resetDisk(const SDeviceInfo &dev)
{
    if (!dev.device.isEmpty()) {
        m_currentDeviceInfo.remove(dev);
        QList<SPartitionInfo> avaParts = m_availableDeviceInfo.value(dev);
        SDeviceInfo avaDev = m_availableDeviceInfo.key(avaParts);

        m_currentDeviceInfo[avaDev] = avaParts;
    }
}

void PartitionOperateManager::resetDisk()
{
    m_currentDeviceInfo = m_availableDeviceInfo;
    this->resetVGDevice();
}

void PartitionOperateManager::resetVGDevice()
{
    m_currentLVMInfo = m_availableLVMInfo;
}

bool PartitionOperateManager::isBaseInstallMode() const
{
    //根据分区类型决定是否显示初始化备份勾选控件
    int partType = CommonFunc::getSettingsInt("DI_PARTITION_TYPE");
    switch (partType) {
        case FullDiskMode: // 全盘分区
        case LvmFullDisk:  // 全盘lvm
        case Cryptsetup:   // 全盘加密
        case SaveData:     // 保留用户数据
            return true;
        default:
            return false;
    }
}


void PartitionOperateManager::setUimgDirSize(QMap<QString, quint64> &dirSize)
{
    m_uimgDirSize = dirSize;
}

QMap<QString, quint64> PartitionOperateManager::getUimgDirSize()
{
    return m_uimgDirSize;
}

quint64 PartitionOperateManager::getGhostPartitionSize(const QString &partmt)
{
    return m_uimgDirSize.contains(partmt) ? m_uimgDirSize[partmt] / (1024 * 1024) : 0;
}

void PartitionOperateManager::mergePartitionInfo(QList<GhostCheckInfo> &ghostCheckList)
{
    auto devInfo = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    auto lvmInfo = PartitionOperateManager::instance()->getCurrentLVMInfo();
    QList<SPartitionInfo> totalPartition;
    QList<SLVMInfo> totalLvm;
    std::for_each(devInfo.begin(), devInfo.end(), [&](const QList<SPartitionInfo> &partList){
        totalPartition.append(partList);
    });
    std::for_each(lvmInfo.begin(), lvmInfo.end(), [&](const QList<SLVMInfo> &lvmList){
        totalLvm.append(lvmList);
    });

    for (auto &partInfo: totalPartition) {
        GhostCheckInfo checkPart;
        checkPart.mountPoint = partInfo.mountPoint;
        checkPart.path = partInfo.device;
        checkPart.size = partInfo.size;
        ghostCheckList.append(checkPart);
    }

    for (auto &lvmInfo: totalLvm) {
        GhostCheckInfo checkPart;
        checkPart.mountPoint = lvmInfo.mountPoint;
        checkPart.path = lvmInfo.path;
        checkPart.size = lvmInfo.size;
        ghostCheckList.append(checkPart);
    }
}

quint64 PartitionOperateManager::getGhostRootSize(const QList<GhostCheckInfo> &ghostCheckList, QString &err)
{
    QList<QString> ghostPart = m_uimgDirSize.keys();
    auto pred = [](const QString &info, const QString &value){return info == value; };
    auto findmt = [&](const QString &partmt){ return std::find_if(ghostPart.begin(), ghostPart.end(),
                                                                  std::bind(pred, std::placeholders::_1, partmt)); };
    quint64 ghostRootSize = m_uimgDirSize.value("/");
    QStringList mountPointList;
    bool existHome=false;

    for (auto &checkPart: ghostCheckList) {
        if(checkPart.mountPoint == "/home") {
            existHome = true;
        }
    }

    for (auto &checkPart: ghostCheckList) {
        //只搜索非根分区
        if(!checkPart.mountPoint.isEmpty() && checkPart.mountPoint != "/") {
            if (checkPart.mountPoint == "swap") {
                continue;
            }

            if(checkPart.mountPoint == "/persistent") {
                //mountPointList = QStringList{"/persistent", "/var", "/root", "/opt"};
                mountPointList = QStringList{"/persistent"};
                if (!existHome) {
                    mountPointList << "/home";
                }
            } else {
                mountPointList = QStringList(checkPart.mountPoint);
            }

            quint64 totalSize = 0;
            std::for_each(mountPointList.begin(), mountPointList.end(), [&](const QString &mountPoint){
                auto iter = findmt(mountPoint);
                if(iter != ghostPart.end()) {
                    totalSize += m_uimgDirSize.value(*iter);
                }
            });

            if(checkPart.size * 1024 * 1024 < totalSize) {
                err = ::QObject::tr("Insufficient space in %1 partition").arg(checkPart.path);
                qInfo() << "uimg partition size:" << totalSize;
                qInfo() << "partition " << checkPart.path << " size is too small:" << checkPart.size;
                return 0;
            }

            ghostRootSize -= totalSize;
            qInfo() << QString("ghost root sub part %1, size:%2").arg(checkPart.mountPoint).arg(totalSize);
        }
    }

    return ghostRootSize;
}

bool PartitionOperateManager::checkGhostSpace(QString &err)
{
    qInfo() << "check ghost root space";
    QList<GhostCheckInfo> ghostCheckList;
    mergePartitionInfo(ghostCheckList);

    quint64 ghostRootSize = getGhostRootSize(ghostCheckList, err);
    if(ghostRootSize == 0) {
        return false;
    }
    //check root size
    auto rootPart = std::find_if(ghostCheckList.begin(), ghostCheckList.end(), [&](const GhostCheckInfo &partInfo){
        return partInfo.mountPoint == "/";});
    if(rootPart == ghostCheckList.end()) {
        err = ::QObject::tr("you need create a / partition first");
        return false;
    }

    quint64 currentRootSize = rootPart->size * 1024 * 1024;
    qInfo() << "current Root Size:" << currentRootSize << ", ghost root size:" << ghostRootSize;
    if(currentRootSize < ghostRootSize) {
        qWarning() << "root size is not enough:" << currentRootSize << ghostRootSize;
        err = ::QObject::tr("The system space is not enough, please scale up to %1Gib or above.").arg(CommonFunc::bytesToG((qint64)ghostRootSize) + 5);
        return false;
    }

    return true;
}
