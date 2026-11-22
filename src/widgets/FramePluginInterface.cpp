#include "FramePluginInterface.h"
#include "CommonFunc.h"
#include "QuitInstallDialog.h"

#include <QApplication>
#include <QScreen>
#include <QWidget>
#include <DIconButton>
#include <QResizeEvent>
#include <QTranslator>
#include <DBlurEffectWidget>

DWIDGET_USE_NAMESPACE

//页面名称
extern const QString gFrameLanguage = "DeepinInstaller_Language";
extern const QString gFrameVerify = "DeepinInstaller_Verify";
extern const QString gFramePartition = "DeepinInstaller_Partition";
extern const QString gFrameUserInfo = "DeepinInstaller_UserInfo";
extern const QString gFramePrepare = "DeepinInstaller_Prepare";
extern const QString gFrameInstall = "DeepinInstaller_Install";
extern const QString gFrameConfigure = "FedoraDDEInstaller_Configure";
extern const QString gFrameApplication = "DeepinInstaller_Application";


FramePluginInterface::FramePluginInterface(QWidget *parent):
    QWidget(parent)
{
    DFrame *backGround = new DFrame;
    DStyle::setFrameRadius(backGround, 18);
//    DBlurEffectWidget *blur = new DBlurEffectWidget(this);
//    blur->setFull(true);
//    blur->setMaskColor(DBlurEffectWidget::LightColor);
//    blur->setBlurRectXRadius(18);
//    blur->setBlurRectYRadius(18);
    //title
    QWidget *titleWidget = new QWidget;
    titleWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    titleWidget->setFixedHeight(50);
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->setSpacing(0);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    initTitleButton();
    titleLayout->addSpacing(10);
    titleLayout->addWidget(m_backBnt, 0, Qt::AlignLeft | Qt::AlignVCenter);
    titleLayout->addSpacing(50);
    titleLayout->addStretch();
    titleLayout->addWidget(m_closeBnt, 0, Qt::AlignRight | Qt::AlignVCenter);
    titleWidget->setLayout(titleLayout);
    //content
    m_contentLayout = new QVBoxLayout;
    m_contentLayout->setSpacing(0);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    QWidget *content = new QWidget;
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    content->setLayout(m_contentLayout);
    //mainlayout
    QVBoxLayout *mainlayout = new QVBoxLayout;
    mainlayout->setContentsMargins(0, 0, 0, 0);
    mainlayout->setSpacing(0);
    mainlayout->addWidget(backGround);
    QVBoxLayout *centerLayout = new QVBoxLayout;
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);
    backGround->setLayout(centerLayout);
    //blur->setLayout(centerLayout);

    centerLayout->addWidget(titleWidget);
    centerLayout->addWidget(content);
    setLayout(mainlayout);
}

void FramePluginInterface::hideBackButton()
{
    m_backBnt->setVisible(false);
}

void FramePluginInterface::hideCloseButton()
{
    m_closeBnt->setVisible(false);
}

QString FramePluginInterface::frameName()
{
    return m_frameName;
}

void FramePluginInterface::enterEvent(QEnterEvent *event) {
    // 处理alt+tab切换后，点击主界面获得焦点问题
    if (!this->isActiveWindow()) {
        this->activateWindow();
    }
    return QWidget::enterEvent(event);
}

void FramePluginInterface::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->updateTs();
    }

    return QWidget::changeEvent(event);
}

void FramePluginInterface::showEvent(QShowEvent *event)
{
    updateTs();
    setFocus();
    QWidget::showEvent(event);
}

void FramePluginInterface::mousePressEvent(QMouseEvent *event) {
    // 处理弹窗关闭后，主界面获得焦点问题
    if (!this->isActiveWindow()) {
        this->activateWindow();
    }

    if (event->type() == QMouseEvent::MouseButtonPress) {
        auto focus = focusWidget();
        if (focus) {
            focus->clearFocus();
        }
    }

    return QWidget::mousePressEvent(event);
}

void FramePluginInterface::updateTs()
{
}

void FramePluginInterface::initTitleButton()
{
    //back button
    m_backBnt = new DIconButton(this);
    m_backBnt->setFocusPolicy(Qt::TabFocus);
    m_backBnt->setFixedSize(35, 35);
    m_backBnt->setIconSize(QSize(15, 15));
    m_backBnt->setIcon(QIcon::fromTheme("go-previous"));
    connect(m_backBnt, &DIconButton::clicked, this, &FramePluginInterface::back);
    //close button
    m_closeBnt = new DWindowCloseButton(this);
    m_closeBnt->setIconSize(QSize(50, 50));
    m_closeBnt->setFocusPolicy(Qt::TabFocus);
    connect(m_closeBnt, &QAbstractButton::clicked, this, &FramePluginInterface::quitInstaller);

}

void FramePluginInterface::quitInstaller()
{
    if (!CommonFunc::installWorkflow().canChangePartitions())
        return;
    QuitInstallDialog quitDialog;
    if (quitDialog.exec() == QDialog::Accepted)
        QCoreApplication::quit();
}
