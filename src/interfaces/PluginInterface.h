#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QFrame>

class PluginInterface
{
public:
    virtual bool load() {return true;}
    virtual ~PluginInterface(){}
};

#define PluginInterface_iid "Plugins.PluginsManager.PluginInterface"
Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

class TaskPluginInterface : public QObject, public PluginInterface
{
    Q_OBJECT
public:
    virtual bool start() = 0;

signals:
    void progress(int);
};

#endif // PLUGININTERFACE_H
