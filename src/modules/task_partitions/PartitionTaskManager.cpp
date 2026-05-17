#include "PartitionTaskManager.h"
#include "CommonFunc.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariant>
#include <QUuid>
#include <QDateTime>
#include <QTimeZone>


PartitionTaskManager* PartitionTaskManager::m_instance = nullptr;

PartitionTaskManager::PartitionTaskManager()
{

}

PartitionTaskManager *PartitionTaskManager::instance()
{
    if(m_instance == nullptr)
        m_instance = new PartitionTaskManager();
    return m_instance;
}

bool PartitionTaskManager::getDeviceOperator(const QString &conf, QList<SDeviceOperate> &infos, QString &error)
{
    QJsonDocument policydoc = QJsonDocument();
    if (!CommonFunc::readJsonFile(conf, policydoc, error)) {
        return false;
    }

    const QJsonArray partitionOperateList = policydoc.array();
    for (const QJsonValue& partitionOperate : partitionOperateList) {
        const QJsonObject obj = partitionOperate.toObject();
        if (!obj.value("type").toString().compare("disk")) {
            SDeviceOperate devinfoOperateInfo;
            devinfoOperateInfo.operate              = CommonFunc::qstringToEnumOperate(obj.value("operate").toString());
            devinfoOperateInfo.device.device        = obj.value("device").toString();
            devinfoOperateInfo.device.type          =  CommonFunc::qstringToEnumDisklabel(obj.value("diskType").toString());
            devinfoOperateInfo.device.ptuuid        = obj.value("ptuuid").toString();
            infos.append(devinfoOperateInfo);
        }
    }

    return true;
}

bool PartitionTaskManager::getDeviceOperator(const QString &conf, QList<SPartitionOperate> &infos, QString &error)
{
    QJsonDocument policydoc = QJsonDocument();
    if (!CommonFunc::readJsonFile(conf, policydoc, error)) {
        return false;
    }

    const QJsonArray partitionOperateList = policydoc.array();
    for (const QJsonValue& partitionOperate : partitionOperateList) {
        const QJsonObject obj = partitionOperate.toObject();
        if (!obj.value("type").toString().compare("partition")) {
            SPartitionOperate partitionOperateInfo;
            partitionOperateInfo.partition.device        = obj.value("device").toString();
            partitionOperateInfo.partition.id            = obj.value("id").toString();
            partitionOperateInfo.partition.label         = obj.value("label").toString();
            partitionOperateInfo.partition.filesystem    = obj.value("filesystem").toString();
            partitionOperateInfo.partition.mountPoint    = obj.value("mountPoint").toString();
            partitionOperateInfo.partition.size          = obj.value("size").toVariant().toLongLong();
            partitionOperateInfo.partition.startPoint    = obj.value("startPoint").toVariant().toLongLong();
            partitionOperateInfo.partition.endPoint      = obj.value("endPoint").toVariant().toLongLong();
            partitionOperateInfo.partition.sectorSize    = obj.value("sectorSize").toVariant().toLongLong();
            partitionOperateInfo.partition.deviceSize    = obj.value("deviceSize").toVariant().toLongLong();
            partitionOperateInfo.partition.index         = obj.value("index").toInt();
            partitionOperateInfo.partition.isStartPoint  = obj.value("isstartpoint").toBool();
            partitionOperateInfo.partition.partitionType = CommonFunc::qstringToEnumType(obj.value("partType").toString());
            partitionOperateInfo.partition.path          = obj.value("path").toString();
            partitionOperateInfo.partition.isNeedFormat  = obj.value("isneedformat").toBool();
            partitionOperateInfo.partition.isCrypt       = obj.value("isCrypt").toBool();
            partitionOperateInfo.partition.cryptPassword = obj.value("cryptPassword").toString();
            partitionOperateInfo.partition.isCryptHeader = obj.value("isCryptHeader").toBool();
            partitionOperateInfo.partition.isNeedCryptHeader = obj.value("isNeedCryptHeader").toBool();
            partitionOperateInfo.partition.partUuid      = obj.value("partUuid").toString();
            partitionOperateInfo.operate                 = CommonFunc::qstringToEnumOperate(obj.value("operate").toString());
            infos.append(partitionOperateInfo);
        }
    }

    return true;
}

bool PartitionTaskManager::getDeviceOperator(const QString &conf, QList<SVGOperate> &infos, QString &error)
{
    QJsonDocument policydoc = QJsonDocument();
    if (!CommonFunc::readJsonFile(conf, policydoc, error)) {
        return false;
    }

    const QJsonArray vgOperateList = policydoc.array();
    for (const QJsonValue& vgOperate : vgOperateList) {
        const QJsonObject obj = vgOperate.toObject();
        if (!obj.value("type").toString().compare("VG")) {
            SVGOperate vgOperateInfo;
            vgOperateInfo.vg.id   = obj.value("id").toString();
            vgOperateInfo.vg.name = obj.value("name").toString();
            vgOperateInfo.vg.isCryptHeader = obj.value("isCryptHeader").toBool();
            const QJsonArray pvIdList = obj.value("pv").toArray();
            for (auto partitionId : pvIdList) {
                vgOperateInfo.vg.pvIds.append(partitionId.toString());
            }
            vgOperateInfo.operate = CommonFunc::qstringToEnumOperate(obj.value("operate").toString());
            infos.append(vgOperateInfo);
        }
    }

    return true;
}

bool PartitionTaskManager::getDeviceOperator(const QString &conf, QList<SLVMOperate> &infos, QString &error)
{
    QJsonDocument policydoc = QJsonDocument();
    if (!CommonFunc::readJsonFile(conf, policydoc, error)) {
        return false;
    }

    const QJsonArray lvmOperateList = policydoc.array();
    for (const QJsonValue& lvmOperate : lvmOperateList) {
        const QJsonObject obj = lvmOperate.toObject();
        if (!obj.value("type").toString().compare("LVM")) {
            SLVMOperate lvmOperateInfo;
            lvmOperateInfo.lvm.id           = obj.value("id").toString();
            lvmOperateInfo.lvm.name = obj.value("name").toString();
            lvmOperateInfo.lvm.filesystem   = obj.value("filesystem").toString();
            lvmOperateInfo.lvm.mountPoint   = obj.value("mountPoint").toString();
            lvmOperateInfo.lvm.label        = obj.value("label").toString();
            lvmOperateInfo.lvm.path         = obj.value("path").toString();
            lvmOperateInfo.lvm.size         = obj.value("size").toVariant().toLongLong();
            lvmOperateInfo.lvm.vgName       = obj.value("vg").toString();
            lvmOperateInfo.lvm.isNeedFormat = obj.value("isneedformat").toBool();
            lvmOperateInfo.lvm.isCrypt       = obj.value("isCrypt").toBool();
            lvmOperateInfo.lvm.cryptPassword = obj.value("cryptPassword").toString();
            lvmOperateInfo.lvm.isCryptHeader = obj.value("isCryptHeader").toBool();
            lvmOperateInfo.operate          = CommonFunc::qstringToEnumOperate(obj.value("operate").toString());
            infos.append(lvmOperateInfo);
        }
    }

    return true;
}
