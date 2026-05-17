#ifndef TASKPARTITION_H
#define TASKPARTITION_H

#include "PluginInterface.h"

class TaskPartition : public TaskPluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)
public:
    bool start() override;

signals:
    void progress(int);
};

#endif // TASKPARTITION_H
