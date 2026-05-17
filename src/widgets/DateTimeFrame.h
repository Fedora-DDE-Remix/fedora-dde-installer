#ifndef DATETIMEFRAME_H
#define DATETIMEFRAME_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <DCheckBox>
#include <QDateTime>

DWIDGET_USE_NAMESPACE

// QLineEdit清空其中的内容，焦点转移出去，根本不会发射QLineEdit::editingFinished信号
// 不确定这是不是bug，目前采用TimeDateLineEdit子类解决此问题。
class TimeDateLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    TimeDateLineEdit(QWidget* parent = nullptr);

signals:
    void lostFocus();

protected:
    void focusOutEvent(QFocusEvent* event) override;
};

class DateTimeFrame : public QWidget
{
    Q_OBJECT

public:
    explicit DateTimeFrame(QWidget *parent = nullptr);

    void updateData(const bool& enableNTP, const QString& dateTime);

    QString timedate() const;
    bool isEnabled() const;

signals:
    void okButtonEnable(bool enable);

protected:
    void showEvent(QShowEvent* event) override;

private:
    void updateTs();

    void updateDateTimeData(const QDateTime& dateTime = QDateTime::currentDateTime());

    bool validateHour(const QString& str) const;
    bool validateMinute(const QString& str) const;
    bool validateYear(const QString& str) const;
    bool validateMonth(const QString& str) const;
    bool validateDay(const QString& str) const;

    void checkOkButtonEnable();

    void onYearEditingFinished();
    void onMonthEditingFinished();
    void onDayEditingFinished();
    void onHourEditingFinished();
    void onMinuteEditingFinished();

    void autoAdjustDay();

    DCheckBox* m_setDateTimeCheckBox = nullptr;
    TimeDateLineEdit* m_hourEdit = nullptr;
    TimeDateLineEdit* m_minuteEdit = nullptr;
    TimeDateLineEdit* m_yearEdit = nullptr;
    TimeDateLineEdit* m_monthEdit = nullptr;
    TimeDateLineEdit* m_dayEdit = nullptr;

    QLabel* m_hourLabel = nullptr;
    QLabel* m_minuteLabel = nullptr;
    QLabel* m_yearLabel = nullptr;
    QLabel* m_monthLabel = nullptr;
    QLabel* m_dayLabel = nullptr;

    int m_maxYear = 9999;
};

#endif // DATETIMEFRAME_H
