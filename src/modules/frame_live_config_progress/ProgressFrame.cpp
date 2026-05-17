#include "ProgressFrame.h"
#include "globals.h"
#include "CommonFunc.h"
#include "HooksManager.h"
#include "AnimationWidget.h"
#include "WarningDialog.h"

#include <QObject>
#include <QDebug>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>
#include <DIconButton>
#include <QLabel>
#include <QLabel>
#include <DBlurEffectWidget>

DWIDGET_USE_NAMESPACE

ProgressFream::ProgressFream(QWidget *parent):
    FramePluginInterface(parent)
{
    hideBackButton();
    hideCloseButton();

    QLabel *logoLabel = new QLabel(this);
    logoLabel->setFixedSize(CommonFunc::isBigScreen() ? QSize(350, 320) : QSize(240, 200));
    logoLabel->setPixmap(CommonFunc::loadSvgImage(":/images/config_progress.svg",logoLabel->width(), logoLabel->height()));
    logoLabel->setAlignment(Qt::AlignCenter);
    QHBoxLayout *logoLayout = new QHBoxLayout;
    logoLayout->setContentsMargins(0, 0, 0, 0);
    logoLayout->setSpacing(0);
    logoLayout->addStretch();
    logoLayout->addWidget(logoLabel);
    logoLayout->addStretch();

    // 标题提示信息
    m_title = new TitleWidget;
    m_title->setTitleLabelProperty(::QObject::tr("Configuring your computer"), "");

    // 进度显示
    QProgressBar *progressWidget = new QProgressBar(this);
    progressWidget->setFixedWidth(CommonFunc::isBigScreen() ? 500 : 380);
    progressWidget->setValue(0);
    QHBoxLayout *progressLayout = new QHBoxLayout;
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->setSpacing(0);
    progressLayout->addStretch();
    progressLayout->addWidget(progressWidget);
    progressLayout->addStretch();

    // 进度提示信息
    m_progLabel = new QLabel(::QObject::tr("It takes about a few minutes"), this);
    m_progLabel->setAlignment(Qt::AlignCenter);
    QHBoxLayout *progLayout = new QHBoxLayout;
    progLayout->setContentsMargins(0, 0, 0, 0);
    progLayout->setSpacing(0);
    progLayout->addStretch();
    progLayout->addWidget(m_progLabel);
    progLayout->addStretch();

    m_warningFrame = new WarningDialog;
    m_warningFrame->setCommentText(::QObject::tr("An unknown problem has occurred, which may not affect the system running. Please click \"Skip\" to continue the installation. If you cannot enter the system later, please try to reinstall."));
    m_warningFrame->setBtnText(::QObject::tr("Skip", "button"));
    m_warningFrame->setDialogIcon(QPixmap(":/images/interaction_warning.svg"));

    // 主布局
    m_contentLayout->addSpacing(CommonFunc::isBigScreen() ? 160 : 80);
    m_contentLayout->addLayout(logoLayout);
    m_contentLayout->addWidget(m_title);
    m_contentLayout->addStretch();
    m_contentLayout->addLayout(progressLayout);
    m_contentLayout->addSpacing(CommonFunc::isBigScreen() ? 20 : 10);
    m_contentLayout->addLayout(progLayout);
    m_contentLayout->addSpacing(50);

    connect(HooksManager::instance(), &HooksManager::progress, progressWidget, [=](int progressValue){
        if (CommonFunc::getSettingsInt("DI_INSTALLER_STATUS") == 100) {
            progressWidget->setValue(100);
        } else {
            progressWidget->setValue(progressValue);
        }
    });
    connect(HooksManager::instance(), &HooksManager::finished, this, &ProgressFream::finished);
    connect(HooksManager::instance(), &HooksManager::exitInstall, this, &ProgressFream::exitInstall);

    connect(m_warningFrame, &WarningDialog::enterBtnSignal, this, &ProgressFream::warnWidgetEnterBtnSlot);
}

void ProgressFream::showEvent(QShowEvent *event)
{
    this->startInstall();
    return FramePluginInterface::showEvent(event);
}

void ProgressFream::updateTs()
{
    m_title->setTitleLabelProperty(::QObject::tr("Configuring your computer"));
    m_progLabel->setText(::QObject::tr("It takes about a few minutes"));
    m_warningFrame->setCommentText(::QObject::tr("An unknown problem has occurred, which may not affect the system running. Please click \"Skip\" to continue the installation. If you cannot enter the system later, please try to reinstall."));
    m_warningFrame->setBtnText(::QObject::tr("Skip", "button"));
}

void ProgressFream::startInstall()
{
#ifdef QT_DEBUG
    QStringList testList = {gInstallDirTest + gInstallDirBeforeChroot, gInstallDirTest + gInstallDirInChroot, gInstallDirTest + gInstallDirAfterChroot};
    HooksManager::instance()->setWorkspaceDirs(testList);
#else
    HooksManager::instance()->setWorkspaceDirs(CommonFunc::getSettingsStringList("DI_LC_HOOKS_LIST"));
#endif //QT_DEBUG

    HooksManager::instance()->startInstall();
}

void ProgressFream::finished()
{
    qInfo() << "install finished.";
}

void ProgressFream::exitInstall(const QString &err)
{
    qCritical() << "exit install." << err;
    // 显示错误日志界面
    QPoint point = mapToGlobal(QPoint());
    m_warningFrame->move(point.x() + CommonFunc::getShowSize().width() / 2 - m_warningFrame->width() / 2,
                         point.y() + + CommonFunc::getShowSize().height() / 2 - m_warningFrame->height() / 2);
    m_warningFrame->show();
    m_warningFrame->raise();
}

void ProgressFream::warnWidgetEnterBtnSlot()
{
    CommonFunc::rebootSystem();
}
