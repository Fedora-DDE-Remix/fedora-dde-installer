#include "TimezoneFrame.h"
#include "CommonFunc.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>

const int kHorizontalMargin = 10;
const int kVerticalMargin = 10;

// 这里时区的修改规则为：如果时区数据（来自配置文件或者用户在时区界面操作）为空，那么时区跟随语言联动。
// 用户在时区界面点击OK操作，那么修改时区数据。
bool TimezoneFrame::m_isConfSetByUser = false;

bool TimezoneFrame::isTimezoneSetByUser()
{
    return m_isConfSetByUser;
}

TimezoneFrame::TimezoneFrame(QWidget *parent)
    : DDialog(parent), m_titleLabel(new DLabel), m_msgLabel(new DTipLabel)
{
    setOnButtonClickedClose(false);
    if (!CommonFunc::getSettingsString("DI_TIMEZONE").isEmpty()) {
        m_isConfSetByUser = true;
    }

    m_aliasMap = TimezoneManager::instance()->getTimezoneAliasMap();

    m_timezoneMap = new TimezoneMap(this);
    connect(m_timezoneMap, &TimezoneMap::timezoneUpdated,
            this, &TimezoneFrame::onTimezoneMapUpdated);

    m_selectTimeZoneFrame = new SelectTimeZoneFrame;
    connect(m_selectTimeZoneFrame, &SelectTimeZoneFrame::timezoneUpdated
            , this, &TimezoneFrame::onSelectTimezoneUpdated);

    DIconButton* resetBtn = new DIconButton(this);
    resetBtn->setFixedSize(30, 30);
    resetBtn->setIconSize(QSize(30, 30));
    resetBtn->setIcon(QIcon(":/images/resetTimezone.svg"));
    connect(resetBtn, &DIconButton::clicked, this, [=] {
        this->onResetButtonClicked();
    });

    // 选择时区和设置时间控件
    QHBoxLayout* bottomLayout = new QHBoxLayout;
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->addWidget(m_selectTimeZoneFrame, 0, Qt::AlignLeft | Qt::AlignVCenter);
    bottomLayout->addSpacing(10);
    bottomLayout->addWidget(resetBtn, 0, Qt::AlignLeft | Qt::AlignVCenter);
    bottomLayout->addStretch();

    DFontSizeManager::instance()->bind(m_titleLabel, DFontSizeManager::T3);
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_titleLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(m_msgLabel, 0, Qt::AlignCenter);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_timezoneMap, 0, Qt::AlignHCenter);
    mainLayout->addStretch();
    mainLayout->addLayout(bottomLayout);

    QWidget *widget = new QWidget;
    widget->setLayout(mainLayout);

    addContent(widget);
    addButton(::QObject::tr("Cancel", "button"));
    addButton(::QObject::tr("Confirm", "button"), true, DDialog::ButtonRecommend);
    m_cancelBnt = getButton(0);
    m_okBnt = getButton(1);
    connect(m_cancelBnt, &QAbstractButton::clicked, [=] { this->hide(); });
    connect(m_okBnt, &QPushButton::clicked, this, [=] {
        m_isConfSetByUser = true;
        this->writeConf();
        this->hide();
    });

    setFocusPolicy(Qt::FocusPolicy::NoFocus);

    QSize sz = CommonFunc::getShowSize();
    setFixedSize(sz.width() - kHorizontalMargin * 2, sz.height() - kVerticalMargin * 2);

    updateTs();
}

void TimezoneFrame::updateTs()
{
    m_titleLabel->setText(::QObject::tr("Select Timezone"));
    m_msgLabel->setText(::QObject::tr("Is it the right timezone? You can change it as well"));

    m_okBnt->setText(::QObject::tr("Confirm", "button"));
    m_cancelBnt->setText(::QObject::tr("Cancel", "button"));
}

void TimezoneFrame::readConf()
{
    QString tm = CommonFunc::getSettingsString("DI_TIMEZONE");

    // TODO: 别名的事儿需仔细调研一下，暂不处理
    //    timezone = getTimezoneAlias(timezone);

    if (!TimezoneManager::instance()->isTimezoneInTab(tm)) {
        qWarning() << QString("Invalid timezone %1 in config").arg(tm);

        m_timezone.clear();
        return;
    }

    m_timezone = tm;
}

void TimezoneFrame::writeConf()
{
    if (!TimezoneManager::instance()->isTimezoneInTab(m_timezone)) {
        qWarning() << QString("Invalid timezone %1 in config").arg(m_timezone);
        return;
    }

    const ZoneInfoList zoneinfoList = TimezoneManager::instance()->getZoneInfoList();
    const SZoneInfo curZoneInfo = zoneinfoList[TimezoneManager::instance()->getZoneInfoIndexByTimezone(zoneinfoList, m_timezone)];
    if (curZoneInfo.country == "CN" && curZoneInfo.timezone != "Asia/Urumqi") {
        m_timezone = "Asia/Shanghai";
    }

    CommonFunc::setSettingsValue("DI_TIMEZONE", m_timezone);
}

void TimezoneFrame::showEvent(QShowEvent* event)
{
    readConf();
    m_timezoneBak = m_timezone;

    if (!m_timezone.isEmpty()) {
        m_timezoneMap->setTimezone(m_timezone);
        m_selectTimeZoneFrame->setTimezone(m_timezone);
    }

    updateTs();

    DDialog::showEvent(event);
}

QString TimezoneFrame::getTimezoneAlias(const QString& timezone)
{
    // 如果时区没有别名，返回自身
    return m_aliasMap.value(timezone, timezone);
}

void TimezoneFrame::onTimezoneMapUpdated(const QString& timezone)
{
    m_timezone = timezone;
    m_selectTimeZoneFrame->setTimezone(timezone);
}

void TimezoneFrame::onResetButtonClicked()
{
    m_timezoneMap->setTimezone("Asia/Shanghai");
    m_selectTimeZoneFrame->setTimezone("Asia/Shanghai");
}

void TimezoneFrame::onSelectTimezoneUpdated(const QString &timezone)
{
    m_timezone = timezone;
    m_timezoneMap->setTimezone(timezone);
}
