#include "SelectKBLayoutFrame.h"
#include "CommonFunc.h"
#include "globals.h"

#include <DApplication>
#include <DWidget>
#include <DListView>

#include <QShortcut>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QStandardItemModel>
#include <DBlurEffectWidget>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

SelectKBLayoutFrame::SelectKBLayoutFrame(QWidget *parent) :
    FramePluginInterface (parent)
{
    // 选择键盘布局标题
    m_title = new TitleWidget;

    // 选择键盘视图
    m_selectKBLayoutWidget = new SelectKBLayoutWidget;
    QHBoxLayout *selectLanguageWidgetLayout = new QHBoxLayout;
    selectLanguageWidgetLayout->setSpacing(0);
    selectLanguageWidgetLayout->setContentsMargins(0, 0, 0, 0);
    selectLanguageWidgetLayout->addWidget(m_selectKBLayoutWidget);
    
    // 下一步,返回按钮
    m_nextBnt = new DSuggestButton;
    m_nextBnt->setFixedSize(gnextbuttonwidth, gnextbuttonheight);
    m_nextBnt->setText(::QObject::tr("Next"));
    m_nextBnt->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_nextBnt->setEnabled(true);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(m_nextBnt);
    btnLayout->addStretch();

    // 主布局
    m_contentLayout->addWidget(m_title);
    m_contentLayout->addSpacing(15);
    m_contentLayout->addLayout(selectLanguageWidgetLayout);
    m_contentLayout->addSpacing(15);
    m_contentLayout->addSpacing(20);
    m_contentLayout->addLayout(btnLayout);
    m_contentLayout->addSpacing(CommonFunc::getInnerMargin());

    qInfo() << "m_contentLayout" << m_contentLayout->count();

    connect(m_nextBnt, &QPushButton::clicked, this, &SelectKBLayoutFrame::next);
}

SelectKBLayoutFrame::~SelectKBLayoutFrame()
{
}

void SelectKBLayoutFrame::updateTs()
{
    m_title->setTitleLabelProperty(QObject::tr("Select Keyboard Layout"), "");
    m_nextBnt->setText(::QObject::tr("Next"));
}
