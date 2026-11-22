#include "ProgressFrame.h"
#include "ProgressUpdateFrame.h"
#include "ProgressFailedFrame.h"
#include "globals.h"
#include "CommonFunc.h"
#include "HooksManager.h"
#include "PartitionOperateManager.h"

#include <QProgressBar>
#include <QStackedLayout>
#include <QDateTime>
#include <QTimer>

ProgressFrame::ProgressFrame(QWidget *parent):
    FramePluginInterface(parent)
{
    m_frameName = gFrameInstall;
    hideBackButton();
    hideCloseButton();
    m_progressUpdateWidget = new ProgressUpdateFrame(nullptr);

    m_progressFailedWidget = new ProgressFailedFrame;

    // 主布局
    m_mainLayout = new QStackedLayout;
    m_mainLayout->setSpacing(0);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->addWidget(m_progressUpdateWidget);
    m_mainLayout->addWidget(m_progressFailedWidget);
    m_mainLayout->setCurrentWidget(m_progressUpdateWidget);
    m_contentLayout->addLayout(m_mainLayout);

    connect(HooksManager::instance(), &HooksManager::progress, [=](int value){
        if (!m_running)
            return;
        m_progressUpdateWidget->progressWidget()->setValue(value);
        m_progressUpdateWidget->setProgressStatus(value == 100 ? CommonFunc::getInstallStatus(value) : HooksManager::instance()->getProgressStatus());
    });
    connect(HooksManager::instance(), &HooksManager::finished, this, &ProgressFrame::finished);
    connect(HooksManager::instance(), &HooksManager::exitInstall, this, &ProgressFrame::exitInstall);
}

void ProgressFrame::showEvent(QShowEvent *event)
{
    FramePluginInterface::showEvent(event);
    this->startInstall();
    Q_EMIT m_progressUpdateWidget->startAnime();
}

void ProgressFrame::updateTs()
{
    m_progressUpdateWidget->updateTS();
}

void ProgressFrame::startInstall()
{
    if (!CommonFunc::installWorkflow().beginInstall())
        return;
    m_running = true;
    QString installErr;
    // 将分区操作的配置写入到配置文件中
    if (!PartitionOperateManager::instance()->writeConf(installErr)) {
        CommonFunc::installWorkflow().installationFailed();
        m_running = false;
        // 显示错误日志界面
        m_progressFailedWidget->showLog(installErr);
        m_mainLayout->setCurrentWidget(m_progressFailedWidget);
        return;
    }

    HooksManager::instance()->setWorkspaceDirs(CommonFunc::getSettingsStringList("DI_HOOKS_LIST"));

    HooksManager::instance()->startInstall();

    QString curTime = QDateTime::fromMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()).toString("yyyy-MM-dd hh:mm:ss");
    CommonFunc::setSettingsValue("DI_RECORD_INSTALL_START", curTime);
}

void ProgressFrame::finished()
{
    if (!m_running || !CommonFunc::installWorkflow().installationSucceeded())
        return;
    m_running = false;
    CommonFunc::setSettingsValue("DI_RECORD_INSTALL_STOP", QDateTime::currentDateTime().toString(Qt::ISODate));
    qInfo() << "System installed; continuing account setup in this live session.";
    // The backend keeps /target mounted until configuration and finalization.
    Q_EMIT next();
}

void ProgressFrame::exitInstall(const QString &err)
{
    if (!m_running)
        return;
    m_running = false;
    CommonFunc::installWorkflow().installationFailed();
    QString curTime = QDateTime::fromMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()).toString("yyyy-MM-dd hh-mm-ss");
    CommonFunc::setSettingsValue("DI_RECORD_INSTALL_STOP", curTime);

    qCritical() << "exit install." << err;
    // 显示错误日志界面
    m_progressFailedWidget->showLogFile("/var/log/fedora-dde-installer/fedora-dde-installer.log");
    m_mainLayout->setCurrentWidget(m_progressFailedWidget);
}
