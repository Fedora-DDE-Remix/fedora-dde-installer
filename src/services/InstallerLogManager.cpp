#include "InstallerLogManager.h"

#include <fcntl.h>
#include <unistd.h>
#include <QDebug>

InstallerLogManager *InstallerLogManager::instance()
{
    static InstallerLogManager lm;
    return &lm;
}

InstallerLogManager::InstallerLogManager()
{
}

bool InstallerLogManager::redirectLog(const QString &file)
{
    int fd = open(file.toStdString().c_str(), O_RDWR | O_CREAT | O_TRUNC
                  , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        qWarning() << "Failed to create log file " << file;
        return false;
    }

    if (dup2(fd, STDOUT_FILENO) < 0) {
        qWarning() << "Failed to redirect stdout";
        close(fd);
        return false;
    }

    if (dup2(fd, STDERR_FILENO) < 0) {
        qWarning() << "Failed to redirect stderr";
        close(fd);
        return false;
    }

    close(fd);

    return true;
}
