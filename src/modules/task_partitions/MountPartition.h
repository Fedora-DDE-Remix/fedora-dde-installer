#ifndef MOUNTPARTITION_H
#define MOUNTPARTITION_H

#include "PartitionInterface.h"

class MountPartition : public PartitionInterface {
public:
    ~MountPartition() override;

public:
    bool start(const QString &config, QString &err) override;
private:
    //  挂载前处理
    QList<PartMountPointInfo> beforeMounting(const QString &config);

    //  分区挂载
    bool mount(const QString &device, const QString &mountPoint, QString &err);

    //  swap分区激活
    bool swapon(const QString &device, QString &err);
};

#endif // MOUNTPARTITION_H
