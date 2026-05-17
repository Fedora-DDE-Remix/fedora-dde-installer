#ifndef LIBPARTEDMANAGER_H
#define LIBPARTEDMANAGER_H

#include "structs.h"

#include <parted/parted.h>
#include <QString>

class LibpartedManager
{
public:
    static LibpartedManager* instance();

    bool resizeExtendedPartition(const SPartitionInfo& partInfo);
    QString getPartitionPath(const SPartitionInfo& partInfo);

private:
    LibpartedManager();
    LibpartedManager(const LibpartedManager& lm) = delete;

    void settleDevice(int timeout);
    bool commit(PedDisk* lpDisk);
    void destroyDeviceAndDisk(PedDevice*& lpDevice, PedDisk*& lpDisk);
    bool getDeviceAndDisk(const QString& devicePath,
                          PedDevice*& lpDevice,
                          PedDisk*& lpDisk);
    QString getPartitionPath(PedPartition* lpPartition);
    PedPartition* getPartition(PedDisk* lpDisk, const QString& partPath);
};

#endif // LIBPARTEDMANAGER_H
