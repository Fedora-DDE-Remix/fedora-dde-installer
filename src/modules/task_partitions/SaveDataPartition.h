#ifndef SAVEDATAPAPARTITION_H
#define SAVEDATAPAPARTITION_H

#include "PartitionInterface.h"
#include "structs.h"

#include <QList>

class SaveDataPartition : public PartitionInterface {
public:
    ~SaveDataPartition() override;

public:
    bool start(const QString &config, QString &err) override;
};

#endif // SAVEDATAPAPARTITION_H
