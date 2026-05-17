#ifndef CONVENTIONALPARTITION_H
#define CONVENTIONALPARTITION_H

#include "PartitionInterface.h"
#include "structs.h"

class ConventionalPartition : public PartitionInterface {
public:
    ~ConventionalPartition() override;

public:
    bool start(const QString &config, QString &err) override;

private:
    bool diskTask(const QString &config, QString &err);
    bool partitionTask(const QString &config, QList<SPartitionOperate> &infos, QString &err);
    bool vgTask(const QString &config, const QList<SPartitionOperate> &filter, QString &err);
    bool lvmTask(const QString &config, QList<SLVMOperate> &infos, const QList<SPartitionOperate> &partitionInfos, QString &err);
    bool lvmCryptHeaderTreat(const SLVMOperate &lvmInfo, const QList<SPartitionOperate> &partitionInfos, QString &err);
};

#endif // CONVENTIONALPARTITION_H
