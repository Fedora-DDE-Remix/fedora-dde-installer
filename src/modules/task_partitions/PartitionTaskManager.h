#ifndef PARTITIONTASKMANAGER_H
#define PARTITIONTASKMANAGER_H

#include "structs.h"

class PartitionTaskManager
{
public:
    static PartitionTaskManager *instance();

public:
    bool validateEncryptionPlan(const QString &conf, QString &error);
    bool getDeviceOperator(const QString &conf, QList<SDeviceOperate> &infos, QString &error);
    bool getDeviceOperator(const QString &conf, QList<SPartitionOperate> &infos, QString &error);
    bool getDeviceOperator(const QString &conf, QList<SVGOperate> &infos, QString &error);
    bool getDeviceOperator(const QString &conf, QList<SLVMOperate> &infos, QString &error);

private:
    PartitionTaskManager();
    PartitionTaskManager(const PartitionTaskManager&) = delete;
    void operator =(const PartitionTaskManager &) = delete;

private:
    static PartitionTaskManager *m_instance;

};

#endif // PARTITIONTASKMANAGER_H
