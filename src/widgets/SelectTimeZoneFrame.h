#ifndef SELECTTIMEZONEFRAME_H
#define SELECTTIMEZONEFRAME_H

#include "TimezoneManager.h"

#include <QLabel>
#include <DFrame>
#include <DComboBox>

DWIDGET_USE_NAMESPACE

class SelectTimeZoneFrame : public DFrame
{
    Q_OBJECT

public:
    explicit SelectTimeZoneFrame(QWidget* parent = nullptr);

    void setTimezone(const QString& timezone);

signals:
    void timezoneUpdated(const QString& timezone);

protected:
    void showEvent(QShowEvent* event) override;

private:
    void onCitySelectedChanged(const QString& text);
    void updateTs();

    QLabel* m_countryLabel = nullptr;
    DComboBox* m_cityComboBox = nullptr;

    // 所有时区的<本地化，英语名称>列表
    QMap<QString, QString> m_totalTimeZones;
};

#endif // SELECTTIMEZONEFRAME_H
