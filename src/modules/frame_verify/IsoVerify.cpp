
#include "IsoVerify.h"
#include "CommonFunc.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

ISOVerify::ISOVerify(QObject *parent) : QThread(parent)
{

}

void ISOVerify::run()
{
    QStringList filterst = {"initrd*.lz", "initrd*.img"};
    QStringList isDirs = {"live/", "boot/"};
    QString isoDirPath = CommonFunc::getSystemEnv(DI_LIVE_DIR_ENV);

    QStringList sourceFileList;
    for (QString dirItem : isDirs) {
        QDir dirIsoLive(isoDirPath + dirItem);
        QFileInfoList file_list = dirIsoLive.entryInfoList(filterst, QDir::Files);
        
        for (QFileInfo infoItem : file_list) {
            sourceFileList.append(isoDirPath + dirItem + infoItem.fileName());
        }
    }
    
    sourceFileList.append(isoDirPath + "oem.squashfs");
    sourceFileList.append(isoDirPath + "live/filesystem.squashfs");

    for (QString sourceFileItem : sourceFileList) {
        QString sourceFile = sourceFileItem;
        QString verifyFile = sourceFileItem + "_sign";
#ifdef QT_DEBUG
        msleep(1000);
#else
        QString err;
        if (!verifyHandle(sourceFile, verifyFile, err)) {
            qWarning() << err;
            Q_EMIT verified(false);
            return;
        }
#endif
    }
    Q_EMIT verified(true);
}

bool ISOVerify::verifyHandle(const QString &sourceFilePath, const QString &verifyFilePath, QString &err)
{
    QString cmd = CommonFunc::getSettingsString("DI_DEEPIN_SQUASHFS_VERIFY");
    QStringList args = {"-f", sourceFilePath, "-s", verifyFilePath};

    QString out;
    if (QFileInfo::exists(cmd)) {
        if (!CommonFunc::command(cmd, args, out)) {
            err = QString("%1 handle verify faield. out: %2.  %3.").arg(cmd + " " + args.join(" "), out, err);
            return false;
        }
    } else {
        err = QString("Command: %1 failed. %2 not found").arg(cmd + " " + args.join(" ")).arg(cmd);
        return false;
    }

    qInfo() << QString("Command: %1 succeed.").arg(cmd + " " + args.join(" "));
    return true;
}