#include "ProgressFrame.h"
#include "CommonFunc.h"
#include "HooksManager.h"

#include <QApplication>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

ProgressFrame::ProgressFrame(QWidget *parent)
    : FramePluginInterface(parent)
{
    m_frameName = gFrameConfigure;
    hideBackButton();
    hideCloseButton();

    m_logoLabel = new QLabel(this);
    m_logoLabel->setFixedSize(160, 160);
    m_logoLabel->setPixmap(QPixmap(CommonFunc::getSystemLogo()).scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel = new QLabel(this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-size:22px;font-weight:bold;");
    m_titleLabel->setWordWrap(true);
    m_progLabel = new QLabel(this);
    m_progLabel->setAlignment(Qt::AlignCenter);
    m_progLabel->setTextFormat(Qt::PlainText);
    m_progLabel->setWordWrap(true);
    m_progLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_progressWidget = new QProgressBar(this);
    m_progressWidget->setRange(0, 100);
    m_progressWidget->setValue(0);
    m_progressWidget->setFixedWidth(CommonFunc::isBigScreen() ? 500 : 380);
    m_actionButton = new QPushButton(this);
    m_actionButton->setMinimumSize(220, 36);
    m_secondaryButton = new QPushButton(this);
    m_secondaryButton->setMinimumSize(220, 36);
    auto *actions = new QHBoxLayout;
    actions->addStretch();
    actions->addWidget(m_secondaryButton);
    actions->addWidget(m_actionButton);
    actions->addStretch();

    m_contentLayout->addStretch();
    m_contentLayout->addWidget(m_logoLabel, 0, Qt::AlignHCenter);
    m_contentLayout->addSpacing(20);
    m_contentLayout->addWidget(m_titleLabel);
    m_contentLayout->addSpacing(15);
    m_contentLayout->addWidget(m_progLabel);
    m_contentLayout->addSpacing(20);
    m_contentLayout->addWidget(m_progressWidget, 0, Qt::AlignHCenter);
    m_contentLayout->addStretch();
    m_contentLayout->addLayout(actions);
    m_contentLayout->addSpacing(CommonFunc::getInnerMargin());
    m_actionButton->hide();
    m_secondaryButton->hide();

    connect(HooksManager::instance(), &HooksManager::progress, this, [this](int value) {
        if (m_running)
            m_progressWidget->setValue(value);
    });
    connect(HooksManager::instance(), &HooksManager::finished, this, &ProgressFrame::finished);
    connect(HooksManager::instance(), &HooksManager::exitInstall, this, &ProgressFrame::exitInstall);
    connect(m_actionButton, &QPushButton::clicked, this, &ProgressFrame::actionClicked);
    connect(m_secondaryButton, &QPushButton::clicked, this, [this] {
        if (m_complete) {
            QApplication::quit();
        } else if (!m_running && CommonFunc::installWorkflow().canEditUser()) {
            Q_EMIT back();
        }
    });
}

void ProgressFrame::showEvent(QShowEvent *event)
{
    FramePluginInterface::showEvent(event);
    if (!m_running && !m_complete)
        QTimer::singleShot(0, this, &ProgressFrame::startInstall);
}

void ProgressFrame::updateTs()
{
    if (m_complete) {
        m_titleLabel->setText(tr("Fedora DDE Remix is ready"));
        m_progLabel->setText(tr("Your account and system settings have been saved. You can restart now or continue using the live desktop."));
        m_actionButton->setText(tr("Restart Now"));
        m_secondaryButton->setText(tr("Continue Using Live Desktop"));
    } else if (!m_error.isEmpty()) {
        m_titleLabel->setText(tr("Configuration could not be completed"));
        m_progLabel->setText((m_accountSaved
            ? tr("Your account has been saved. Retry to finish safely unmounting the installed system.")
            : tr("The installed system is still mounted. Correct the problem and retry; the disk will not be formatted again.")) + "\n\n" + m_error);
        m_actionButton->setText(tr("Retry Configuration"));
        m_secondaryButton->setText(tr("Edit Settings"));
    } else {
        m_titleLabel->setText(tr("Saving your account and system settings"));
        m_progLabel->setText(tr("Configuring the installed system and finishing the installation. Keep this window open."));
    }
    m_progressWidget->setFormat(tr("Configuration progress") + " %v%");
}

void ProgressFrame::startInstall()
{
    if (m_running || m_complete)
        return;
    if (!CommonFunc::installWorkflow().beginConfiguration()) {
        m_error = tr("There is no completed installation to configure in this live session.");
        updateTs();
        return;
    }
    m_running = true;
    m_error.clear();
    m_progressWidget->show();
    m_progressWidget->setValue(0);
    m_actionButton->hide();
    m_secondaryButton->hide();
    updateTs();
    // Backend validates /run state and mounted target before any account write.
    HooksManager::instance()->setWorkspaceDirs(CommonFunc::getSettingsStringList("DI_FB_HOOHS_LIST"));
    HooksManager::instance()->startInstall();
}

void ProgressFrame::finished()
{
    if (!m_running || !CommonFunc::installWorkflow().configurationSucceeded())
        return;
    m_running = false;
    m_complete = true;
    m_progressWidget->setValue(100);
    m_actionButton->show();
    m_secondaryButton->show();
    updateTs();
    // Deliberately no automatic reboot, even if a legacy setting requests it.
}

void ProgressFrame::exitInstall(const QString &error)
{
    if (!m_running)
        return;
    m_running = false;
    CommonFunc::installWorkflow().configurationFailed();
    m_error = error.isEmpty() ? tr("See the installation log for details.") : error;
    QFile stateFile("/run/fedora-dde-installer/state.json");
    if (stateFile.open(QIODevice::ReadOnly)) {
        const QString phase = QJsonDocument::fromJson(stateFile.readAll()).object().value("phase").toString();
        m_accountSaved = phase == "user-configured" || phase == "finalized";
    }
    m_actionButton->show();
    m_secondaryButton->setVisible(!m_accountSaved);
    updateTs();
}

void ProgressFrame::actionClicked()
{
    if (m_complete && CommonFunc::installWorkflow().canRestart()) {
        // Restart is only reachable through this explicit user action, after
        // the backend has configured the account and finalized the target.
        if (!CommonFunc::rebootSystem())
            m_progLabel->setText(tr("The system is ready, but the restart request failed. You can restart from the live desktop."));
    } else if (!m_running && CommonFunc::installWorkflow().canEditUser()) {
        startInstall();
    }
}
