#include "HooksManagerPrivate.h"
#include "CommonFunc.h"

#include <QDebug>
#include <QDateTime>
#include <QThread>

//调试伦轮播图
#define DEBUG_PROGRESS_UPDATE_FRAME 0
void HooksManagerPrivate::install()
{
    qInfo() << "hooks manager start...";

    QString installLog;
    QDateTime start = QDateTime::currentDateTime();
    for (const SHooksInfo& hook: m_workspacesHooks) {
        for (const QString& hk : hook.hooks) {
            if (!CommonFunc::execHook(hk, installLog)) {
                Q_EMIT exitInstall(QString("%1 hooks run error.").arg(hk));
                return;
            } else {
                Q_EMIT this->update();
            }

        }
        m_hooksStep++;
        Q_EMIT this->update();
    }

#if DEBUG_PROGRESS_UPDATE_FRAME
    QThread::sleep(60);
#endif
    Q_EMIT finished();

    qInfo() << QString("hooks manager end! run time is: %1s").arg(start.secsTo(QDateTime::currentDateTime()));
}

int HooksManagerPrivate::jobSize()
{
    int size = 0;
    for (const SHooksInfo &hook: m_workspacesHooks) {
        size += hook.hooks.size();
    }

    return size;
}
