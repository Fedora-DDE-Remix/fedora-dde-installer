#include "LibpartedManager.h"
#include "CommonFunc.h"

#include <QDebug>

int main(int argc, char* argv[])
{
    if (argc != 6) {
        qWarning() << QString("Usage: %1 devicePath partitionPath partitionType partitionStartSector partitionEndSector").arg(argv[0]);
        return 1;
    }

    SPartitionInfo info;
    info.device = argv[1];
    info.path = argv[2];
    info.partitionType = CommonFunc::qstringToEnumType(argv[3]);
    info.startPoint = QString(argv[4]).toLongLong();
    info.endPoint = QString(argv[5]).toLongLong();

    if (!LibpartedManager::instance()->resizeExtendedPartition(info)) {
        qWarning() << QString("Call resizeExtendedPartition failed, device %1, partition %2, type %3, start sector %4, end sector %5")
                      .arg(info.device)
                      .arg(info.path)
                      .arg(CommonFunc::enumTypeToString(info.partitionType))
                      .arg(info.startPoint)
                      .arg(info.endPoint);
        return 1;
    }

    return 0;
}
