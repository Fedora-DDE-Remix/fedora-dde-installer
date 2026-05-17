#ifndef CRYPTSETUPPARTITION_H
#define CRYPTSETUPPARTITION_H

#include "PartitionInterface.h"
#include "structs.h"

class CryptsetupPartition : public PartitionInterface {
public:
    ~CryptsetupPartition() override;

public:
    bool start(const QString &config, QString &err) override;

private:
    bool diskTask(const QString &config, QString &err);
    bool partitionTask(const QString &config, QList<SPartitionOperate> &infos, QString &err);
    bool vgTask(const QString &config, const QList<SPartitionOperate> &filter, QString &err);
    bool lvmTask(const QString &config, QList<SLVMOperate> &infos, QString &err);
};

#endif // CRYPTSETUPPARTITION_H
