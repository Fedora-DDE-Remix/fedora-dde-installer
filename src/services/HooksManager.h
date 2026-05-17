#ifndef HOOKSMANAGER_H
#define HOOKSMANAGER_H

#include "structs.h"

#include <QThread>
#include <QString>
#include <QObject>

#include <QSharedPointer>

class HooksManagerPrivate;

class HooksManager : public QThread {
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
    static bool m_started;
    HooksManagerPrivate *d;         // 存储分区配置
};




#endif // HOOKSMANAGER_H
