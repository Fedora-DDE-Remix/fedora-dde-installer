#include "ProgressFailedFrame.h"
#include "CommonFunc.h"
#include "ShowLogWidget.h"
#include "SaveDeviceWidget.h"
#include "TitleWidget.h"
#include "globals.h"
#include "WarningDialog.h"
#include "ExportFileDialog.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QDebug>
#include <QStackedWidget>
#include <QApplication>
#include <QScreen>
#include <DMessageManager>

ProgressFailedFrame::ProgressFailedFrame(QWidget *parent)
     : QWidget(parent), m_saveToUsb(new SaveFileToUsb(this))
{
    initUI();
    initConnect();
}

void ProgressFailedFrame::showLog(const QString &content)
{
    if (m_showLogWidget != nullptr) {
        m_showLogWidget->showLog(content);
    }
}

void ProgressFailedFrame::showLogFile(const QString &filepath)
{
    if (m_showLogWidget != nullptr) {
        m_showLogWidget->showLogFile(filepath);
    }
}

void ProgressFailedFrame::showSaveLogSlot()
{
    // 点击保存按钮后显示日志界面，并更新标题和子标题文案
    m_saveLogWidget->setMaximumSize(CommonFunc::getShowSize().width() - 120, CommonFunc::getShowSize().height() - 120);
    m_saveLogWidget->setMinimumSize(CommonFunc::getShowSize().width() - 120, CommonFunc::getShowSize().height() - 120);
    m_saveLogWidget->exec();

    //提示弹窗
    QString deviceStr = m_saveLogWidget->getCurrentSelectDevice();
    if (!deviceStr.isEmpty()) {
        WarningDialog tipFrame(this);
        if (m_saveLogWidget->getSaveStatus()) {
            tipFrame.setCommentText(::QObject::tr("Successfully exported to the \"%1\" device").arg(deviceStr));
        } else {
            tipFrame.setCommentText(::QObject::tr("Export failed").arg(deviceStr));
        }
        tipFrame.setBtnText(::QObject::tr("Confirm", "button"));
        connect(&tipFrame, &WarningDialog::closeBtnSignal, &tipFrame, &WarningDialog::close);
        connect(&tipFrame, &WarningDialog::enterBtnSignal, &tipFrame, &WarningDialog::close);
        tipFrame.exec();
    }
}

void ProgressFailedFrame::installFailedSlot()
{
    qInfo() << "shutdown System !";
#ifdef QT_DEBUG
    QCoreApplication::exit();
#else
    CommonFunc::shutdownSystem();
#endif //QT_DEBUG
}

void ProgressFailedFrame::initUI()
{
    QLabel *installFailedIco = new QLabel;
    installFailedIco->setAlignment(Qt::AlignCenter);
    installFailedIco->setFixedSize(CommonFunc::getInstallResultIconSize());
    installFailedIco->setPixmap(CommonFunc::loadSvgImage(":/images/install_failed.svg", installFailedIco->width(), installFailedIco->height()));

    // 标题栏
    m_titleWidget = new TitleWidget(this);
    m_titleWidget->setTipsLabelWidth((CommonFunc::getShowSize().width() / 6) * 4);

    // ri zhi jie main
    m_saveToUsb->setSaveFileList(CommonFunc::getDirs(gSystemLogDir));
    m_saveLogWidget = new SaveDeviceWidget(this);
    m_saveLogWidget->saveList(QStringList() << gSystemLogDir);
    m_saveLogWidget->hide();

    m_showLogWidget = new ShowLogWidget;

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, CommonFunc::isBigScreen() ? glayoutTopMargin - 10 : 0, 0, 20);
    mainLayout->addWidget(installFailedIco, 0, Qt::AlignHCenter);
    mainLayout->addWidget(m_titleWidget, 0, Qt::AlignHCenter);
    mainLayout->addWidget(m_showLogWidget);
    this->setLayout(mainLayout);
}

void ProgressFailedFrame::initConnect()
{
    connect(m_showLogWidget, &ShowLogWidget::saveLogSignal, m_saveToUsb, &SaveFileToUsb::beginSaveFile);
    connect(m_showLogWidget, &ShowLogWidget::exitInstallSignal, this, &ProgressFailedFrame::installFailedSlot);
    connect(m_saveToUsb, &SaveFileToUsb::copyDone, [=](bool bSuccess, const QString &devId) {
        DMessageManager::instance()->setContentMargens(this, QMargins(0, 0, 0, 10));
        QIcon messageIcon = bSuccess ? QIcon::fromTheme("icon_ok") : QIcon::fromTheme("icon_fail");
        QString strMessage = bSuccess ? ::QObject::tr("Successfully exported to the \"%1\" device").arg(devId) :
                                        ::QObject::tr("Export failed");
        DMessageManager::instance()->sendMessage(this, messageIcon, strMessage);
    });
}

void ProgressFailedFrame::updateTs()
{
    m_titleWidget->setTitleLabelProperty(::QObject::tr("Installation Failed"), "");
    m_titleWidget->setTipsLabelProperty(::QObject::tr("Sorry for the trouble. Please take photos of the below error log,"
                                                      " or save it to an external disk, or scan the QR code, and send the log to us."
                                                      " We will help solve your issue."), "font-size:12px;font-weight:Medium;", true);
    m_saveLogWidget->setTitleText(::QObject::tr("Save Log"), "QLabel { font-size:24px; font-weight:bold; color:rgba(0,26,46,1); }");
    m_saveLogWidget->setTipsText(::QObject::tr("Save the error log to a removable device"), "");
    m_saveToUsb->setTitleAndMessage(tr("Save Log"), tr("Save the error log to a removable device"));
    m_saveToUsb->updateTs();

    m_titleWidget->adjustSize();
}

void ProgressFailedFrame::showEvent(QShowEvent *event)
{
    updateTs();
    QWidget::showEvent(event);
}

void ProgressFailedFrame::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}
