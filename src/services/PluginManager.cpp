#include "PluginManager.h"
#include "PluginInterface.h"
#include "CommonFunc.h"

#include <QJsonArray>
#include <QDir>
#include <QApplication>
#include <QDebug>

PluginsManager* PluginsManager::m_instance = nullptr;
PluginsManager *PluginsManager::instance(){
    if(m_instance == nullptr)
        m_instance = new PluginsManager();
    return m_instance;
}

void PluginsManager::loadPlugin(const QString &filepath)
{
    PluginInterface *plugin = getPathPlugin(filepath);
    if (plugin) {
        d->m_loaders[filepath] = plugin;
    } else {
        unloadPlugin(filepath);
    }
}

void PluginsManager::unloadPlugin(const QString &filepath)
{
    PluginInterface *loader = d->m_loaders.value(filepath);
    //卸载插件，并从内部数据结构中移除
    if(loader != nullptr)
    {
        d->m_loaders.remove(filepath);
        delete loader;
        loader = nullptr;
    }
}

PluginInterface *PluginsManager::getPlugin(const QString &filepath)
{
     PluginInterface *rest = d->m_loaders.value(filepath);
     if (rest == nullptr) {
        loadPlugin(filepath);
     }
     return d->m_loaders.value(filepath);
}

QVariant PluginsManager::getPluginName(PluginInterface *loader)
{
    return d->m_loaders.key(loader);
}

PluginInterface *PluginsManager::getPathPlugin(const QString &filepath)
{
    if(!QLibrary::isLibrary(filepath)) {
        qWarning() << filepath << " is not library.";
        return nullptr;
    }

    //检测依赖
    if(!d->check(filepath)) {
        qWarning() << filepath << " is check failed.";
        return nullptr;
    }

    //加载插件
    QPluginLoader *loader = new QPluginLoader(filepath);
    if(loader->load())
    {
        PluginInterface *plugin = qobject_cast<PluginInterface *>(loader->instance());
        if (plugin) {
            qInfo() << filepath << " is load successful.";

        } else {
            qInfo() << filepath << " is qobject cast failed.";
        }

        return plugin;
    }  else {
        qWarning() << filepath << " is load failed." << loader->errorString();
    }

    return nullptr;
}

PluginsManager::PluginsManager():
    d(QSharedPointer<PluginsManagerPrivate>(new PluginsManagerPrivate))
{
}


