#include "MapCoordinateManager.h"

#include <math.h>

double radians(double degrees) {
    return (degrees / 360.0) * M_PI * 2;
}

MapCoordinateManager *MapCoordinateManager::instance()
{
    static MapCoordinateManager mcm;
    return &mcm;
}

QPoint MapCoordinateManager::zoneInfoToPosition(const SZoneInfo &zone, int width, int height)
{
    const int x = int(convertLongitudeToX(zone.longitude) * width);
    const int y = int(convertLatitudeToY(zone.latitude) * height);

    return QPoint(x, y);
}

MapCoordinateManager::MapCoordinateManager()
{
}

double MapCoordinateManager::convertLatitudeToY(double latitude)
{
    const double bottomLat = -59;
    const double topLat = 81;
    const double fullRange = 4.6068250867599998;
    double topPer, y, topOffset, mapRange;

    topPer = topLat / 180.0;
    topOffset = fullRange * topPer;
    mapRange = fabs(1.25 * log(tan(M_PI_4 + 0.4 * radians(bottomLat))) -
                     topOffset);

    y = 1.25 * log(tan(M_PI_4 + 0.4 * radians(latitude)));
    y = fabs(y - topOffset);
    y = y / mapRange;

    return y;
}

double MapCoordinateManager::convertLongitudeToX(double longitude)
{
    const double xdegOffset = -6;
    return ((180.0 + longitude) / 360.0 + xdegOffset / 180.0);
}

ZoneInfoList MapCoordinateManager::getNearestZones(const ZoneInfoList& totalZones, double threshold
                                                   , int x, int y, int width, int height)
{
    ZoneInfoList zones;
    double minDistance = width * width + height * height;
    int idx = -1;

    for (int index = 0; index < totalZones.length(); index++) {
        const SZoneInfo& zone = totalZones.at(index);
        const double pointX = convertLongitudeToX(zone.longitude) * width;
        const double pointY = convertLatitudeToY(zone.latitude) * height;
        const double dx = pointX - x;
        const double dy = pointY - y;
        const double distance = dx * dx + dy * dy;

        if (distance < minDistance) {
            minDistance = distance;
            idx = index;
        }

        if (distance <= threshold) {
            zones.append(zone);
        }
    }

    if (zones.isEmpty() && idx >= 0) {
        zones.append(totalZones.at(idx));
    }

    return zones;
}
