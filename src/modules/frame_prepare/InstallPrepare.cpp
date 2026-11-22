#include "InstallPrepare.h"
#include <QVBoxLayout>
#include <DFrame>
#include <QScrollBar>
#include <QTimer>
#include "CommonFunc.h"
#include "PartitionOperateManager.h"

const int PI_CENTER_WIDTH = 620;

InstallPrepare::InstallPrepare(QWidget *parent):FramePluginInterface (parent)
{
    m_frameName = gFramePrepare;
    initUI();
    initConnection();
}

void InstallPrepare::showEvent(QShowEvent *event)
{
    QString operInfo = PartitionOperateManager::instance()->getPartedOperatorInfo();
    if (operInfo.isEmpty()) {
        m_labelInfo->setText(::QObject::tr("Format the disk %1").arg(CommonFunc::getSettingsStringList("DI_DEVICE_LIST").join(";")));
    } else {
        m_labelInfo->setText(operInfo);
    }

    FramePluginInterface::showEvent(event);
}

void InstallPrepare::initUI()
{
    m_title = new TitleWidget;

    DFrame *bgFrame = new DFrame;
    bgFrame->setFixedWidth(PI_CENTER_WIDTH);
    bgFrame->setLineWidth(0);
    bgFrame->setBackgroundRole(DPalette::ItemBackground);
    QHBoxLayout *bgFrameLayout = new QHBoxLayout;
    bgFrameLayout->setContentsMargins(20, 20, 0, 0);
    bgFrame->setLayout(bgFrameLayout);

    m_labelInfo = new DLabel;
    m_labelInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_labelInfo->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_labelInfo->setWordWrap(true);
    bgFrame->setFixedWidth(PI_CENTER_WIDTH);
    bgFrameLayout->addWidget(m_labelInfo);

    QScrollArea *scrollCenter = new QScrollArea();
    scrollCenter->setFocusPolicy(Qt::NoFocus);
    scrollCenter->setFixedWidth(PI_CENTER_WIDTH);
    scrollCenter->setFrameShape(QFrame::NoFrame);
    scrollCenter->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    scrollCenter->setWidgetResizable(true);
    scrollCenter->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollCenter->setWidget(bgFrame);

    m_nvidiaDriver = new QCheckBox();
    m_nvidiaDriver->setFixedWidth(PI_CENTER_WIDTH);
    m_nvidiaDriver->setChecked(true);
    m_nvidiaDriver->hide();

    m_btnNext = new DSuggestButton;
    m_btnNext->setFixedSize(360, 36);
    m_btnNext->setDefault(true);

    m_contentLayout->addWidget(m_title);
    m_contentLayout->addSpacing(CommonFunc::isBigScreen() ? 40 : 20);
    m_contentLayout->addWidget(scrollCenter, 0, Qt::AlignHCenter);
    m_contentLayout->addSpacing(10);
    m_contentLayout->addWidget(m_nvidiaDriver, 0, Qt::AlignHCenter);
    m_contentLayout->addSpacing(20);
    m_contentLayout->addSpacing(CommonFunc::isBigScreen() ? 60 : 20);
    m_contentLayout->addWidget(m_btnNext, 0, Qt::AlignHCenter | Qt::AlignBottom);
    m_contentLayout->addSpacing(CommonFunc::getInnerMargin());
}

void InstallPrepare::initConnection()
{
    connect(m_btnNext, &QAbstractButton::clicked, this, [=]{
        Q_EMIT next();
    });
}

void InstallPrepare::updateTs()
{
    m_title->setTitleLabelProperty(::QObject::tr("Ready to Install"), "");
    m_title->setTipsLabelProperty(::QObject::tr("Make a backup of your important data and then continue"), "font-size:16px;font-weight:Medium;", true);
    m_btnNext->setText(tr("Start installation"));
    m_nvidiaDriver->setText(::QObject::tr("Install NVIDIA closed source driver"));
}