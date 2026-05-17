#include "SetupTimezoneFrame.h"
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

SetupTimezoneFrame::SetupTimezoneFrame(QWidget *parent) :
    FramePluginInterface (parent)
{
    if (!CommonFunc::getSettingsString("DI_TIMEZONE").isEmpty()) {
        m_isConfSetByUser = true;
    }

    setWindowFlags(windowFlags() | Qt::X11BypassWindowManagerHint);

    // 标题栏
    m_title = new TitleWidget;

    m_timezoneMap = new TimezoneMap(this);
    connect(m_timezoneMap, &TimezoneMap::timezoneUpdated, [this] (const QString& timezone) {
        m_timezone = timezone;
        m_selectTimeZoneFrame->setTimezone(timezone);
    });

    m_selectTimeZoneFrame = new SelectTimeZoneFrame;
    connect(m_selectTimeZoneFrame, &SelectTimeZoneFrame::timezoneUpdated, [this] (const QString &timezone) {
        m_timezone = timezone;
        m_timezoneMap->setTimezone(timezone);
    });

    DIconButton* resetBtn = new DIconButton(this);
    resetBtn->setFixedSize(30, 30);
    resetBtn->setIconSize(QSize(30, 30));
    resetBtn->setIcon(QIcon(":/images/resetTimezone.svg"));
    connect(resetBtn, &DIconButton::clicked, this, [=] {
        this->onResetButtonClicked();
    });

    // 选择时区和设置时间控件
    QHBoxLayout* selectTimeZoneFrameLayout = new QHBoxLayout;
    selectTimeZoneFrameLayout->setContentsMargins(0, 0, 0, 0);
    selectTimeZoneFrameLayout->addStretch();
    selectTimeZoneFrameLayout->addWidget(m_selectTimeZoneFrame, 0, Qt::AlignCenter);
    selectTimeZoneFrameLayout->addSpacing(10);
    selectTimeZoneFrameLayout->addWidget(resetBtn, 0, Qt::AlignCenter);
    selectTimeZoneFrameLayout->addStretch();

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
    m_contentLayout->addWidget(m_timezoneMap, 0, Qt::AlignHCenter);
    m_contentLayout->addStretch();
    m_contentLayout->addLayout(selectTimeZoneFrameLayout);
    m_contentLayout->addSpacing(20);
    m_contentLayout->addLayout(btnLayout);
    m_contentLayout->addSpacing(CommonFunc::getInnerMargin());

    updateTs();

    qInfo() << "m_contentLayout" << m_contentLayout->count();

    connect(m_nextBnt, &QPushButton::clicked, [this] {
        writeConf();
        emit next();
    });
}

SetupTimezoneFrame::~SetupTimezoneFrame()
{
}

void SetupTimezoneFrame::onResetButtonClicked()
{
    if (!m_timezoneBak.isEmpty()) {
        m_timezoneMap->setTimezone(m_timezoneBak);
        m_selectTimeZoneFrame->setTimezone(m_timezoneBak);
    }
}

void SetupTimezoneFrame::updateTs()
{
    m_title->setTitleLabelProperty(::QObject::tr("Select Timezone"), "");
    m_title->setTipsLabelProperty(::QObject::tr("Is it the right timezone? You can change it as well"), "font-size:16px;font-weight:Medium;", true);
    m_nextBnt->setText(::QObject::tr("Next"));
}

void SetupTimezoneFrame::showEvent(QShowEvent *event)
{
    initTimezoneConf(CommonFunc::getSettingsString("DI_LOCALE"));
    m_timezoneBak = m_timezone;

    if (!m_timezone.isEmpty()) {
        m_timezoneMap->setTimezone(m_timezone);
        m_selectTimeZoneFrame->setTimezone(m_timezone);
    }

    updateTs();
    FramePluginInterface::showEvent(event);
}

void SetupTimezoneFrame::writeConf()
{
    if (!TimezoneManager::instance()->isTimezoneInTab(m_timezone)) {
        qWarning() << QString("Invalid timezone %1 in config").arg(m_timezone);
        return;
    }

    const ZoneInfoList zoneinfoList = TimezoneManager::instance()->getZoneInfoList();
    const SZoneInfo curZoneInfo = zoneinfoList[TimezoneManager::instance()->getZoneInfoIndexByTimezone(zoneinfoList, m_timezone)];
    QString localtime;
    if (curZoneInfo.country == "CN" && curZoneInfo.timezone != "Asia/Urumqi") {
        localtime = "Asia/Shanghai";
    } else {
        localtime = m_timezone;
    }

    CommonFunc::setSettingsValue("DI_TIMEZONE", m_timezone);
    CommonFunc::setSettingsValue("DI_LOCALTIME", localtime);
}

void SetupTimezoneFrame::initTimezoneConf(const QString &locale)
{
    // 根据语言设置更新时区设置数据
    m_timezone = CommonFunc::getSettingsString("DI_TIMEZONE");

    if (m_timezone.isEmpty() || !m_isConfSetByUser) {
        for (auto &it : CommonFunc::getLanguageList()) {
            if (it.locale == locale) {
                m_timezone = it.timezone;
                break;
            }
        }

        if (m_timezone.isEmpty()) {
            qWarning() << "Timezone is empty in all places";
            m_timezone = "Asia/Beijing";
        }
    }
}
