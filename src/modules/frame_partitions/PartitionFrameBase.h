#ifndef PARTITIONFRAMEBASE_H
#define PARTITIONFRAMEBASE_H

#include <QWidget>

class PartitionFrameBase : public QWidget
{
    Q_OBJECT
public:
    explicit PartitionFrameBase(QWidget *parent = nullptr);

    virtual void updateTs() = 0;
signals:
    void next();
public slots:
};

#endif // PARTITIONFRAMEBASE_H
