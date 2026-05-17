#ifndef TIMEZONEMAP_H
#define TIMEZONEMAP_H

#include "PopupMenu.h"
#include "TooltipPin.h"
#include "TimezoneManager.h"

#include <QFrame>
#include <QLabel>

class TimezoneMap : public QFrame {
    Q_OBJECT

public:
    explicit TimezoneMap(QWidget* parent = nullptr);

    void setTimezone(const QString& timezone);

signals:
    void timezoneUpdated(const QString& timezone);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

    void hideEvent(QHideEvent *event) override;

private:
    void updateData(const QString& timezone);

    void mark();
    void popupZoneWindow(const QPoint& pos);

    void onPopupZoneSelected(int index);

    // 当前时区数据结构
    SZoneInfo m_currentZone;

    // 所有时区信息列表
    ZoneInfoList m_totalZones;

    // 当前鼠标点击点附近的时区信息列表
    ZoneInfoList m_nearestZones;

    QLabel* m_dot = nullptr;
    TooltipPin* m_zonePin = nullptr;
    PopupMenu* m_popupWindow = nullptr;
    QLabel* m_mapLabel = nullptr;
};

#endif // TIMEZONEMAP_H
