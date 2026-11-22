#ifndef HOOKSMANAGER_H
#define HOOKSMANAGER_H

#include "structs.h"

#include <QString>
#include <QStringList>
#include <QObject>


class HooksManagerPrivate;

class HooksManager : public QObject {
    Q_OBJECT

public:
    static HooksManager *instance();

public:
    void setWorkspaceDirs(const QStringList &lis);
    void startInstall();
    QString getProgressStatus();

signals:
    void progress(int progress);
    void finished();
    void exitInstall(const QString &err);

private:
    void updateProgress();

private:
    HooksManager();
    HooksManager(const HooksManager&) = delete;

private:
    static HooksManager *m_instance;
    bool m_started = false;
    bool m_succeeded = false;
    int m_progress = 0;
    QString m_error;
    QStringList m_directories;
};




#endif // HOOKSMANAGER_H
