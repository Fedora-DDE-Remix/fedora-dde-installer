#include "TimezoneMap.h"
#include "CommonFunc.h"
#include "MapCoordinateManager.h"
#include "TimezoneManager.h"

#include <QDebug>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QImage>

const int kZonePinHeight = 30;
const int kZonePinMinimumWidth = 60;

const double kDistanceThreshold = 50.0;

const int kMapBesideGap = 30;
const QString kTimeZoneMap = ":/images/mapnoframe.svg";

TimezoneMap::TimezoneMap(QWidget* parent)
    : QFrame(parent)
    , m_totalZones(TimezoneManager::instance()->getZoneInfoList())
{
    QPixmap dotPixmap = CommonFunc::renderPixmap(":/images/position.svg");
    if (dotPixmap.isNull()) {
        qWarning() << "Timezone dot pixmap is null";
    }

    m_dot = new QLabel(this);
    m_dot->setPixmap(dotPixmap);
    m_dot->setFixedSize(dotPixmap.size() / devicePixelRatioF());
    m_dot->hide();

    m_zonePin = new TooltipPin(this);
    m_zonePin->setFixedHeight(kZonePinHeight);
    m_zonePin->setMinimumWidth(kZonePinMinimumWidth);
    m_zonePin->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_zonePin->hide();

    m_popupWindow = new PopupMenu(this);
    m_popupWindow->setShowCheckIcon(false);
    m_popupWindow->hide();

    connect(m_popupWindow, &PopupMenu::onHide, m_dot, [=] {
        m_dot->hide();
    });
    connect(m_popupWindow, &PopupMenu::menuSelected,
            this, &TimezoneMap::onPopupZoneSelected);

    m_mapLabel = new QLabel(this);
    m_mapLabel->installEventFilter(this);
    m_mapLabel->setAlignment(Qt::AlignCenter);

    QImage image(kTimeZoneMap);
    QSize mapSize = image.size();
    if(!CommonFunc::isBigScreen()) {
        mapSize *= 0.5;
    }

    m_mapLabel->setFixedSize(mapSize);
    QPixmap mapPixmap = CommonFunc::renderSVG(kTimeZoneMap, mapSize);
    m_mapLabel->setPixmap(mapPixmap);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(20, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_mapLabel, 0, Qt::AlignCenter);

    setLayout(layout);

    mark();
}

void TimezoneMap::setTimezone(const QString& timezone)
{
    if (!timezone.isEmpty()) {
        updateData(timezone);
        mark();
    } else {
        qWarning() << "Timezone is empty";
    }
}

void TimezoneMap::updateData(const QString& timezone)
{
    const int index = TimezoneManager::instance()->getZoneInfoIndexByTimezone(m_totalZones, timezone);

    if (0 <= index && index < m_totalZones.size()) {
        m_currentZone = m_totalZones.at(index);
        m_nearestZones.clear();
        m_nearestZones.append(m_currentZone);
    } else {
        qWarning() << QString("Timezone not found %1").arg(timezone);
    }
}

bool TimezoneMap::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_mapLabel) {
        if (event->type() == QMouseEvent::MouseButtonPress) {
            QMouseEvent* me = reinterpret_cast<QMouseEvent*>(event);
            m_nearestZones = MapCoordinateManager::instance()->getNearestZones(m_totalZones, kDistanceThreshold, me->position().toPoint().x(), me->position().toPoint().y(),
                                                                               m_mapLabel->width(), m_mapLabel->height());
            if (m_nearestZones.length() == 1) {
                m_currentZone = m_nearestZones.first();
                mark();
                emit timezoneUpdated(m_currentZone.timezone);
            } else {
                popupZoneWindow(me->pos());
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void TimezoneMap::hideEvent(QHideEvent *event)
{
    // TODO: m_popupWindow为DAbstractDialog弹窗，他的隐藏目前是手动触发的
    m_popupWindow->hide();

    QFrame::hideEvent(event);
}

void TimezoneMap::popupZoneWindow(const QPoint& pos)
{
    m_dot->hide();
    m_zonePin->hide();
    m_popupWindow->hide();

    QStringList zoneNames;
    for (const SZoneInfo& zone : m_nearestZones) {
        zoneNames.append(TimezoneManager::instance()->getLocalTimezone(
                             zone.timezone, CommonFunc::getSettingsString("DI_LOCALE")).second);
    }

    m_popupWindow->setStringList(zoneNames);

    // TODO：这里考虑封装成一个函数是否合理
    const int halfWidth = m_dot->width() / 2;
    const int halfHeight = m_dot->height() / 2;

    QPoint parentPos(m_mapLabel->mapToParent(pos));
    const QPoint dotPos(parentPos.x() - halfWidth,
                        parentPos.y() - halfHeight);
    m_dot->move(dotPos);
    m_dot->show();
    m_dot->raise();

    parentPos = m_mapLabel->mapToGlobal(pos);
    const QPoint popupPos(parentPos.x(), parentPos.y() - halfHeight - 8);

    m_popupWindow->popup(popupPos);
    //每次显示将选中项默认给第一项
    m_popupWindow->setCurrentRow(0);
    m_popupWindow->raise();
    m_popupWindow->show();
}

void TimezoneMap::mark()
{
    if (!isVisible()) {
        return;
    }

    m_dot->hide();
    m_zonePin->hide();
    m_popupWindow->hide();

    if (!m_nearestZones.isEmpty()) {
        m_zonePin->setText(TimezoneManager::instance()->getLocalTimezone(
                               m_currentZone.timezone, CommonFunc::getSettingsString("DI_LOCALE")).second);
        m_zonePin->adjustSize();

        const QPoint zonePos = MapCoordinateManager::instance()->zoneInfoToPosition(m_currentZone
                                                                                    , m_mapLabel->width()
                                                                                    , m_mapLabel->height());

        // TODO：这里考虑封装成一个函数是否合理
        const int halfWidth = m_dot->width() / 2;
        const int halfHeight = m_dot->height() / 2;

        const QPoint parentPos(m_mapLabel->mapToParent(zonePos));
        QPoint zonePinPos(parentPos.x() - m_zonePin->width() / 2, parentPos.y() - halfHeight - 2 - m_zonePin->height());

        Orientation ot = Orientation::Down;
        if (zonePinPos.x() < 0) {
            ot = Orientation::Left;
            zonePinPos = QPoint(parentPos.x() + halfWidth + 2, parentPos.y() - m_zonePin->height() / 2);
        } else if (zonePinPos.y() < 0) {
            ot = Orientation::Up;
            zonePinPos = QPoint(parentPos.x() - m_zonePin->width() / 2, parentPos.y() + halfHeight + 2);
        }

        m_zonePin->setOrientation(ot);

        m_zonePin->popup(zonePinPos);
        m_zonePin->raise();

        const QPoint dotPos(parentPos.x() - halfWidth,
                            parentPos.y() - halfHeight);
        m_dot->move(dotPos);
        m_dot->show();
        m_dot->raise();
    }
}

void TimezoneMap::onPopupZoneSelected(int index)
{
    if (0 <= index && index < m_nearestZones.length()) {
        m_currentZone = m_nearestZones.at(index);
        mark();
        emit timezoneUpdated(m_currentZone.timezone);
    }
}
