#ifndef INSTALLERLOGMANAGER_H
#define INSTALLERLOGMANAGER_H

#include <QString>

class InstallerLogManager
{
public:
    static InstallerLogManager* instance();

    // 重定向stdout和stderr到file
    bool redirectLog(const QString& file);

private:
    InstallerLogManager();
    InstallerLogManager(const InstallerLogManager&) = delete;
};

#endif // INSTALLERLOGMANAGER_H
