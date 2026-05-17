#ifndef PARTITIONINTERFACE_H
#define PARTITIONINTERFACE_H

#include <QString>
#include <QList>
#include <QVariant>
#include <QPair>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

#include "structs.h"
#include "CommonFunc.h"
#include "globals.h"

class PartitionInterface {
public:
    virtual ~PartitionInterface() {}
    virtual bool start(const QString &config, QString &err) = 0;

protected:
    virtual void setupMountPoint(const QList<SPartitionOperate> &partInfos, const QList<SLVMOperate> &lvmInfos)
    {
        int compress = CommonFunc::getSettingsInt("DI_ENABLE_COMPRESS");
        // 新建分区
        QStringList mp_list;
        for (auto &in : partInfos) {
            if ((in.operate == EPartitionOperate::PartitionDelete) || (in.operate == EPartitionOperate::PartitionNormal)) {
                continue;
            }

            QString mountPoint = in.partition.mountPoint;
            if (gSwapFileSystems.contains(in.partition.filesystem)) {
                mountPoint = gSwapMountPoint;
            }

            if (!mountPoint.isEmpty()) {
                QString strMount = QString("%1=%2").arg(in.partition.path, mountPoint);
                if(in.partition.filesystem == "btrfs" && compress == 1) {
                    strMount.append("[compress=zstd]");
                }

                mp_list.append(strMount);
            }
        }

        for (auto &in : lvmInfos) {
            if ((in.operate == EPartitionOperate::PartitionDelete) || (in.operate == EPartitionOperate::PartitionNormal)) {
               continue;
            }

            QString mountPoint = in.lvm.mountPoint;
            if (gSwapFileSystems.contains(in.lvm.filesystem)) {
                mountPoint = gSwapMountPoint;
            }

            if (!mountPoint.isEmpty()) {
                QString strMount = QString("%1=%2").arg(in.lvm.path, mountPoint);
                if(in.lvm.filesystem == "btrfs" && compress == 1) {
                    strMount.append("[compress=zstd]");
                }

                mp_list.append(strMount);
            }
        }

        CommonFunc::setSettingsValue("DI_MOUNTS_POINTS", mp_list.join(";"));
    }

    // 更新存放加密头的分区信息
    bool updateCryptHeaderHidePartInfo(const QString &config, const QList<SPartitionOperate> &partInfos, QString &err) {
        if (!CommonFunc::getSettingsBool("DI_USE_HIDE_CRYPT_HEADER")) {
            return true;
        }
        QJsonDocument policyDoc = QJsonDocument();
        if (!CommonFunc::readJsonFile(config, policyDoc, err)) {
            return false;
        }

        const QJsonArray partitionOperateList = policyDoc.array();
        QJsonArray partitionOperateListOut;
        QList<QPair<QString, QString>> vgNewNameList;
        for (const QJsonValue& partitionOperate : partitionOperateList) {
            QJsonObject obj = partitionOperate.toObject();
            if (!obj.value("type").toString().compare("VG")) {
                if (obj.value("name").toString().contains("usec-vg-")) {
                    // 更新vgname
                    QString vgName = QString("usec-vg-%1").arg(obj.value("pv").toArray().first().toString());
                    vgName.replace("{", "").replace("}", "");

                    if (!obj.value("name").toString().compare(vgName)) {
                        QList<SPartitionOperate>::const_iterator partIterItem = std::find_if(partInfos.begin(), partInfos.end(), [=](const SPartitionOperate& partitionOperate) {
                            return partitionOperate.partition.id == obj.value("pv").toArray().first().toString();
                        });
                        if (partIterItem != partInfos.end()) {
                            // 根据记录的pv的uuid查找对应分区的ptuuid即pv分区所在磁盘的ptuuid
                            QString cmd = QString("echo -n $(lsblk -lpno NAME,UUID,PTUUID | grep -w %1 | awk '{print $NF}')").arg((*partIterItem).partition.path);
                            if (!CommonFunc::command("/bin/bash", {"-c", cmd}, err)) {
                                err = QString("Command: %1 failed. %2").arg(cmd, err);
                                return false;
                            }

                            QString newVgName = QString("usec-vg-%1").arg(err);
                            vgNewNameList.append(QPair<QString, QString>(vgName, newVgName));
                            obj["name"] = QJsonValue(newVgName);
                        }
                    }
                }
            } else if (!obj.value("type").toString().compare("LVM")) {
                // 更新lvm配置中的vg名称
                QString vgName = obj.value("vg").toString();
                QList<QPair<QString, QString>>::Iterator vgNameIter = std::find_if(vgNewNameList.begin(), vgNewNameList.end(), [=](const QPair<QString, QString> &pair) {
                    return pair.first == vgName;
                });
                if (vgNameIter != vgNewNameList.end()) {
                    obj["vg"] = QJsonValue((*vgNameIter).second);
                }

                // 更新lvm配置中的lv名称
                QString lvName = obj.value("name").toString();
                QList<SPartitionOperate>::const_iterator partIterItem = std::find_if(partInfos.begin(), partInfos.end(), [=](const SPartitionOperate& partitionOperate) {
                    QString testLvName = QString("lv-%1").arg(partitionOperate.partition.id);
                    testLvName.replace("{", "").replace("}", "");
                    return lvName == testLvName;
                });
                if (partIterItem != partInfos.end()) {
                    obj["name"] = QString("lv-%1").arg((*partIterItem).partition.partUuid);
                    obj["label"] = QString("lv-%1").arg((*partIterItem).partition.partUuid);
                }
            } else if (!obj.value("type").toString().compare("partition")) {
                // 更新配置中分区的路径信息
                QList<SPartitionOperate>::const_iterator partIterItem = std::find_if(partInfos.begin(), partInfos.end(), [=](const SPartitionOperate& partitionOperate) {
                    return partitionOperate.partition.id == obj.value("id").toString();
                });
                if (partIterItem != partInfos.end()) {
                    obj["path"] = (*partIterItem).partition.path;
                }
            }
            partitionOperateListOut.append(obj);
        }

        // 写入分区的配置
        QJsonDocument policyDocOut;
        policyDocOut.setArray(partitionOperateListOut);
        QString bakFileName = config + "_bak";
        if (QFile::exists(bakFileName)) {
            QFile::remove(bakFileName);
        }
        QFile::rename(config, config + "_bak");
        QFile file(config);
        if (!file.open(QIODevice::ReadWrite)) {
            err = QString("%1 File open error. err:<%2>").arg(config, file.errorString());
            return false;
        }
        file.write(policyDocOut.toJson());
        file.close();

        return true;
    }
};

#endif // PARTITIONINTERFACE_H

