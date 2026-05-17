#ifndef PLUGINSMANAGER_H
#define PLUGINSMANAGER_H

#include "globals.h"
#include "inner/PluginManagerPrivate.h"

#include <QObject>
#include <QPluginLoader>
#include <QVariant>
#include <QSharedPointer>

class PluginInterface;
class QTPLUGINSMANAGERSHARED_EXPORT PluginsManager : public QObject
{
    Q_OBJECT
public:
    static PluginsManager *instance();
    //加载其中某个插件
    void loadPlugin(const QString &filepath);
    //卸载某个插件
    void unloadPlugin(const QString &filepath);
    //获取某个插件
    PluginInterface* getPlugin(const QString &filepath);
    //获取某个插件名称
    QVariant getPluginName(PluginInterface *loader);

    PluginInterface *getPathPlugin(const QString &filepath);

private:
    PluginsManager();
    PluginsManager(const PluginsManager&) = delete;

private:
    static PluginsManager *m_instance;
    QSharedPointer<PluginsManagerPrivate> d;
};

#endif
