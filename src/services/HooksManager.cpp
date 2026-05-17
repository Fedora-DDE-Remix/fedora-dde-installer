#include "HooksManager.h"
#include "CommonFunc.h"
#include "globals.h"
#include "inner/HooksManagerPrivate.h"

#include <QThread>
#include <QDebug>
#include <QFile>
#include <QTimer>

HooksManager* HooksManager::m_instance = nullptr;
bool HooksManager::m_started = false;
HooksManager *HooksManager::instance(){
    if(m_instance == nullptr)
        m_instance = new HooksManager();
    return m_instance;
}

HooksManager::HooksManager():
    d(new HooksManagerPrivate)
{
}

void HooksManager::setWorkspaceDirs(const QStringList &lis)
{
    qInfo() << "hook list " << lis;

    // 再获取目录下hooks脚本进行处理
    for (QString key : lis) {
        SHooksInfo hook;
        hook.id = key;
        hook.hooks = CommonFunc::getDirs(key);
        d->m_workspacesHooks.append(hook);
    }
}

void HooksManager::startInstall()
{
    if (m_started) {
        return;
    }

    // 重置安装进度
    CommonFunc::setSettingsValue("DI_INSTALLER_STATUS", 0);

    QTimer *progressTimer = new QTimer;
    auto dataDevice = CommonFunc::getSettingsValue("DI_DATA_DEVICE_CONFIG");
    int timerValue = 10000;
    if (!CommonFunc::getSettingsValue("DI_DEVICE_LIST").toString().contains("nvme") || CommonFunc::isVirtualMachine()) {
        timerValue = 15000;
    }
    qInfo() << "timerValue: " << timerValue;
    progressTimer->setInterval(timerValue);
    
    connect(progressTimer, &QTimer::timeout, this, &HooksManager::updateProgress);
    progressTimer->start();

    QThread* installThread = new QThread;
    connect(installThread, &QThread::started, d, &HooksManagerPrivate::install);
    connect(installThread, &QThread::finished, installThread, &QThread::deleteLater);
    connect(d, &HooksManagerPrivate::update, this, &HooksManager::updateProgress);
    connect(d, &HooksManagerPrivate::finished, this, &HooksManager::finished);
    connect(d, &HooksManagerPrivate::exitInstall, this, &HooksManager::exitInstall);
    connect(d, &HooksManagerPrivate::finished, installThread, &QThread::quit);
    connect(d, &HooksManagerPrivate::exitInstall, installThread, &QThread::quit);
    connect(d, &HooksManagerPrivate::finished, progressTimer, &QTimer::stop);
    connect(d, &HooksManagerPrivate::exitInstall, progressTimer, &QTimer::stop);
    connect(d, &HooksManagerPrivate::finished, progressTimer, &QTimer::deleteLater);
    connect(d, &HooksManagerPrivate::exitInstall, progressTimer, &QTimer::deleteLater);
    
    d->moveToThread(installThread);  // 将对象move到线程中运行
    installThread->start();
    m_started = true;
    this->updateProgress();
}

QString HooksManager::getProgressStatus()
{
    return CommonFunc::getInstallStatus(CommonFunc::getSettingsInt("DI_INSTALLER_STATUS"));
}

void HooksManager::updateProgress()
{
    static int offset = 0;
    if (!d->m_workspacesHooks.isEmpty() && d->jobSize() > 0) {
        if (d->m_hooksStep > d->m_workspacesHooks.size()) {
            offset = 100;
            Q_EMIT progress(offset);
        } else {
            if (offset >= 100) {
                Q_EMIT progress(100);
            } else {
                offset++;
                int installStatus = CommonFunc::getSettingsInt("DI_INSTALLER_STATUS");
                offset = qMax(offset, installStatus);
                //qInfo() << "offset=" << offset;
                Q_EMIT progress(offset);
            }
        }
    } else {
        offset = 100;
        Q_EMIT progress(offset);
    }
}
