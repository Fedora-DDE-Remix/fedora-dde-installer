#include "TaskPartition.h"
#include "globals.h"
#include "ConventionalPartition.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFileInfo>
#include <unistd.h>

bool TaskPartition::start()
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Fedora DDE Remix partition helper");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption modeOption("m", "Partition mode: 0 full disk, 1 advanced, 6 custom.", "mode");
    QCommandLineOption configOption("c", "Reviewed partition policy file.", "config");
    parser.addOption(modeOption);
    parser.addOption(configOption);
    parser.process(*QCoreApplication::instance());

    bool validNumber = false;
    const int mode = parser.value(modeOption).toInt(&validNumber);
    const QFileInfo config(parser.value(configOption));
    if (geteuid() != 0 || !parser.isSet(modeOption) || !parser.isSet(configOption)
            || !validNumber || (mode != FullDiskMode && mode != AdvancedMode && mode != CustomMode)
            || !config.isAbsolute() || !config.isFile() || !config.isReadable()) {
        qCritical() << "A supported mode and reviewed policy file are required.";
        return false;
    }
    ConventionalPartition task;
    QString error;
    if (!task.start(config.absoluteFilePath(), error)) {
        qCritical() << "Partitioning failed:" << error;
        return false;
    }
    return true;
}
