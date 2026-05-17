#include "DateTimeFrame.h"
#include "CommonFunc.h"

#include <QCheckBox>
#include <QChar>
#include <QRegularExpressionValidator>
#include <QDebug>
#include <functional>

const int kSystemDateTimeFrameWidth = 691;

const int kMonthDayHourMinuteQLineEditWidth = 40;
const int kDateTimeLabelWidth = 30;
const int kYearQLineEditWidth = 55;

const int kMaxIncrementYear = 30;

const int kDateTimeItemInnerSpacing = 1;
const int kDateTimeItemExternalSpacing = 2;

TimeDateLineEdit::TimeDateLineEdit(QWidget* parent)
    : QLineEdit (parent)
{
}

void TimeDateLineEdit::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    QLineEdit::focusOutEvent(event);
    emit lostFocus();
}

DateTimeFrame::DateTimeFrame(QWidget *parent)
    : QWidget(parent)
    , m_maxYear(QDateTime::currentDateTime().date().year() + kMaxIncrementYear)
{
    m_setDateTimeCheckBox = new DCheckBox;
    m_setDateTimeCheckBox->setCheckable(true);
    m_setDateTimeCheckBox->setChecked(false);

    m_hourLabel = new QLabel;
    m_hourLabel->setFixedSize(kDateTimeLabelWidth, 36);

    m_hourEdit = new TimeDateLineEdit;
    m_hourEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{1,2}")));
    m_hourEdit->setFixedSize(kMonthDayHourMinuteQLineEditWidth, 36);
    m_hourEdit->setAlignment(Qt::AlignHCenter);
    m_hourEdit->setContextMenuPolicy(Qt::NoContextMenu);

    connect(m_hourEdit, &QLineEdit::editingFinished, this
            , &DateTimeFrame::onHourEditingFinished);
    connect(m_hourEdit, &TimeDateLineEdit::lostFocus, this
            , &DateTimeFrame::onHourEditingFinished);
    connect(m_hourEdit, &QLineEdit::textChanged, this
            , &DateTimeFrame::checkOkButtonEnable);

    m_minuteLabel = new QLabel;
    m_minuteLabel->setFixedSize(kDateTimeLabelWidth, 36);

    m_minuteEdit = new TimeDateLineEdit;
    m_minuteEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{1,2}")));
    m_minuteEdit->setFixedSize(kMonthDayHourMinuteQLineEditWidth, 36);
    m_minuteEdit->setAlignment(Qt::AlignHCenter);
    m_minuteEdit->setContextMenuPolicy(Qt::NoContextMenu);

    connect(m_minuteEdit, &QLineEdit::editingFinished, this
            , &DateTimeFrame::onMinuteEditingFinished);
    connect(m_minuteEdit, &TimeDateLineEdit::lostFocus, this
            , &DateTimeFrame::onMinuteEditingFinished);
    connect(m_minuteEdit, &QLineEdit::textChanged, this
            , &DateTimeFrame::checkOkButtonEnable);

    m_yearLabel = new QLabel;
    m_yearLabel->setFixedSize(kDateTimeLabelWidth, 36);

    m_yearEdit = new TimeDateLineEdit;
    m_yearEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{1,4}")));
    m_yearEdit->setFixedSize(kYearQLineEditWidth, 36);
    m_yearEdit->setAlignment(Qt::AlignHCenter);
    m_yearEdit->setContextMenuPolicy(Qt::NoContextMenu);

    connect(m_yearEdit, &QLineEdit::editingFinished, this
            , &DateTimeFrame::onYearEditingFinished);
    connect(m_yearEdit, &TimeDateLineEdit::lostFocus, this,
            &DateTimeFrame::onYearEditingFinished);
    connect(m_yearEdit, &QLineEdit::textChanged, this
            , &DateTimeFrame::checkOkButtonEnable);

    m_monthLabel = new QLabel;
    m_monthLabel->setFixedSize(kDateTimeLabelWidth, 36);

    m_monthEdit = new TimeDateLineEdit;
    m_monthEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{1,2}")));
    m_monthEdit->setAlignment(Qt::AlignHCenter);
    m_monthEdit->setFixedSize(kMonthDayHourMinuteQLineEditWidth, 36);
    m_monthEdit->setContextMenuPolicy(Qt::NoContextMenu);

    connect(m_monthEdit, &QLineEdit::editingFinished, this
            , &DateTimeFrame::onMonthEditingFinished);
    connect(m_monthEdit, &TimeDateLineEdit::lostFocus, this
            , &DateTimeFrame::onMonthEditingFinished);
    connect(m_monthEdit, &QLineEdit::textChanged, this
            , &DateTimeFrame::checkOkButtonEnable);

    m_dayLabel = new QLabel;
    m_dayLabel->setFixedSize(kDateTimeLabelWidth, 36);

    m_dayEdit = new TimeDateLineEdit;
    m_dayEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{1,2}")));
    m_dayEdit->setAlignment(Qt::AlignHCenter);
    m_dayEdit->setFixedSize(kMonthDayHourMinuteQLineEditWidth, 36);
    m_dayEdit->setContextMenuPolicy(Qt::NoContextMenu);

    connect(m_dayEdit, &QLineEdit::editingFinished, this
            , &DateTimeFrame::onDayEditingFinished);
    connect(m_dayEdit, &TimeDateLineEdit::lostFocus, this
            , &DateTimeFrame::onDayEditingFinished);
    connect(m_dayEdit, &QLineEdit::textChanged, this
            , &DateTimeFrame::checkOkButtonEnable);

    connect(m_setDateTimeCheckBox, &DCheckBox::checkStateChanged, this, [=] {
        bool enble = m_setDateTimeCheckBox->isChecked();

        m_hourEdit->setEnabled(enble);
        m_minuteEdit->setEnabled(enble);
        m_yearEdit->setEnabled(enble);
        m_monthEdit->setEnabled(enble);
        m_dayEdit->setEnabled(enble);

        // 如果不需要手工设置时间，那么OK按钮永远使能
        if (!enble) {
            emit this->okButtonEnable(true);
        }
    });

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(m_setDateTimeCheckBox, 0, Qt::AlignLeft);
    mainLayout->addStretch();
    mainLayout->addWidget(m_yearEdit);
    mainLayout->addSpacing(kDateTimeItemInnerSpacing);
    mainLayout->addWidget(m_yearLabel);
    mainLayout->addSpacing(kDateTimeItemExternalSpacing);
    mainLayout->addWidget(m_monthEdit);
    mainLayout->addSpacing(kDateTimeItemInnerSpacing);
    mainLayout->addWidget(m_monthLabel);
    mainLayout->addSpacing(kDateTimeItemExternalSpacing);
    mainLayout->addWidget(m_dayEdit);
    mainLayout->addSpacing(kDateTimeItemInnerSpacing);
    mainLayout->addWidget(m_dayLabel);
    mainLayout->addSpacing(kDateTimeItemExternalSpacing);
    mainLayout->addWidget(m_hourEdit);
    mainLayout->addSpacing(kDateTimeItemInnerSpacing);
    mainLayout->addWidget(m_hourLabel);
    mainLayout->addSpacing(kDateTimeItemExternalSpacing);
    mainLayout->addWidget(m_minuteEdit);
    mainLayout->addSpacing(kDateTimeItemInnerSpacing);
    mainLayout->addWidget(m_minuteLabel);

    setLayout(mainLayout);
    setMaximumWidth(kSystemDateTimeFrameWidth);
    setMinimumHeight(36);

    updateDateTimeData();

    updateTs();
}

void DateTimeFrame::updateData(const bool &enableNTP, const QString &dateTime)
{
    m_setDateTimeCheckBox->setChecked(!enableNTP);

    QString format("yyyy-MM-dd hh:mm:ss");
    QDateTime dt = QDateTime::fromString(dateTime, format);

    if (!dt.isValid()) {
        updateDateTimeData();
        return;
    }

    updateDateTimeData(dt);
}

// TODO:这里采用QDateTime的标准格式化统一一下
QString DateTimeFrame::timedate() const
{
    return QString("%1-%2-%3 %4:%5:%6")
           .arg(m_yearEdit->text(), 4, '0')
           .arg(m_monthEdit->text(), 2, '0')
           .arg(m_dayEdit->text(), 2, '0')
           .arg(m_hourEdit->text(), 2, '0')
           .arg(m_minuteEdit->text(), 2, '0')
           .arg("0", 2, '0');
}

bool DateTimeFrame::isEnabled() const
{
    return m_setDateTimeCheckBox->isChecked();
}

void DateTimeFrame::showEvent(QShowEvent *event)
{
    updateTs();

    QWidget::showEvent(event);
}

void DateTimeFrame::onYearEditingFinished()
{
    if (!validateYear(m_yearEdit->text())) {
        m_yearEdit->setText(QString::number(QDate::currentDate().year()));
    }

    autoAdjustDay();
}

void DateTimeFrame::onMonthEditingFinished()
{
    if (!validateMonth(m_monthEdit->text())) {
        m_monthEdit->setText(QString::number(QDate::currentDate().month()));
    }

    autoAdjustDay();
}

void DateTimeFrame::onDayEditingFinished()
{
    if (!validateDay(m_dayEdit->text())) {
        m_dayEdit->setText(QString::number(QDate::currentDate().day()));
    }
}

void DateTimeFrame::onHourEditingFinished()
{
    if (!validateHour(m_hourEdit->text())) {
        m_hourEdit->setText(QString::number(QTime::currentTime().hour()));
        return;
    }
}

void DateTimeFrame::onMinuteEditingFinished()
{
    if (!validateMinute(m_minuteEdit->text())) {
        m_minuteEdit->setText(QString::number(QTime::currentTime().minute()));
    }
}

void DateTimeFrame::autoAdjustDay()
{
    onDayEditingFinished();
}

bool DateTimeFrame::validateHour(const QString &str) const
{
    bool ok;
    int hour = str.toInt(&ok);

    if (!ok) {
        return false;
    }

    if (hour < 0 || hour > 23) {
        return false;
    }

    return true;
}

bool DateTimeFrame::validateMinute(const QString &str) const
{
    bool ok;
    int minute = str.toInt(&ok);

    if (!ok) {
        return false;
    }

    if (minute < 0 || minute > 59) {
        return false;
    }

    return true;
}

bool DateTimeFrame::validateYear(const QString& str) const
{
    bool ok;
    int year = str.toInt(&ok);

    if (!ok) {
        return false;
    }

    if (year < 1970 || year > m_maxYear) {
        return false;
    }

    return true;
}

bool DateTimeFrame::validateMonth(const QString& str) const
{
    bool ok;
    int month = str.toInt(&ok);

    if (!ok) {
        return false;
    }

    if (month < 1 || month > 12) {
        return false;
    }

    return true;
}

bool DateTimeFrame::validateDay(const QString& str) const
{
    return QDate::isValid(m_yearEdit->text().toInt(), m_monthEdit->text().toInt(), str.toInt());
}

void DateTimeFrame::checkOkButtonEnable()
{
    bool enable = validateYear(m_yearEdit->text())
            && validateMonth(m_monthEdit->text())
            && validateDay(m_dayEdit->text())
            && validateHour(m_hourEdit->text())
            && validateMinute(m_minuteEdit->text());

    emit okButtonEnable(enable);
}

void DateTimeFrame::updateTs()
{
    m_setDateTimeCheckBox->setText(::QObject::tr("Change time"));
    m_hourLabel->setText(::QObject::tr("hr"));
    m_minuteLabel->setText(::QObject::tr("min"));
    m_yearLabel->setText(::QObject::tr("Y"));
    m_monthLabel->setText(::QObject::tr("M"));
    m_dayLabel->setText(::QObject::tr("D"));
}

void DateTimeFrame::updateDateTimeData(const QDateTime &dateTime)
{
    const QDate& date = dateTime.date();
    const QTime& time = dateTime.time();

    m_yearEdit->setText(QString::number(date.year()));
    m_monthEdit->setText(QString::number(date.month()));
    m_dayEdit->setText(QString::number(date.day()));
    m_hourEdit->setText(QString::number(time.hour()));
    m_minuteEdit->setText(QString::number(time.minute()));

    onYearEditingFinished();
    onMonthEditingFinished();
    onDayEditingFinished();
    onHourEditingFinished();
    onMinuteEditingFinished();

    emit m_setDateTimeCheckBox->checkStateChanged(m_setDateTimeCheckBox->checkState());
}
