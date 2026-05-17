#ifndef TIMEZONEMANAGER_H
#define TIMEZONEMANAGER_H

#include <QHash>
#include <QMap>
#include <QPair>
#include <QStringList>
#include <QDebug>
#include <QSharedPointer>

struct SZoneInfo {
    // 国家代码
    QString country;
    QString timezone;

    // 时区坐标
    double latitude;
    double longitude;
};

QDebug& operator<<(QDebug& debug, const SZoneInfo& info);

typedef QList<SZoneInfo> ZoneInfoList;
// TODO:关于时区的别名，需要深入的调查一下这玩意的现实意义
typedef QHash<QString, QString> TimezoneAliasMap;

class TimezoneManager
{
public:
    static TimezoneManager* instance();

    ZoneInfoList getZoneInfoList() const;
    void getTotalTimezones(QStringList &tms) const;
    TimezoneAliasMap getTimezoneAliasMap();

    int getZoneInfoIndexByTimezone(const ZoneInfoList& list, const QString& timezone);
    QPair<QString, QString> getLocalTimezone(const QString& timezone, const QString& locale);

    bool isTimezoneInTab(const QString& timezone);

private:
    TimezoneManager();
    TimezoneManager(const TimezoneManager&) = delete;

    void parseZoneFile();

    ZoneInfoList m_zoneInfoList;
};

#endif // TIMEZONEMANAGER_H
