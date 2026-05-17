
#ifndef MODULES_LOADER_H
#define MODULES_LOADER_H

#include "tray_module_interface.h"

#include <QHash>
#include <QThread>

namespace dss {
namespace module {

class BaseModuleInterface;
class ModulesLoader : public QThread
{
Q_OBJECT
public:
    static ModulesLoader &instance();

    inline QHash<QString, BaseModuleInterface *> moduleList() { return m_modules; }
    BaseModuleInterface *findModuleByName(const QString &name) const;
    QHash<QString, BaseModuleInterface *> findModulesByType(const int type) const;

signals:
    void moduleFound(BaseModuleInterface *);

protected:
    void run() override;

private:
    explicit ModulesLoader(QObject *parent = nullptr);
    ~ModulesLoader() override;
    ModulesLoader(const ModulesLoader &) = delete;
    ModulesLoader &operator=(const ModulesLoader &) = delete;

    bool checkVersion(const QString &target, const QString &base);
    void findModule(const QString &path);

private:
    QHash<QString, BaseModuleInterface *> m_modules;
};

} // namespace module
} // namespace dss
#endif // MODULES_LOADER_H
