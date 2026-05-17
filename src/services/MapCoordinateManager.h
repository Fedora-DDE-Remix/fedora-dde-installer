#ifndef MAPCOORDINATEMANAGER_H
#define MAPCOORDINATEMANAGER_H

#include "TimezoneManager.h"

#include <QPoint>

class MapCoordinateManager
{
public:
    static MapCoordinateManager* instance();

    QPoint zoneInfoToPosition(const SZoneInfo& zone, int width, int height);

    ZoneInfoList getNearestZones(const ZoneInfoList& totalZones, double threshold,
                                 int x, int y, int width, int height);

private:
    MapCoordinateManager();
    MapCoordinateManager(const MapCoordinateManager&) = delete;

    double convertLatitudeToY(double latitude);
    double convertLongitudeToX(double longitude);
};

#endif // MAPCOORDINATEMANAGER_H
