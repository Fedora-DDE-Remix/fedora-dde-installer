#include "HooksManager.h"
#include "CommonFunc.h"
#include "globals.h"
#include "inner/HooksManagerPrivate.h"

#include <QThread>
#include <QDebug>
#include <QTimer>

HooksManager *HooksManager::m_instance = nullptr;
HooksManager *HooksManager::instance()
{
    if (!m_instance)
        m_instance = new HooksManager;
    return m_instance;
}

HooksManager::HooksManager() = default;

void HooksManager::setWorkspaceDirs(const QStringList &directories)
{
    if (m_started) {
        qWarning() << "Cannot replace an active installation stage";
        return;
    }
    m_directories = directories;
}

void HooksManager::startInstall()
{
    if (m_started)
        return;

    auto *worker = new HooksManagerPrivate;
    for (const auto &directory : m_directories) {
        SHooksInfo stage;
        stage.id = directory;
        stage.hooks = CommonFunc::getDirs(directory, {"*.job"});
        if (stage.hooks.isEmpty()) {
            delete worker;
            Q_EMIT exitInstall(tr("No installation hooks were found in %1").arg(directory));
            return;
        }
        worker->m_workspacesHooks.append(stage);
    }
    if (worker->jobSize() == 0) {
        delete worker;
        Q_EMIT exitInstall(tr("No installation stage is configured"));
        return;
    }

    m_started = true;
    m_succeeded = false;
    m_error.clear();
    m_progress = 0;
    CommonFunc::setSettingsValue("DI_INSTALLER_STATUS", 0);
    auto *thread = new QThread(this);
    auto *timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, &HooksManager::updateProgress);
    connect(worker, &HooksManagerPrivate::update, this, &HooksManager::updateProgress);
    connect(thread, &QThread::started, worker, &HooksManagerPrivate::install);
    connect(worker, &HooksManagerPrivate::finished, this, [this, thread] {
        m_succeeded = true;
        thread->quit();
    });
    connect(worker, &HooksManagerPrivate::exitInstall, this, [this, thread](const QString &error) {
        m_error = error;
        thread->quit();
    });
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, timer, thread] {
        timer->stop();
        timer->deleteLater();
        thread->deleteLater();
        // Reset before notifying the GUI. Its next page may immediately start
        // the account-configuration stage or retry a failed configuration.
        m_started = false;
        if (m_succeeded) {
            m_progress = 100;
            Q_EMIT progress(100);
            Q_EMIT finished();
        } else {
            Q_EMIT exitInstall(m_error);
        }
    });
    worker->moveToThread(thread);
    timer->start();
    thread->start();
    Q_EMIT progress(0);
}

QString HooksManager::getProgressStatus()
{
    return CommonFunc::getInstallStatus(CommonFunc::getSettingsInt("DI_INSTALLER_STATUS"));
}

void HooksManager::updateProgress()
{
    if (!m_started)
        return;
    // Never claim completion from a timer; only successful hooks finish a stage.
    m_progress = qBound(0, CommonFunc::getSettingsInt("DI_INSTALLER_STATUS"), 99);
    Q_EMIT progress(m_progress);
}
