//
// Created by uos on 2022/12/23.
//

#include "VerifyFrame.h"
#include "CommonFunc.h"
#include "TitleWidget.h"

#include <QTimer>
#include <mutex>

static std::once_flag verify_flag;
const int RESULT_ICON_WIDTH = 128;
const int RESULT_ICON_HIGH = 128;

VerifyFrame::VerifyFrame(QWidget *parent)
    : FramePluginInterface(parent)
{
    m_frameName = gFrameVerify;
    m_isoVerify = new ISOVerify;
    connect(m_isoVerify, &ISOVerify::verified, this, &VerifyFrame::onVerifyFinished);
    connect(m_isoVerify, &QThread::finished, m_isoVerify, &QThread::deleteLater);
    initUI();
    connect(m_nextBnt, &QPushButton::clicked, [=] {
       Q_EMIT next();
    });
}

bool VerifyFrame::load()
{
#ifdef QT_DEBUG
    return true;
#else
    if (QFile::exists(CommonFunc::getSettingsString("DI_DEEPIN_SQUASHFS_VERIFY"))) {
         return true;
    }
    return false;
#endif
}

void VerifyFrame::startVerify()
{
#ifdef QT_DEBUG
    QTimer::singleShot(0, this, &VerifyFrame::onStart);
#else
    if (QFile::exists(CommonFunc::getSettingsString("DI_DEEPIN_SQUASHFS_VERIFY"))) {
         QTimer::singleShot(0, this, &VerifyFrame::onStart);
    }
#endif
}

void VerifyFrame::onStart()
{
    m_spinner->start();
    m_isoVerify->start();
}

void VerifyFrame::onVerifyFinished(bool success)
{
    m_tipsVerifying->setVisible(false);
    m_spinner->setVisible(false);
    m_spinner->stop();
    if (success) {
        auto installMode = CommonFunc::getSettingsString("DI_INSTALL_MODE");
        //无人值守安装，自动下一步
        if (!installMode.isEmpty() && installMode.contains("auto")) {
            Q_EMIT next();
        }
        m_tipsVerifySuccess->setVisible(true);
        m_resultImage->setPixmap(CommonFunc::hidpiPixmap(":/images/success.svg", QSize(RESULT_ICON_WIDTH, RESULT_ICON_HIGH)));
    } else {
        m_tipsVerifyFailed->setVisible(true);
        m_resultImage->setPixmap(QIcon::fromTheme("icon_fail").pixmap(QSize(RESULT_ICON_WIDTH, RESULT_ICON_HIGH)));
    }
    m_nextBnt->setEnabled(success);
}

void VerifyFrame::showEvent(QShowEvent *event)
{
    std::call_once(verify_flag, std::bind(&VerifyFrame::startVerify, this));
    FramePluginInterface::showEvent(event);
}

void VerifyFrame::initUI()
{
    m_titleWidget = new TitleWidget;
    m_contentLayout->addWidget(m_titleWidget);
    m_contentLayout->addSpacing(CommonFunc::isBigScreen() ? 160 : 60);

    m_spinner = new DSpinner;
    m_spinner->setFixedSize(RESULT_ICON_WIDTH, RESULT_ICON_HIGH);
    m_contentLayout->addWidget(m_spinner, 0, Qt::AlignHCenter);

    m_resultImage = new QLabel;
    m_resultImage->setAlignment(Qt::AlignCenter);

    m_contentLayout->addWidget(m_resultImage, 0, Qt::AlignHCenter);
    m_tipsVerifying = new QLabel;
    m_tipsVerifying->setAlignment(Qt::AlignHCenter);
    m_contentLayout->addWidget(m_tipsVerifying);
    m_tipsVerifyFailed = new QLabel;
    m_tipsVerifyFailed->setVisible(false);
    m_tipsVerifyFailed->setAlignment(Qt::AlignHCenter);
    m_contentLayout->addWidget(m_tipsVerifyFailed, 0, Qt::AlignHCenter);
    m_tipsVerifySuccess = new QLabel;
    m_tipsVerifySuccess->setVisible(false);
    m_tipsVerifySuccess->setAlignment(Qt::AlignHCenter);
    m_contentLayout->addWidget(m_tipsVerifySuccess, 0, Qt::AlignHCenter);

    m_contentLayout->addStretch();
    m_nextBnt = new DSuggestButton;
    m_nextBnt->setFixedSize(gnextbuttonwidth, gnextbuttonheight);
    m_nextBnt->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_nextBnt->setEnabled(false);
    m_nextBnt->setDefault(true);
    m_contentLayout->addWidget(m_nextBnt, 0, Qt::AlignHCenter);

    m_contentLayout->addSpacing(CommonFunc::getInnerMargin());
}

void VerifyFrame::updateTs()
{
    m_titleWidget->setTitleLabelProperty(tr("Verify ISO"), "");
    m_titleWidget->setTipsLabelProperty(tr("To ensure system security, tampered ISO files will not be installed"),
                                        "font-size:16px;font-weight:Medium;");
    m_nextBnt->setText(::QObject::tr("Next"));
    m_tipsVerifying->setText(tr("Verifying image files..."));
    m_tipsVerifySuccess->setText(tr("Verification successful"));
    m_tipsVerifyFailed->setText(tr("Verification failed"));
}
