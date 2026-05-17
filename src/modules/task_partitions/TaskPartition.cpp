#include "TaskPartition.h"

#include "globals.h"
#include "PartitionInterface.h"
#include "SaveDataPartition.h"
#include "ConventionalPartition.h"
#include "CryptsetupPartition.h"
#include "MountPartition.h"
#include "PartitionTaskManager.h"
#include <QCommandLineParser>
#include <QDebug>

bool TaskPartition::start()
{
    // init command line
    QCommandLineOption op_mode("m", QString("%1-full disk / %2-advanced / %3-cryptset"
                                            " / %4-mount partition / %5-save data / %6-lvm full disk").arg(
                                   QString::number(FullDiskMode), QString::number(AdvancedMode), QString::number(Cryptsetup),
                                   QString::number(MountMode), QString::number(SaveData), QString::number(LvmFullDisk)));

    op_mode.setValueName("mode");
    QCommandLineOption op_config("c", "partition config.");
    op_config.setValueName("config");

    QCommandLineParser parser;
    parser.setApplicationDescription("deepin-installer-parted");
    parser.addHelpOption();      // 添加帮助选项 （"-h" 或 "--help"）
    parser.addVersionOption();   // 添加版本选项 ("-v" 或 "--version")
    parser.addOption(op_mode);
    parser.addOption(op_config);
    parser.addOption({"k", "generate recovery key"});
    parser.process(*qApp);

    PartitionInterface* task = nullptr;
    switch (parser.value(op_mode).toInt()) {
        case FullDiskMode: // 全盘分区
        case AdvancedMode: // 手动分区
        case LvmFullDisk: // 全盘lvm
        case CustomMode:
        case GhostMode:
            task = new ConventionalPartition;
        break;
        case Cryptsetup: // 全盘加密
            task = new CryptsetupPartition;
        break;
        case SaveData: // 保留用户数据
            task = new SaveDataPartition;
        break;
        case MountMode: // mount partition
            task = new MountPartition;
        break;
        default: qCritical() << "Invalid data -m"; return false;
    }

    QString err;
    if (task && !task->start(parser.value(op_config), err)) {
        qCritical() << "task partition start failed. " << err;
        return false;
    }

    return true;
}
