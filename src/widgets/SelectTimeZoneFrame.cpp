#include "SelectTimeZoneFrame.h"
#include "CommonFunc.h"

#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QCollator>
#include <DIconButton>

DWIDGET_USE_NAMESPACE

SelectTimeZoneFrame::SelectTimeZoneFrame(QWidget *parent)
    : DFrame (parent)
{
    m_countryLabel = new QLabel;
    m_countryLabel->setFixedSize(30, 36);

    m_cityComboBox = new DComboBox(this);
    m_cityComboBox->setFixedSize(270, 36);
    connect(m_cityComboBox, &QComboBox::currentTextChanged,
            this, &SelectTimeZoneFrame::onCitySelectedChanged);

    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_countryLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_cityComboBox, 0, Qt::AlignLeft | Qt::AlignVCenter);

    setLayout(mainLayout);
    setContentsMargins(0, 0, 0, 0);

    setObjectName("SelectTimeZoneFrame");
    setStyleSheet("#SelectTimeZoneFrame {background-color: transparent;}");

    updateTs();
}

void SelectTimeZoneFrame::onCitySelectedChanged(const QString& text)
{
    if (!m_totalTimeZones.contains(text)) {
        qWarning() << QString("Current city combobox %1 is not exist in total map").arg(text);
    } else {
        emit timezoneUpdated(m_totalTimeZones.value(text));
    }
}

void SelectTimeZoneFrame::setTimezone(const QString &timezone)
{
    const QString locale(CommonFunc::getSettingsString("DI_LOCALE"));
    QStringList tms;
    QString localCity;

    TimezoneManager::instance()->getTotalTimezones(tms);
    QList<QPair<QString, QString>> localizedTms;

    for (const auto& it : tms) {
        QString key(TimezoneManager::instance()->getLocalTimezone(it, locale).second);
        localizedTms << QPair<QString, QString>(key, it);

        if (it == timezone) {
            localCity = key;
        }
    }

    // 按照locale排序

    QCollator collator{QLocale(locale)};
    collator.setCaseSensitivity(Qt::CaseInsensitive);

    std::sort(localizedTms.begin(), localizedTms.end()
        , [&](const QPair<QString, QString>& a, const QPair<QString, QString>& b) {
            return collator.compare(a.first, b.first) < 0;
        }
    );

    m_totalTimeZones.clear();
    for (const auto& it : localizedTms) {
        m_totalTimeZones.insert(it.first, it.second);
    }

    disconnect(m_cityComboBox, nullptr, this, nullptr);
    m_cityComboBox->clear();
    m_cityComboBox->addItems(m_totalTimeZones.keys());
    m_cityComboBox->setCurrentText(localCity);
    connect(m_cityComboBox, &QComboBox::currentTextChanged,
            this, &SelectTimeZoneFrame::onCitySelectedChanged);
}

void SelectTimeZoneFrame::updateTs()
{
    m_countryLabel->setText(::QObject::tr("City"));
}

void SelectTimeZoneFrame::showEvent(QShowEvent* event)
{
    updateTs();
    DFrame::showEvent(event);
}
