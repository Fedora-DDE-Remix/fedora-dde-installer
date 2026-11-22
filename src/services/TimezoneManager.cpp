#include "TimezoneManager.h"
#include "CommonFunc.h"

#include <cmath>
#include <libintl.h>
#include <locale.h>
#include <time.h>

// 时区数据文件
const char kZoneTabFile[] = "/usr/share/zoneinfo/zone.tab";
const char kAntarctica[] = "Antarctica";

double convertPos(const QString& pos, int digits)
{
    if (pos.length() < 4 || digits > 9) {
        return 0.0;
    }

    const QString integer = pos.left(digits + 1);
    const QString fraction = pos.mid(digits + 1);
    const double t1 = integer.toDouble();
    const double t2 = fraction.toDouble();

    if (t1 > 0.0) {
        return t1 + t2 / pow(10.0, fraction.length());
    } else {
        return t1 - t2 / pow(10.0, fraction.length());
    }
}

QDebug &operator<<(QDebug &debug, const SZoneInfo &info)
{
    debug << "SZoneInfo {"
          << "cc:" << info.country
          << "tz:" << info.timezone
          << "lat:" << info.latitude
          << "lng:" << info.longitude
          << "}";
    return debug;
}

TimezoneManager *TimezoneManager::instance()
{
    static TimezoneManager tm;
    return &tm;
}

TimezoneManager::TimezoneManager()
{
    parseZoneFile();
}

void TimezoneManager::parseZoneFile()
{
    const QString& content(CommonFunc::readFile(kZoneTabFile));
    QStringList strList = content.split('\n');

    for (const QString& line : strList) {
        if (!line.startsWith('#')) {
            const QStringList parts(line.split('\t'));
            if(parts.length() < 3){
                continue;
            }

            const QString& continentTimeZone = parts.at(2);
            if (continentTimeZone.contains(kAntarctica)){
                continue;
            }

            int index = continentTimeZone.indexOf('/');
            if(index < 0){
                qWarning() << "invalid Continent/timeZone: " << continentTimeZone;
                continue;
            }

            const QString coordinates = parts.at(1);
            int coordIndex = coordinates.indexOf('+', 3);
            if (coordIndex < 0) {
                coordIndex = coordinates.indexOf('-', 3);
            }
            if (coordIndex < 0) {
                qWarning() << QString("Invalid coordinate %1").arg(coordinates);
                continue;
            }

            const double latitude = convertPos(coordinates.left(coordIndex), 2);
            const double longitude = convertPos(coordinates.mid(coordIndex), 3);
            const SZoneInfo zoneInfo = {parts.at(0), parts.at(2), latitude, longitude};

            m_zoneInfoList << zoneInfo;
        }
    }

}

int TimezoneManager::getZoneInfoIndexByTimezone(const ZoneInfoList& list, const QString& timezone)
{
    for (int idx = 0; idx < list.length(); ++idx) {
        if (list.at(idx).timezone == timezone) {
            return idx;
        }
    }

    return -1;
}

QPair<QString, QString> TimezoneManager::getLocalTimezone(const QString& timezone, const QString& locale)
{
    QString localName;

    if(locale == "en_US"){
        localName = timezone;
    } else {
        QString localBak = setlocale(LC_ALL, nullptr);

        // TODO: 对于报错的处理
        setlocale(LC_ALL, QString(locale + ".utf8").toLocal8Bit().constData());

        // TODO: 调研是否需要保护环境变量
        localName =
            dgettext("fedora-dde-installer-timezones", timezone.toLocal8Bit().constData());

        if (!localBak.isEmpty()) {
            setlocale(LC_ALL, localBak.toLocal8Bit().constData());
        }
    }

    int index = localName.indexOf('/');

    if (index <= 0) {
        qWarning() << "invalid local timezone:" << localName;
        return QPair<QString, QString>("", localName);
    }

     return QPair<QString, QString>(localName.left(index), localName.mid(index + 1));
}

TimezoneAliasMap TimezoneManager::getTimezoneAliasMap()
{
  TimezoneAliasMap map;
  const QString kTimezoneAliasFile = CommonFunc::getSystemEnv(DI_SRC_DIR_ENV) + "/resources/timezone_alias";

  if (!QFile::exists(kTimezoneAliasFile)) {
      qWarning() << QString("Map file %1 is not exist").arg(kTimezoneAliasFile);
      return map;
  }

  const QString content = CommonFunc::readFile(kTimezoneAliasFile);
  for (const QString& line : content.split('\n')) {
      if (line.isEmpty()) {
          continue;
      }

      const QStringList parts = line.split(':');
      if (parts.length() == 2) {
          map.insert(parts.at(0), parts.at(1));
      }
  }

  return map;
}

bool TimezoneManager::isTimezoneInTab(const QString& timezone)
{
    const int index = getZoneInfoIndexByTimezone(m_zoneInfoList, timezone);

    return index > -1;
}

ZoneInfoList TimezoneManager::getZoneInfoList() const
{
    return m_zoneInfoList;
}

void TimezoneManager::getTotalTimezones(QStringList &tms) const
{
    for (auto& it : m_zoneInfoList) {
        tms << it.timezone;
    }
}
