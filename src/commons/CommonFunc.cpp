#include "CommonFunc.h"
#include "globals.h"
#include "Process.h"
#include "LibpartedManager.h"

#include <QUuid>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDir>
#include <QThread>
#include <QBitmap>
#include <QPainter>
#include <QWidget>
#include <QProcess>
#include <iostream>
#include <QCryptographicHash>
#include <QImageReader>
#include <QApplication>
#include <QScreen>
#include <QSettings>
#include <cmath>
#include <QPainterPath>
#include <QRegularExpression>
#include <QDateTime>
#include <QTimeZone>
#include <QDir>


static const int LSBLK_RETRYNUM = 30;

QDebug operator<<(QDebug debug, const FSTabInfo &info) {
    QDebugStateSaver saver(debug);
    debug.nospace().quote() << info.spec << ", " << info.mountPoint << ", " <<
                               info.fstype << ", " << info.dump << ", " << info.fsck;
    return debug;
}

QString CommonFunc::getSystemEnv(const QString &key)
{

#ifdef QT_DEBUG
    QString reset = key;
#else
    QString reset = qgetenv(key.toStdString().c_str());
#endif // QT_DEBUG

    return reset;
}

QMap<QString, QStringList> CommonFunc::getDirs(const QStringList &dirs)
{
    QMap<QString, QStringList> infos;
    for (QString name : dirs) {
        infos[name] = getDirs(name);
    }

    return infos;
}

QStringList CommonFunc::getDirs(const QString &dir, const QStringList &nameFilters)
{
    QStringList reset;
    for (auto path : QDir(dir).entryInfoList(nameFilters, QDir::NoDotAndDotDot | QDir::AllEntries | QDir::System)) {
        reset << path.absoluteFilePath();
    }

    return reset;
}

QString CommonFunc::getInstallStatus(int progress)
{
    if (progress >= 100)
        return QObject::tr("System installed; account setup is next...");
    if (progress >= 90)
        return QObject::tr("Preparing account setup...");
    if (progress >= 85)
        return QObject::tr("Finishing system configuration...");
    if (progress >= 60)
        return QObject::tr("Installing the bootloader and system settings...");
    if (progress >= 10)
        return QObject::tr("Copying system files...");
    if (progress >= 1)
        return QObject::tr("Partitioning...");
    return QObject::tr("Preparing...");
}

void CommonFunc::initConfig(const QString &filePath)
{
    const QString defaultSettings = getSystemEnv(DI_CONFIG_DIR_ENV) + "/settings/default_settings.ini";
    QSettings source(defaultSettings, QSettings::IniFormat);
    QSettings destination(filePath, QSettings::IniFormat);
    for (const QString &key : source.allKeys())
        destination.setValue(key, source.value(key));
    destination.sync();
}

QStack<SPluginInfo> CommonFunc::getPlugins(const QString &jsonName)
{
    QStack<SPluginInfo> infos;
    QString config = "";

    QString defaultConfig = QString("%1/settings/%2.json").arg(CommonFunc::getSystemEnv(DI_CONFIG_DIR_ENV), jsonName);
    if (QFileInfo::exists(defaultConfig)) {
        config = defaultConfig;
    }

    QString err;
    QJsonDocument jsonDoc;
    if (!readJsonFile(config, jsonDoc, err)) {
        qCritical() << err;
        return infos;
    }

    QJsonArray policyArray = jsonDoc.array();
    // 倒序解析json入栈，界面显示顺序按json中顺序
    qInfo() << "DI_INSTALL_MODE: " << getSettingsString("DI_INSTALL_MODE");
    for(int i = policyArray.size() - 1; i >=0; i--)
    {
        QString mode = policyArray[i].toObject()["mode"].toString();
        if (mode == gInstallModeAll || mode == getSettingsString("DI_INSTALL_MODE")) {
            QJsonArray ar = policyArray[i].toObject()["plugins"].toArray();
            for(int j = ar.size() - 1; j >=0; j--)
            {
                SPluginInfo info;
                info.name = ar[j].toObject()["name"].toString();
                info.version = ar[j].toObject()["version"].toString();
                info.path = QString(getSystemEnv(DI_INSTALL_MODULE_DIR_ENV)) + QDir::separator() + ar[j].toObject()["path"].toString();
                QJsonArray osTypes = ar[j].toObject()["osType"].toArray();
                if (osTypes.contains(getCurrentOsType())) {
                    infos.append(info);
                }
            }
        }
    }

    return infos;
}

QVariant CommonFunc::getSettingsValue(const QString &key, const QString &section, const QString &filePath)
{
    QString path(filePath);

    if (!path.isEmpty() && !QFileInfo::exists(path)) {
        qWarning() << "file not exist." << path;
        return QString();
    }

    if (path.isEmpty()) {
        path = getSystemEnv(DI_CONFIG_FILE_ENV);
    }

    if (!QFileInfo::exists(path)) {
        qWarning() << "file not exist." << path;
        return QString();
    }

    QSettings settings(path, QSettings::IniFormat);
    QVariant value;

    if (section.isEmpty()) {
        value = settings.value(key).toString();
    } else {
        settings.beginGroup(section);
        value = settings.value(key).toString();
        settings.endGroup();
    }

    return value;
}

QStringList CommonFunc::getSettingsStringList(const QString &key, const QString &section, const QString &filePath)
{
    QVariant value = getSettingsValue(key, section, filePath);
    return value.isNull() ? QStringList() : qstringSplitToQStringList(value.toString(), ";");
}

bool CommonFunc::getSettingsBool(const QString &key, const QString &section, const QString &filePath)
{
    QVariant value = getSettingsValue(key, section, filePath);
    return value.isNull() ? false : value.toBool();
}

QString CommonFunc::getSettingsString(const QString &key, const QString &section, const QString &filePath)
{
    QVariant value = getSettingsValue(key, section, filePath);
    return  value.isNull() ? QString() : value.toString();
}

int CommonFunc::getSettingsInt(const QString &key, const QString &section, const QString &filePath)
{
    QVariant value = getSettingsValue(key, section, filePath);
    return  value.isNull() ? -1 : value.toInt();
}

void CommonFunc::setSettingsValue(const QString &key, const QVariant &value, const QString &section, const QString &filePath)
{
    QString path(filePath);

    if (!path.isEmpty() && !QFileInfo::exists(path)) {
        qWarning() << "file not exist." << path;
        return;
    }

    if (path.isEmpty()) {
        path = getSystemEnv(DI_CONFIG_FILE_ENV);
    }

    if (!QFileInfo::exists(path)) {
        qWarning() << "file not exist." << path;
        return;
    }

    QSettings settings(path, QSettings::IniFormat);

    if (section.isEmpty()) {
        settings.setValue(key, value);
    } else {
        settings.beginGroup(section);
        settings.setValue(key, value);
        settings.endGroup();
    }

    settings.sync();
}

bool CommonFunc::isBigScreen()
{
    QRect mm = QGuiApplication::primaryScreen()->geometry();
    if (mm.width() < 1920 || mm.height() < 1080) {
        return false;
    }

    return true;
}

QSize CommonFunc::getShowSize()
{
    if (!isBigScreen()) {
        return QSize(gmainwindowwidthsmall, gmainwindowheightsmall);
    }

    return QSize(gmainwindowwidthbig, gmainwindowheightbig);
}

QSize CommonFunc::getInnerSize()
{
    const int bigWidth = 300;
    const int bigHeight = 600;

    const int smallWidth = 200;
    const int smallHeight = 400;

    if (!isBigScreen()) {
        return QSize(smallWidth, smallHeight);
    }

    return QSize(bigWidth, bigHeight);
}

int CommonFunc::getInnerMargin()
{
    const int bigMagin = 20;
    const int smallMagin = 5;

    if (!isBigScreen()) {
        return smallMagin;
    }

    return bigMagin;
}

int CommonFunc::getTopMargin()
{
    const int bigMargin = 50;
    const int smallMargin = 8;

    if (!isBigScreen()) {
        return smallMargin;
    }

    return bigMargin;
}

int CommonFunc::getBottomMargin()
{
    const int bigMargin = 70;
    const int smallMargin = 18;

    QRect mm = QGuiApplication::primaryScreen()->geometry();
    if (mm.width() <= gmainwindowwidthbig || mm.height() <= gmainwindowheightbig) {
        return mm.height() - smallMargin;
    }

    return mm.height() - bigMargin;
}

QSize CommonFunc::getDiskIconSize()
{
    const int bigSize = 150;
    const int smallSize = 80;

    if (!isBigScreen()) {
        return QSize(smallSize, smallSize);
    }

    return QSize(bigSize, bigSize);
}

QSize CommonFunc::getInstallResultIconSize()
{
    const int bigSizeWidth = 320; //380
    const int bigSizeHeight = 227; //270
    const int smallSizeWidth = 240;
    const int smallSizeHeight = 130;

    if (!isBigScreen()) {
        return QSize(smallSizeWidth, smallSizeHeight);
    }

    return QSize(bigSizeWidth, bigSizeHeight);
}

QSize CommonFunc::getTimezoneMapSize()
{
    const int kMaxMapWidth = 950;
    const int kMaxMapHeight = 490;

    const int kMinMapWidth = 480;
    const int kMinMapHeight = 240;

    if (!CommonFunc::isBigScreen()) {
        return QSize(kMinMapWidth, kMinMapHeight);
    }

    return QSize(kMaxMapWidth, kMaxMapHeight);
}

QSize CommonFunc::getQRPicSize()
{
    const int kMaxMapWidth = 134;
    const int kMaxMapHeight = 134;

    const int kMinMapWidth = 67;
    const int kMinMapHeight = 67;

    if (!CommonFunc::isBigScreen()) {
        return QSize(kMinMapWidth, kMinMapHeight);
    }

    return QSize(kMaxMapWidth, kMaxMapHeight);
}

QSize CommonFunc::getAnimationPicSize()
{
    return CommonFunc::isBigScreen() ? QSize(860, 395) : QSize(565, 260);
}

bool CommonFunc::cryptKeyFile(QString &keyPath, QString &err)
{
    QString pukFileName = CommonFunc::getSettingsString("DI_DISK_PUB_RSA_FILE_CONFIG");
    if (pukFileName.isEmpty()) {
        err = "no puk file name set in conf !";
        keyPath = "";
        return false;
    }

    QString pukFilePath = CommonFunc::getSystemEnv(DI_LIVE_DIR_ENV) + "/" + pukFileName;
    if (!QFile::exists(pukFilePath)) {
        err = QString("puk file %1 not found!").arg(pukFilePath);
        keyPath = "";
        return false;
    }

    QDateTime currentDateTime = QDateTime::currentDateTimeUtc();
    QTimeZone gmtTimeZone = QTimeZone::utc();
    QDateTime gmtDateTime = currentDateTime.toTimeZone(gmtTimeZone);
    qint64 gmtTimestamp = gmtDateTime.toSecsSinceEpoch();
    QString cryptKeyFilePath = "/tmp/" + CommonFunc::getSettingsString("DI_DISK_CRYPT_KEY_FILE_CONFIG").arg(QString("_%1").arg(gmtTimestamp));
    QString keyFilePath = CommonFunc::getSettingsString("DI_CRYPT_RECOVERY_KEY");
    // 创建加密密钥
    QString create_crypt_key_cmd = QString("openssl rsautl -encrypt -in %1 -inkey %2 -pubin -out %3")
            .arg(keyFilePath, pukFilePath, cryptKeyFilePath);
    if (!command("/bin/bash", {"-c", create_crypt_key_cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(create_crypt_key_cmd).arg(err);
        keyPath = "";
        return false;
    }

    keyPath = cryptKeyFilePath;

    return true;
}

bool CommonFunc::bindRecoveryKeyToDev(const QString &diskPwd, const QStringList &cryptDevList, const QString &keyStr, QString &err)
{
    for (const QString &devItem : cryptDevList) {
        //获取加密设备对应的实际分区路径
        QString cmd = QString("cryptsetup status \"%1\" | grep \"device\" | awk '{print $2}'").arg(devItem);
        QString cmdOut = "";
        if (!command("/bin/bash", {"-c", cmd}, cmdOut)) {
            err = QString("Command: %1 failed. err : %2").arg(cmd).arg(cmdOut);
            return false;
        }

        cmdOut.replace("\n", "");
        cmdOut.replace(" ", "");

        if (cmdOut.isEmpty()) {
            err = QString("no crypt device found!").arg(cmd).arg(cmdOut);
            return false;
        }

        //先删除之前的秘钥槽设置
        cmd = QString("echo '%1' | cryptsetup luksKillSlot %2 7").arg(diskPwd).arg(cmdOut);
        if (!command("/bin/bash", {"-c", cmd}, err)) {
            qWarning() << QString("Command: %1 failed. warn : %2").arg(cmd.replace(QRegularExpression("echo[\\s|\\S]*\\|"), "echo ****** |")).arg(err);
        }

        // 设置新的秘钥槽
        cmd = QString("echo '%1' | cryptsetup luksAddKey %2 '%3' --key-slot 7").arg(diskPwd).arg(cmdOut).arg(keyStr);
        if (!command("/bin/bash", {"-c", cmd}, err)) {
            err = QString("Command: %1 failed. %2").arg(cmd).arg(err);
            return false;
        }
    }

    return true;
}

bool CommonFunc::isCryptDisk()
{
    return !getSettingsString("DI_CRYPT_PASSWORD").isEmpty();
}

LanguageList CommonFunc::getLanguageList()
{
    LanguageList list;
    const QString content(CommonFunc::readFile(getSystemEnv(DI_CONFIG_DIR_ENV) + "/settings/languages.json"));
    const QJsonArray langList = QJsonDocument::fromJson(content.toUtf8()).array();
    for (const QJsonValue& langValue : langList) {
        const QJsonObject obj = langValue.toObject();
        SLanguageItem item;
        item.name = obj.value("name").toString();
        item.locale = obj.value("locale").toString();
        item.localName = obj.value("local_name").toString();
        item.timezone = obj.value("timezone").toString();
        list.append(item);
    }
  return list;
}

InstallWorkflow &CommonFunc::installWorkflow()
{
    static InstallWorkflow workflow;
    return workflow;
}

QString CommonFunc::getSystemLogo()
{
    return QStringLiteral(":/images/fedora-dde-remix.png");
}

QString CommonFunc::getMainGifAnimations()
{
    auto backgroundImage = getSettingsString("DI_BACKGROUND_IMAGE");
    if (QFile::exists(backgroundImage)) {
        return backgroundImage;
    }
    
    qWarning() << QString("%1 not exist").arg(backgroundImage);
    
    QStringList reset = getMainAnimations();
    // 主动画目录不为空的情况下，获取该目录下排序第一的文件
    return reset.isEmpty() ? QString("") : reset.first();
}

QStringList CommonFunc::getMainAnimations()
{
    return getDirs(getSystemEnv(DI_CONFIG_DIR_ENV) + "/slides/Fedora/mains");
}

QStringList CommonFunc::getInstallAnimations()
{
    return getDirs(getSystemEnv(DI_CONFIG_DIR_ENV) + "/slides/Fedora/installs");
}

QStringList CommonFunc::getConfigAnimations()
{
    return getDirs(getSystemEnv(DI_CONFIG_DIR_ENV) + "/slides/Fedora/configs");
}

QString CommonFunc::getPartitionPolicy()
{
    return getSystemEnv(DI_CONFIG_DIR_ENV) + "/settings/full_disk_policy.json";
}

double CommonFunc::mToG(qint64 size)
{
    return round((double)size / (double)giDeviceUnitSize * 10) / 10;
}

qint64 CommonFunc::gToM(double size)
{
    return qint64(size * giDeviceUnitSize);
}

qint64 CommonFunc::gToM(qint64 size)
{
    return size * giDeviceUnitSize;
}

qint64 CommonFunc::bytesToM(qint64 size)
{
    return size / giDeviceUnitSize / giDeviceUnitSize;
}

qint64 CommonFunc::bytesToG(qint64 size)
{
    return size / (giDeviceUnitSize * giDeviceUnitSize * giDeviceUnitSize);
}

qint64 CommonFunc::kbToMB(qint64 size)
{
    return size / giDeviceUnitSize;
}

qint64 CommonFunc::strToM(const QString &size)
{
    QString strSize = size.toUpper();
    if (strSize.contains("MIB")) {
        return QString(size).replace(QRegularExpression("[a-zA-Z]"), "").toInt();

    } else if (strSize.contains("GIB")) {
        return QString(size).replace(QRegularExpression("[a-zA-Z]"), "").toInt() * giDeviceUnitSize;

    } else if (strSize.contains("TIB")) {
        return QString(size).replace(QRegularExpression("[a-zA-Z]"), "").toInt() * giDeviceUnitSize * giDeviceUnitSize;

    } else {
        return 0;
    }
}

QString CommonFunc::showSizeUnit(double& size, int flag)
{
    if ((qint64)size >= flag * giDeviceUnitSize) {
        size = round(size / (double)giDeviceUnitSize * 10) / 10;
        return "GiB";
    }

    return "MiB";
}

QString CommonFunc::showSizeUnit(double &size, double &totai, int flag)
{
    if ((qint64)totai >= flag * giDeviceUnitSize) {
        size = round(size / (double)giDeviceUnitSize * 10) / 10;
        totai = round(totai / (double)giDeviceUnitSize * 10) / 10;
        return getSettingsString("DI_SIZE_GUNIT_CONFIG");
    }

    return getSettingsString("DI_SIZE_MUNIT_CONFIG");;
}

qint64 CommonFunc::mToSectors(qint64 size, qint64 sectorsSize)
{
    if (sectorsSize <= 0) {
        qWarning() << QString("Parameter sectorsSize value: %1 error.").arg(sectorsSize);
        return -1;
    }

    return size * giDeviceUnitSize * giDeviceUnitSize / sectorsSize;
}

qint64 CommonFunc::sectorsToM(qint64 sectorsCnt, qint64 sectorsSize)
{
    if (sectorsSize <= 0) {
        qWarning() << QString("Parameter sectorsSize value: %1 error.").arg(sectorsSize);
        return -1;
    }

    qint64 oneMib = mToSectors(1, sectorsSize);
    return sectorsCnt / oneMib;
}

qint64 CommonFunc::percentageToM(qint64 size, const QString &percentage)
{
    return size * QString(percentage).replace("%", "").toInt() / 100;
}

qint64 CommonFunc::getMemSize()
{
    QString out;
    if (!CommonFunc::command("/bin/bash", {"-c", "cat /proc/meminfo | grep 'MemTotal' | awk '{print $2}'"}, out)) {
        qWarning() << "get mem size err: " << out;
        return 0;
    }

    return kbToMB(out.toInt());
}

qint64 CommonFunc::getSwapSize()
{
    qint64 reset = getMemSize();
    reset += qRound(sqrt(reset * giDeviceUnitSize * 1.0));
    return qMin(reset, (qint64)CommonFunc::getSettingsInt("DI_SWAP_MAX_SIZE_CONFIG"));
}

qint64 CommonFunc::getPartitionEndPoint(qint64 startPoint, qint64 partSize, qint64 diskSize, qint64 sectors)
{
    qint64 endPoint = startPoint + mToSectors(partSize, sectors);
    return getPartitionEndPoint(endPoint, diskSize, sectors);
}

qint64 CommonFunc::getPartitionEndPoint(qint64 partEndPoint, qint64 diskSize, qint64 sectors)
{
    qint64 diskPoint = mToSectors(diskSize, sectors);
    return qMin(partEndPoint, diskPoint) - 1;
}

qint64 CommonFunc::getPartitionStartPoint(qint64 startPoint, qint64 sectors)
{
    if (sectors <= 0) {
        qWarning() << QString("Parameter sectors value: %1 error.").arg(sectors);
        return startPoint;
    }

    // 采用最优对齐，通常是1Mib。 可以通过"sudo parted 设备 align-check opt 分区号" 查询
    int alignSize = 1 * giDeviceUnitSize * giDeviceUnitSize / sectors;
    startPoint = startPoint + (alignSize - (startPoint % alignSize)) % alignSize;
    
    return startPoint;
}

QString CommonFunc::getEmptyMountPoint()
{
    // 通过一个函数处理动态翻译
    return ::QObject::tr("Do not use");
}

std::pair<QString, bool> CommonFunc::getPartitionStartPointText()
{
    // 通过一个函数处理动态翻译
    return {::QObject::tr("Start"), true};
}

std::pair<QString, bool> CommonFunc::getPartitionEndPointText()
{
    // 通过一个函数处理动态翻译
    return {::QObject::tr("End"), false};
}

QString CommonFunc::getCurrentPlatform()
{
    QMap<QString, QString> BUILD_ARCH_MAP{ { "x86_64",  "x86" },
                                           { "i386", "x86" },
                                           { "i686", "x86" },
                                           { "amd64", "x86" },
                                           { "x86", "x86" },
                                           { "sw_64",   "sw" },
                                           { "mips64", "loongson" },
                                           { "loongarch64", "loongarch64" },
                                           { "riscv64", "riscv64" },
                                           { "aarch64", "arm" } };

    return BUILD_ARCH_MAP[DI_PLATFORM_BUILD_ARCH_ENV];
}

QString CommonFunc::getCurrentOsType()
{
    return QStringLiteral("Fedora");
}

QString CommonFunc::getCurrentOsVersion()
{
    QSettings release("/etc/os-release", QSettings::IniFormat);
    return release.value("VERSION_ID", "45").toString().remove('"');
}

QString CommonFunc::getVersion()
{
    QString version;
    if (command("rpm", {"-q", "--qf", "%{VERSION}-%{RELEASE}", "fedora-dde-installer"}, version))
        return version.trimmed();
    return QStringLiteral("development");
}

OSType CommonFunc::getCurrentOs()
{
    return OSType::Community;
}

QString CommonFunc::isNameDeepin()
{
    return QStringLiteral("Fedora DDE Remix");
}

bool CommonFunc::getSettingsPolicy(QJsonDocument &policyDoc, QString &error)
{
    return readJsonFile(getPartitionPolicy(), policyDoc, error);
}

bool CommonFunc::execHook(const QString &hooksPath, QString &log)
{
    // 由于后端脚本都是bash语法，故修改shell解释器为bash
    return commandPrintLog("/usr/bin/bash", QStringList()
                   << QString("%1/%2").arg(getSystemEnv(DI_INSTALL_TOOLS_DIR_ENV), gHooksManagerScript) << hooksPath, log);
}

bool CommonFunc::command(const QString cmd, const QStringList &args, QString &log)
{
    QProcess *process = new QProcess;
    process->setProgram(cmd);
    process->setArguments(args);
    process->start();

    if (!process->waitForStarted()) {
        log = QString("QProcess::ProcessError: %1").arg(QString::number(process->error()));
        return false;
    }

    // Wait for process to finish without timeout.
    process->waitForFinished(-1);
    if (process->exitStatus() == QProcess::NormalExit && process->exitCode() == 0) {
        log = process->readAllStandardOutput();
        return true;
    } else {
        // 获取具体的错误信息供前端显示
        log = process->readAllStandardError();
        return false;
    }
}

bool CommonFunc::commandPrintLog(const QString cmd, const QStringList &args, QString &log)
{
    Process *process = new Process;  // 执行脚本时实时打印日志类
    process->setProgram(cmd);
    process->setArguments(args);
    process->start();

    if (!process->waitForStarted()) {
        log = QString("QProcess::ProcessError: %1").arg(QString::number(process->error()));
        return false;
    }

    // Wait for process to finish without timeout.
    process->waitForFinished(-1);
    if (process->exitStatus() == QProcess::NormalExit && process->exitCode() == 0) {
        log = process->readAllStandardOutput();
        return true;
    } else {
        // 获取具体的错误信息供前端显示
        log = process->readAllStandardError();
        return false;
    }
}

bool CommonFunc::getCmdListReturn(const QString &cmd, QStringList &listReturn, QString &err)
{
    QString cmdLog = "";
    if (!command("/bin/bash", {"-c", cmd}, cmdLog)) {
        err = "error cmd : " + cmd + " err : " + cmdLog;
        return false;
    }

    cmdLog.replace("0B", "0");
    cmdLog.replace("none", "0");
    cmdLog.replace("null", "0");

    listReturn = qstringSplitToQStringList(cmdLog, "\n");
    return true;
}

void CommonFunc::deleteLayout(QLayout* layout)
{
    if (layout == nullptr) {
        return;
    }

    while (QLayoutItem* item = layout->takeAt(0))
    {
        if (QWidget* widget = item->widget())
            widget->deleteLater();

        if (QLayout* childLayout = item->layout())
            deleteLayout(childLayout);

        if (QSpacerItem* spaerItem = item->spacerItem())
            layout->removeItem(spaerItem);

        delete item;
    }
}

bool CommonFunc::isUEFI()
{
    QString isEFI = getSettingsString("DI_BOOTLOADER_IS_EFI");
    if (isEFI.isEmpty()) {
        return false;
    }

    return CommonFunc::getSettingsBool("DI_BOOTLOADER_IS_EFI");
}

bool CommonFunc::isSW()
{
    return CommonFunc::getCurrentPlatform() == "sw";
}

bool CommonFunc::isPxeInstall()
{
    QFile cmd_file("/proc/cmdline");
    cmd_file.open(QIODevice::ReadOnly);
    if (!cmd_file.isOpen()) {
        qCritical() << "isPxeInstall: Failed to open file. /proc/cmdline";
        return false;
    }

    QString info = cmd_file.readAll();
    if (!info.contains("nfsroot")) {
        return false;
    }

    qInfo() << "isPxeInstall: /proc/cmdline <" << info << ">";
    return true;
}

bool CommonFunc::readJsonFile(const QString &filepath, QJsonDocument &policydoc, QString &err)
{
    QFile file(filepath);
    if (!file.open(QIODevice::Text | QIODevice::ReadOnly)) {
        err = QString("could't open json file : %1 err : %2").arg(filepath).arg(file.errorString());
        return false;
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonParseError jsonError;
    policydoc = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        err = QString("%1 json error. line: %2\n%3").arg(filepath).arg(QString::number(jsonError.offset)).arg(jsonError.errorString());
        return false;
    }

    return true;
}

bool CommonFunc::isTPM()
{
    return CommonFunc::getSettingsBool("DI_TPM_CHECK");
}

bool CommonFunc::isGhost()
{
    return getSettingsBool("DI_IS_GHOST_MODE");
}

bool CommonFunc::isFirstBoot()
{
    return false;
}

bool CommonFunc::checkDisklabelType(const SDeviceInfo &deviceinfo)
{
    return isUEFI() ? (deviceinfo.type == EDisklabelType::GPT) : (deviceinfo.type == EDisklabelType::MsDos);
}

bool CommonFunc::setDisklabelType(const SDeviceInfo &deviceinfo, QString &err)
{
    QString cmd = QString("wipefs -a -f %1").arg(deviceinfo.device);
    if (!command("/bin/bash", {"-c", cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(cmd).arg(err);
        return false;
    }

    qInfo() << QString("Command: %1 succeed.").arg(cmd);

    if (!partprobe(deviceinfo.device, err)) {
        return false;
    }

    switch (deviceinfo.type) {
        case EDisklabelType::GPT: {
            cmd = QString("parted -s %1 mklabel gpt").arg(deviceinfo.device);
        }
        break;
        case EDisklabelType::MsDos: {
            cmd = QString("parted -s %1 mklabel msdos").arg(deviceinfo.device);
        }
        break;

        case EDisklabelType::Others: {
            err = "disklabeltype is empty";
            return false;
        }
        break;
    }

    if (!command("/bin/bash", {"-c", cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(cmd).arg(err);
        return false;
    }

    qInfo() << QString("Command: %1 succeed.").arg(cmd);

    if (!partprobe(deviceinfo.device, err)) {
        return false;
    }

    return true;
}

bool CommonFunc::umountOnDevice(const QString &devicePath, QString &err)
{
    QString cmd = QString("lsblk %1 -npo MOUNTPOINT | awk '{print $1}'").arg(devicePath);
    QStringList listReturn = {};
    if (getCmdListReturn(cmd, listReturn, err)) {
        while(listReturn.size() != 0) {
            for (QString listItem:listReturn) {
                QString umountCmd = QString("umount -R %1").arg(listItem);
                if (!command("/bin/bash", {"-c", umountCmd}, err)) {
                    err = umountCmd + " ERR : " + err;
                    return false;
                }
            }
            if (!getCmdListReturn(cmd, listReturn, err)) {
                err = cmd + " ERR : " + err;
                return false;
            }
        }
    } else {
        err = cmd + " ERR : " + err;
        return false;
    }
    return true;
}

bool CommonFunc::closeVgOnDevice(const QString &devicePath, QString &err)
{
    // 获取vg name的列表
    QString cmd = "vgdisplay | grep \"VG Name\" | awk '{print $NF}'";
    QStringList vgNames = {};
    if (!getCmdListReturn(cmd, vgNames, err)) {
        err = cmd + " ERR : " + err;
        return false;
    }

    for (QString vgNameItem : vgNames) {
        // 确定设备包含对应的vg
        cmd = QString("lsblk %1 -npl | grep %2 || echo \"\"").arg(devicePath).arg(vgNameItem);
        QStringList vgNamesInDev;
        if (!getCmdListReturn(cmd, vgNamesInDev, err)) {
            err = cmd + " ERR : " + err;
            return false;
        }

        // 反激活对应的vg
        if (vgNamesInDev.size() > 0) {
            cmd = QString("vgchange -an %1").arg(vgNameItem);
            if (!command("/bin/bash", {"-c", cmd}, err)) {
                err = cmd + " ERR : " + err;
                return false;
            }
        }
    }

    return true;
}

bool CommonFunc::closeCryptOnDevice(const QString &devicePath, QString &err)
{
    QString cmd = QString("lsblk %1 -plfno NAME,TYPE | grep \"crypt$\" | awk '{print $1}'").arg(devicePath);
    QStringList cryptList = {};
    if (!getCmdListReturn(cmd, cryptList, err)) {
        err = cmd + " ERR : " + err;
        return false;
    }

    for (QString cryptItem : cryptList) {
        cmd = QString("cryptsetup luksClose %1").arg(cryptItem);
        if (!command("/bin/bash", {"-c", cmd}, err)) {
            err = cmd + " ERR : " + err;
            return false;
        }
    }
    return true;
}


bool CommonFunc::clearDeviceStatus(const QString &devicePath, QString &err)
{
    // 卸载磁盘上的挂载点
    if (!umountOnDevice(devicePath, err)) {
        return false;
    }

    // 反激活磁盘上的vg信息
    if (!closeVgOnDevice(devicePath, err)) {
        return false;
    }

    // 关闭加密卷占用
    if (!closeCryptOnDevice(devicePath, err)) {
        return false;
    }

    return true;
}

bool CommonFunc::checkDeviceExist(const QString &devicePath, QString &err)
{
    QString cmd = QString("lsblk -plno NAME | grep %1$").arg(devicePath);
    int count = 5;
    while (count--) {
        if (command("/bin/bash", {"-c", cmd}, err)) {
            return true;
        } else {
            qWarning() << QString("%1 not found, retry...").arg(devicePath);
            QThread::sleep(1);
            continue;
        }
    }
    return count > 0;
}

bool CommonFunc::editPartition(SPartitionInfo &info, QString &err)
{
    if (!checkDeviceExist(info.path, err)) {
        qWarning() << err;
        return true;
    }

    // 卸载磁盘上的挂载点
    if (!umountOnDevice(info.path, err)) {
        return false;
    }

    // 关闭加密卷占用
    if (!closeCryptOnDevice(info.path, err)) {
        return false;
    }

    // 编辑分区
    if (info.partitionType == EPartitionType::PartitionTypeExtension) {
        QString cmd = QString("fedora-dde-installer-parted-util %1 %2 %3 %4 %5")
                .arg(info.device)
                .arg(info.path)
                .arg("extended")
                .arg(info.startPoint)
                .arg(info.endPoint);

        if (!command("/bin/bash", {"-c", cmd}, err)) {
            err = QString("Command: %1 failed. %2").arg(cmd).arg(err);
            return false;
        }

        qInfo() << QString("Command: %1 succeed.").arg(cmd);

        return true;
    }

    QString bak = info.path;
    info.path = LibpartedManager::instance()->getPartitionPath(info);

    if (info.path != bak) {
        qInfo() << QString("Partition [%1s ~ %2s] path be changed from %3 to %4")
                   .arg(info.startPoint)
                   .arg(info.endPoint)
                   .arg(bak)
                   .arg(info.path);
    }

    if (info.isNeedFormat) {
        if (!formatPartitionOK(info, err)) {
            return false;
        }
    }

    return true;
}

bool CommonFunc::creatPartition(SPartitionInfo &info, QString &err)
{
    // 保存创建分区前的分区路径列表
    QStringList oldPaths;
    if (!CommonFunc::getDevicePaths(info.device, oldPaths, err)) {
        err = QString("Call getDevicePaths failed. %1").arg(err);
        return false;
    }

    QString cmd = QString("parted -s %1 mkpart %2 %3 %4s %5s")
            .arg(info.device)
            .arg(CommonFunc::getPartitionTypeName(info.partitionType))
            .arg(info.filesystem == "ntfs" ? "ntfs" : "")
            .arg(info.startPoint)
            .arg(info.endPoint);

    if (!command("/bin/bash", {"-c", cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(cmd).arg(err);
        return false;
    }

    if (!partprobe(info.device, err)) {
        return false;
    }

    qInfo() << QString("Command: %1 succeed.").arg(cmd);

    // 获取创建的分区路径
    QStringList newPaths;
    if (!CommonFunc::getDevicePaths(info.device, newPaths, err)) {
        err = QString("Call getDevicePaths failed. %1").arg(err);
        return false;
    }

    QStringList paths = {};
    for (QString path : newPaths) {
        if (!oldPaths.contains(path)) {
            paths.append(path);
        }
    }

    if (paths.count() != 1) {
        err = QString("Device paths data error, old paths: [%1], new paths: [%2], subtract paths: [%3].")
                .arg(oldPaths.join(","))
                .arg(newPaths.join(","))
                .arg(paths.join(","));
        return false;
    }

    info.path = paths.first();

    if (info.partitionType != EPartitionType::PartitionTypeExtension && info.isNeedFormat) {
        if (!formatPartitionOK(info, err)) {
            return false;
        }
    }

    int timeout = 0;
    while (timeout < 3000) {
        // 更新分区的partuuid
        cmd = QString("lsblk %1 -lpno NAME,PARTUUID | awk '{print $NF}'").arg(info.path);
        if (!command("/bin/bash", {"-c", cmd}, err)) {
            err = QString("Command: %1 failed. %2").arg(cmd).arg(err);
            info.partUuid = "";
        } else {
            info.partUuid = err.trimmed();
        }

        if (!info.partUuid.isEmpty()) {
            qInfo() << QString("Command: %1 succeed.partuuid is: %2").arg(cmd).arg(info.partUuid);
            break;
        }
        timeout += 100;
        QThread::msleep(100);
    }

    if (timeout == 3000) {
        return false;
    }
    return true;
}

bool CommonFunc::deletePartition(SPartitionInfo &info, QString &err)
{
    if (!checkDeviceExist(info.path, err)) {
        qWarning() << QString("Device %1 not exist").arg(info.path);
        return true;
    }

    // 清理设备状态
    if (!clearDeviceStatus(info.path, err)) {
        return false;
    }

    // 删除分区
    QString cmd = QString("parted -s %1 rm %2").arg(info.device).arg(info.index);
    if (!command("/bin/bash", {"-c", cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(cmd).arg(err);
        return false;
    }

    qInfo() << QString("Command: %1 succeed.").arg(cmd);
    return true;
}

bool CommonFunc::formatPartition(SPartitionInfo &info, QString &err)
{
    QMap<QString, QString> cmdMap;
    if (!info.filesystem.isEmpty()) {
        // TODO(chenxiong): 对于label是有命名规范的，比如长度和字符集，此事有待完善
        cmdMap.insert("btrfs",       QString("mkfs.btrfs -f [-L %1] %2"));
        cmdMap.insert("efi",         QString("mkfs.vfat -F32 -s 1 -v [-n %1] %2"));
        cmdMap.insert("ext2",        QString("mkfs.ext2 -F [-L %1] %2"));
        cmdMap.insert("ext3",        QString("mkfs.ext3 -F [-L %1] %2"));
        cmdMap.insert("ext4",        QString("mkfs.ext4 -F [-L %1] %2"));
        cmdMap.insert("f2fs",        QString("mkfs.f2fs -f [-l %1] %2"));
        cmdMap.insert("fat16",       QString("mkfs.fat -F16 -v -I [-n %1] %2"));
        cmdMap.insert("fat32",       QString("mkfs.fat -F32 -v -I [-n %1] %2"));
        cmdMap.insert("hfs",         QString("/usr/bin/hformat -f [-l %1] %2"));
        cmdMap.insert("hfsplus",     QString("/usr/bin/hpfsck -f [-v %1] %2"));
        cmdMap.insert("jfs",         QString("mkfs.jfs -q [-L %1] %2"));
        cmdMap.insert("nilfs2",      QString("mkfs.nilfs2 -f [-L %1] %2"));
        cmdMap.insert("ntfs",        QString("mkfs.ntfs -Q -v -F [-L %1] %2"));
        cmdMap.insert("reiser4",     QString("mkfs.reiser4 --force --yes [--label %1] %2"));
        cmdMap.insert("reiserfs",    QString("mkfs.reiserfs -f [--label %1] %2"));
        cmdMap.insert("xfs",         QString("mkfs.xfs -f [-L %1] %2"));
        cmdMap.insert("recovery",    QString("mkfs.ext4 -F [-L %1] %2"));
        cmdMap.insert("linux-swap", QString("mkswap -f [-L %1] %2"));
        cmdMap.insert("swap",       QString("mkswap -f [-L %1] %2"));
        cmdMap.insert("vfat",       QString("mkfs.vfat -F32 [-n %1] %2"));
        cmdMap.insert("ext4_ls",    QString("mkfs.ext4 -O ^64bit -F [-L %1] %2"));
        cmdMap.insert("default",    QString("mkfs -t %1 ").arg(info.filesystem) + "[-L %1] %2");
        cmdMap.insert(gLVM2PVFileSystem, "");
        cmdMap.insert(gCryptoLuksFileSystem, "");


        QString key = info.filesystem;

        if (!cmdMap.contains(info.filesystem)) {
            key = "default";
        } else {
            // 针对ext4文件系统，申威，龙芯这些平台需要特殊处理
            if (info.filesystem == "ext4") {
                if (getCurrentPlatform().contains(QRegularExpression("sw|loongson|loongarch64"))) {
                    key = "ext4_ls";
                }
            } else if (info.filesystem == "vfat" && info.label == gEFILabel) {
                key = "efi";
            }
        }

        if (key == gCryptoLuksFileSystem) {
            if (!CommonFunc::cryptsetupPartition(QByteArray::fromBase64(CommonFunc::getSettingsString("DI_CRYPT_PASSWORD").toLocal8Bit()), info.label, info.path, err)) {
                return false;
            }
        } else if (key == gLVM2PVFileSystem){
            if (!CommonFunc::createPV(info.path, err)) {
                return false;
            }
        } else  {
            qInfo() << QString("Format partition cmd template %1").arg(cmdMap.value(key));

            QString cmd;
            if (CommonFunc::command("which", {"flock"}, err)) {
                cmd = QString("flock -s %1 ").arg(info.path);
            }
            cmd += cmdMap.value(key).arg(info.label, info.path);
            if (key == gLVM2PVFileSystem) {
                cmd = cmdMap.value(key);
            }

            if (info.label.isEmpty()) {
                cmd.replace(QRegularExpression("\\[[^\\[\\]]*\\]"), "");
            } else {
                cmd.replace(QRegularExpression("[\\[|\\]]"), "");
            }

            if (info.isNeedCryptHeader) {
                // 调整分区大小减少32M
                cmd = cmd + QString(" %1M").arg(info.size > 32 ? info.size - 32 : info.size);
            }

            if (!command("/bin/bash", {"-c", cmd}, err)) {
                err = QString("Command: %1 failed. %2").arg(cmd, err);
                return false;
            }

            qInfo() << QString("Command: %1 succeed. out: %2").arg(cmd, err);
        }
    }

    return true;
}

bool CommonFunc::formatPartitionOK(SPartitionInfo &info, QString &err)
{
    bool reset = true;
    CommonFunc::command("udevadm", {"settle", "--timeout=150"}, err);
    for (int i = 0; i < getSettingsInt("DI_FORMAT_PARTITION_CNT_CONFIG"); i++) {
        CommonFunc::command("flock", {"-s", info.path}, err);
        qInfo() << "flock " << info.path;
        if (!formatPartition(info, err)) {
            qWarning() << "formatPartition: " << err;
            reset = false;
            QThread::msleep(1000);
            continue;
        } else {
            return true;
        }
    }

    return reset;
}

bool CommonFunc::formatPartitionOK(SLVMInfo &info, QString &err) {
    SPartitionInfo partitionInfo;
    partitionInfo.label = info.label;
    partitionInfo.path = info.path;
    partitionInfo.filesystem = info.filesystem;
    partitionInfo.mountPoint = info.mountPoint;

    bool result = formatPartitionOK(partitionInfo, err);

    info.label = partitionInfo.label;
    info.path = partitionInfo.path;
    info.filesystem = partitionInfo.filesystem;
    info.mountPoint = partitionInfo.mountPoint;

    return result;
}

bool CommonFunc::getPartitionFileSystem(const QString &path, QString &out)
{
    QString cmd = QString("lsblk -lnf %1 -o FSTYPE").arg(path);
    if (!command("/bin/bash", {"-c", cmd}, out)) {
        out = QString("Command: %1 failed. %2").arg(cmd).arg(out);
        return false;
    }

    qInfo() << QString("Command: %1 succeed. out: %2").arg(cmd, out);
    return true;
}

bool CommonFunc::sync()
{
    QString out;
    QString cmd = QString("sync");
    if (!command("/bin/bash", {"-c", cmd}, out)) {
        out = QString("Command: %1 failed. %2").arg(cmd).arg(out);
        return false;
    }

    qInfo() << QString("Command: %1 succeed.").arg(cmd);
    return true;
}

bool CommonFunc::checkMount(const QString &mountPoint, const QString &device)
{
    if (mountPoint.isEmpty()) {
        return true;
    }

    if (!mountPoint.compare(gSwapMountPoint)) {
        return true;
    }

    QString err;
    if (mountToTarget(device, mountPoint, "", err)) {
        return umount(device, err);
    } else {
        qWarning() << "checkMount: " << err;
        return false;
    }
}

bool CommonFunc::mountToTarget(const QString &device, const QString &mountPoint, const QString &option, QString &err)
{
    QString targetMountPoint = QString("/target/%1").arg(mountPoint);
    targetMountPoint.replace("//", "/");
    return mount(device, targetMountPoint, option, err);
}

bool CommonFunc::mount(const QString &device, const QString &mountPoint, const QString &option, QString &err)
{
    //  挂载前先创建文件路径
    if (!QDir().exists(mountPoint)) {
        if (!QDir().mkpath(mountPoint)) {
            err = QString("mkpath %1 failed").arg(mountPoint);
            return false;
        }
    } else {
        qInfo() << QString("mountPoint %1 is exists not need to create !").arg(mountPoint);
    }

    QString cmd = "mount";
    if(!option.isEmpty()) {
        cmd += " -o ";
        cmd += option;
    }

    cmd.append(QString(" %1 %2").arg(device, mountPoint));
    if (!command("/bin/bash", {"-c", cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(cmd).arg(err);
        return false;
    }

    qInfo() << QString("Command: %1 succeed.").arg(cmd);
    return true;
}

bool CommonFunc::umount(const QString &device, QString &err)
{
    QString cmd = QString("umount \"%1\"").arg(device);
    if (!command("/bin/bash", {"-c", cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(cmd).arg(err);
        return false;
    }

    qInfo() << QString("Command: %1 succeed.").arg(cmd);
    return true;
}

bool CommonFunc::isMounted(const QString &path, QString &err)
{
    QString cmd = QString("lsblk -lnp -o NAME,MOUNTPOINT | grep %1 | awk '{print $2}'").arg(path);
    if (!command("/bin/bash", {"-c", cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(cmd).arg(err);
        return false;
    }

    err.replace("\n", "");
    QString mountPoint = err;
    mountPoint.replace(" ", "");
    return !mountPoint.isEmpty();
}

bool CommonFunc::partprobe(const QString &path, QString &out)
{
    QString cmd = QString("partprobe -s %1").arg(path);
    if (!command("/bin/bash", {"-c", cmd}, out)) {
        out = QString("Command: %1 failed. %2").arg(cmd).arg(out);
        return false;
    }

    qInfo() << QString("Command: %1 succeed. out: %2").arg(cmd, out);
    return true;
}

bool CommonFunc::cryptsetupPartition(const QString password, const QString cryptLabel, QString &pv, QString &err)
{
    static int cryptDeviceNO = 0;

    // 取下一个待加密分区的链接名称
    QString label = QString("luks_crypt%1").arg(cryptDeviceNO++);

    // 加密分区
    QString cmd = QString("echo '%1' | cryptsetup %2 -v luksFormat %3")
            .arg(password)
            .arg(CommonFunc::getSettingsString("DI_DISK_CRYPT_ALGORITHM_CONFIG"))
            .arg(pv);

    if (!command("/bin/bash", {"-c", cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(QString(cmd).replace(QRegularExpression("echo[\\s|\\S]*\\|"), "echo ****** |")).arg(err);
        return false;
    }

    // 设置加密卷标签
    if (!cryptLabel.isEmpty()) {
        cmd = QString("cryptsetup config %1 --label %2").arg(pv).arg(cryptLabel);
        if (!CommonFunc::command("/bin/bash", {"-c", cmd}, err)) {
            err = QString("Command: %1 failed. %2").arg(cmd, err);
            return false;
        }
    }

    qInfo() << QString("Command: %1 succeed.").arg(QString(cmd).replace(QRegularExpression("echo[\\s|\\S]*\\|"), "echo ****** |"));

    // 映射分区
    cmd = QString("echo '%1' | cryptsetup open \"%2\" \"%3\"")
            .arg(password)
            .arg(pv)
            .arg(label);

    pv = QString("/dev/mapper/%1").arg(label);

    if (!command("/bin/bash", {"-c", cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(QString(cmd).replace(QRegularExpression("echo[\\s|\\S]*\\|"), "echo ****** |")).arg(err);
        return false;
    }

    qInfo() << QString("Command: %1 succeed. path: %2").arg(QString(cmd).replace(QRegularExpression("echo[\\s|\\S]*\\|"), "echo ****** |"), pv);

    // 恢复密钥和加密盘做绑定
    QString recoveryKeyStr = getSettingsString("DI_CRYPT_RECOVERY_KEY");
    if ((!recoveryKeyStr.isEmpty()) && (!CommonFunc::bindRecoveryKeyToDev(password, {pv}, recoveryKeyStr, err))) {
        return false;
    }

    return true;
}

bool CommonFunc::findCryptVolum(const QString &partPath, QString &out, QString &err)
{
    // 获取所有加密卷
    QString cmd = QString("lsblk -lnp -o PATH,TYPE | grep crypt | awk '{print $1}' | tr '\n' ';'");
    QString cmdOut = "";
    if (!command("/bin/bash", {"-c", cmd}, cmdOut)) {
        err = QString("Command: %1 failed. %2").arg(cmd).arg(cmdOut);
        out = "";
        return false;
    }

    QStringList cryptVolumDev = qstringSplitToQStringList(cmdOut, ";");
    for (const QString &devItem : cryptVolumDev) {
        cmd = QString("cryptsetup status \"%1\" | grep \"device\" | awk '{print $2}'").arg(devItem);
        cmdOut = "";
        if (!command("/bin/bash", {"-c", cmd}, cmdOut)) {
            err = QString("Command: %1 failed. %2").arg(cmd).arg(cmdOut);
            out = "";
            return false;
        }

        cmdOut.replace("\n", "");
        cmdOut.replace(" ", "");

        // 从加密卷中解析出来的设备路径包含传入的设备路径，则说明该加密卷被使用
        if (cmdOut.contains(partPath)) {
            out = devItem;
            break;
        }
    }

    return !out.isEmpty();
}

bool CommonFunc::findCryptVolumByLVM(const QString &partPath, QString &out, QString &err)
{
    // 获取所有加密卷
    QString cmd = QString("lsblk -lnp -o PATH,TYPE | grep crypt | awk '{print $1}' | tr '\n' ';'");
    QString cmdOut = "";
    if (!command("/bin/bash", {"-c", cmd}, cmdOut)) {
        err = QString("Command: %1 failed. %2").arg(cmd).arg(cmdOut);
        out = "";
        return false;
    }
    QStringList cryptVolumDev = qstringSplitToQStringList(cmdOut, ";");
    for (const QString &devItem : cryptVolumDev) {
        cmd = QString("lsblk -pln \"%1\" | grep \"%2\" || echo \"none\"").arg(devItem).arg(partPath);
        cmdOut = "";
        if (!command("/bin/bash", {"-c", cmd}, cmdOut)) {
            err = QString("Command: %1 failed. %2").arg(cmd).arg(cmdOut);
            out = "";
            return false;
        }

        cmdOut.replace("\n", "");
        cmdOut.replace(" ", "");

        // 从加密卷中的分区中传入的设备路径，则说明该加密卷被使用
        if (cmdOut.compare("none")) {
            out = devItem;
            break;
        }
    }

    return !out.isEmpty();
}

bool checkLvmInfoComplete(const QString &devPath)
{
    QString output;
    if(!CommonFunc::command("/bin/bash", {"-c", QString("lsblk -JO %1").arg(devPath)}, output)) {
        qInfo() << "cmd error:" << output;
        return false;
    }

    QJsonDocument devDoc = QJsonDocument::fromJson(output.toLocal8Bit());
    if(devDoc.isNull()) {
        qInfo() << "parse json error";
        return false;
    }

    QJsonObject devObj = devDoc.object();
    auto devArray = devObj["blockdevices"].toArray();
    QJsonArray childArray = devArray.first().toObject().value("children").toArray();
    if(childArray.size() <= 0) {
        qInfo() << "not find children";
        return false;
    }

    for (int i = 0; i < childArray.size(); ++i) {
        auto childObj = childArray[i].toObject();
        if(childObj["name"].toString().isEmpty() ||
           childObj["uuid"].toString().isEmpty() ||
           childObj["path"].toString().isEmpty()) {
            return false;
        }
    }

    qInfo() << "check success" << childArray.first()["label"].toString();
    return true;
}

QJsonObject CommonFunc::getPartitionInfo(const QString &path, QString &err) {
    QJsonObject partInfo;
    QString cmd = QString("lsblk -p -OJ %1").arg(path);
    QString deviceInfoString = "";
    if ( ! CommonFunc::command("/bin/bash", {"-c", cmd}, deviceInfoString) ) {
        err = QString("run cmd err:  %1").arg(cmd);
        return partInfo;
    }

    QJsonParseError jsonerror;
    QJsonObject devInfoObj = QJsonDocument::fromJson(deviceInfoString.toLocal8Bit(), &jsonerror).object();
    if (jsonerror.error != QJsonParseError::NoError) {
        err = QString("json parse error: %1").arg(deviceInfoString);
        return partInfo;
    }

    if(!devInfoObj.contains("blockdevices")) {
        err = QStringLiteral("not find block device");
        return partInfo;
    }

    QJsonArray blockArray = devInfoObj["blockdevices"].toArray();
    if(blockArray.isEmpty()) {
        err = QStringLiteral("block device is empty");
        return partInfo;
    }

    partInfo = blockArray.first().toObject();
    return partInfo;
}

bool CommonFunc::deCryptPartition(const QString password, QString &path, QString &err)
{
    // 获取加密卷和解密状态
    QString cmd = QString("lsblk %1 -p -OJ").arg(path);
    QString deviceInfoString = "";
    if ( ! CommonFunc::command("/bin/bash", {"-c", cmd}, deviceInfoString) ) {
        err = QString("find crypt dev failed! err:  %1").arg(deviceInfoString);
        return false;
    }

    QJsonParseError jsonerror;
    QJsonObject devInfoObj = QJsonDocument::fromJson(deviceInfoString.toStdString().data(), &jsonerror).object();
    if (jsonerror.error != QJsonParseError::NoError) {
        err = QString("read crypt dev info failed! ");
        return false;
    }

    QJsonArray deviceArray = devInfoObj.value("blockdevices").toArray();

    QString cryptPartPath = "";
    QString cryptDevPath = "";
    bool deCryptStatus = false;
    for (int i = 0; i < deviceArray.size(); i++) {
        QJsonObject deviceObj = deviceArray.at(i).toObject();
        if ( deviceObj.value("fstype").toString() == "crypto_LUKS") {
            cryptPartPath = deviceObj.value("path").toString();
            cryptDevPath = deviceObj.value("pkname").toString();
            // children为空则说明没有解密
            deCryptStatus = deviceObj.value("children").toArray().size() > 0 ? true : false;
            break;
        }
    }

    // 如果是没有解密的加密卷，则进行解密操作
    if ((!cryptPartPath.isEmpty()) && (!deCryptStatus)) {
        cmd = QString("echo '%1' | base64 -d | cryptsetup luksOpen \"%2\" \"luks_crypt%3\"").arg(password).arg(cryptPartPath).arg(cryptPartPath.replace("/",""));
        if ( ! CommonFunc::command("/bin/bash", {"-c", cmd}, err) ) {
            err = QString("decrypt failed !   cmd :  %1").arg(cmd);
            qInfo() << "decrypt failed:" << err;
            return false;
        } else {
            qInfo() << "decrypt success";
            cmd = QString("partprobe %1").arg(cryptDevPath);
            CommonFunc::command("/bin/bash", {"-c", cmd}, err);
        }

        path = QString("/dev/mapper/luks_crypt%1").arg(cryptPartPath.replace("/",""));
    } else {
        err = QString("crypt dev is empty !");
        return false;
    }
    //check decrypt path
    QJsonObject decryptPartInfo;
    for (int i = 0; i < 10; ++i) {
        decryptPartInfo = getPartitionInfo(path, err);
        if(!decryptPartInfo.isEmpty()) {
            break;
        }

        QThread::msleep(100);
    }

    if(decryptPartInfo.isEmpty()) {
        qInfo() << "get decrypt partition info failed";
        return false;
    }

    if(decryptPartInfo["fstype"].toString() == "LVM2_member") {
        //wait for refresh data
        int n = 0;
        for (; n < LSBLK_RETRYNUM; ++n) {
            if(checkLvmInfoComplete(path)) {
                break;
            }

            QThread::msleep(100);
        }

        return n < LSBLK_RETRYNUM;
    } else {
        qInfo() << "partition decrypt is not lvm:" << decryptPartInfo["fstype"].toString();
        return true;
    }
}

QStringList CommonFunc::getCryptDevPaths()
{
    // 获取加密卷和解密状态
    QString cmd = "lsblk -lp -OJ";
    QString deviceInfoString = "";
    if ( ! CommonFunc::command("/bin/bash", {"-c", cmd}, deviceInfoString) ) {
        return QStringList();
    }

    QJsonParseError jsonerror;
    QJsonObject devInfoObj = QJsonDocument::fromJson(deviceInfoString.toStdString().data(), &jsonerror).object();
    if (jsonerror.error != QJsonParseError::NoError) {
        return QStringList();
    }

    QJsonArray deviceArray = devInfoObj.value("blockdevices").toArray();

    QStringList cryptDevices;
    for (int i = 0; i < deviceArray.size(); i++) {
        QJsonObject deviceObj = deviceArray.at(i).toObject();
        if ( deviceObj.value("fstype").toString() == "crypto_LUKS") {
            cryptDevices.append(deviceObj.value("path").toString());
        }
    }

    return cryptDevices;
}

bool CommonFunc::checkIsPartitionEncrypt(const SPartitionInfo &partInfo)
{
    QString cmd = QString("lsblk -JOb %1").arg(partInfo.path);
    QString output = "";
    if ( ! CommonFunc::command("/bin/bash", {"-c", cmd}, output) ) {
        return false;
    }

    QJsonObject devObj = QJsonDocument::fromJson(output.toUtf8()).object();
    if(devObj.isEmpty()) {
        return false;
    }

    if(!devObj.contains("blockdevices")) {
        return false;
    }

    QJsonArray blockArray = devObj["blockdevices"].toArray();
    if(blockArray.isEmpty()) {
        return false;
    }

    QJsonObject device = blockArray.at(0).toObject();
    if(!device.contains("children")) {
        return false;
    }

    QJsonObject childDev = device["children"].toArray().at(0).toObject();
    return !childDev.contains("children");
}

bool CommonFunc::mountPointWriteable(const QString &mountPoint)
{
    if (mountPoint.compare("")) {
        QFileInfo mountFile(mountPoint);
        if (mountFile.isWritable()) {
            return true;
        }
    }
    return false;
}

bool CommonFunc::isPartContains(const SPartitionInfo &extension, const SPartitionInfo &free)
{
    qint64 offset = mToSectors(1, free.sectorSize);
    if (extension.startPoint - offset <= free.startPoint  &&
        extension.endPoint + offset >= free.endPoint) {
        return true;
    }

    return false;
}

bool CommonFunc::isPartContinuous(const SPartitionInfo &fir, const SPartitionInfo &sec)
{
    qint64 oneMib = mToSectors(1, fir.sectorSize);
    if ((abs(fir.startPoint - sec.endPoint) <= oneMib) ||
        (abs(sec.startPoint - fir.endPoint) <= oneMib)) {
        return true;
    }

    return false;
}

qint64 CommonFunc::alignPE(qint64 lvSize, qint64 peSize, qint64 peTotal)
{
    return qMin(lvSize, peSize * peTotal);
}

bool CommonFunc::getDevicePaths(const QString &device, QStringList &paths, QString &err)
{
    QStringList cmdList = {
        QString("partprobe %1").arg(device),
        QString("fdisk -l %1 | awk '{print $1}'").arg(device)
    };

    for (auto cmd : cmdList) {
        if (!command("/bin/bash", {"-c", cmd}, err)) {
            err = QString("Command: %1 failed. %2").arg(cmd).arg(err);
            return false;
        }
    }

    for (auto it : err.trimmed().split("\n")) {
        if (it.contains(device)) {
            paths << it;
        }
    }

    return true;
}

QString CommonFunc::getPartitionTypeName(EPartitionType type)
{
    return QMap<EPartitionType, QString>{
        {EPartitionType::PartitionTypeLogical, gLogical},
        {EPartitionType::PartitionTypeExtension, gExtended},
        {EPartitionType::PartitionTypePrimary, gPrimary}
    }.value(type);
}

QString CommonFunc::getPartitionOperateName(EPartitionOperate operate)
{
    return QMap<EPartitionOperate, QString>{
        {EPartitionOperate::PartitionCreate, gNewOption},
        {EPartitionOperate::PartitionDelete, gDeleteOption},
        {EPartitionOperate::PartitionEdit, gEditOption}
    }.value(operate);
}

QList<SDeviceInfo> CommonFunc::sortDevice(const QList<SDeviceInfo> &devs)
{
    QList<SDeviceInfo> reset;

    // 磁盘分类
    QList<SDeviceInfo> nvmeList;
    QList<SDeviceInfo> ssdList;
    QList<SDeviceInfo> hddList;
    QList<SDeviceInfo> hddUsbList;
    QList<SDeviceInfo> otherList;
    for (SDeviceInfo d : devs) {
        if (!d.rota && d.transporttype == "nvme") {
            nvmeList.append(d);
        }  else if (!d.rota && d.transporttype == "sata") {
            ssdList.append(d);
        } else if (d.rota && d.transporttype == "sata") {
            hddList.append(d);
        } else if (d.rota && d.transporttype == "usb") {
            hddUsbList.append(d);
        } else {
            otherList.append(d);
        }
    }

    // 同一类磁盘按照大小排序，容量大的排在前面
    auto sortFunc = [=](const SDeviceInfo &fir, const SDeviceInfo &sec){
        return fir.size > sec.size;
    };
    std::sort(nvmeList.begin(), nvmeList.end(), sortFunc);
    std::sort(ssdList.begin(), ssdList.end(), sortFunc);
    std::sort(hddList.begin(), hddList.end(), sortFunc);
    std::sort(hddUsbList.begin(), hddUsbList.end(), sortFunc);
    std::sort(otherList.begin(), otherList.end(), sortFunc);
    reset = nvmeList + ssdList + hddList + otherList + hddUsbList;
    return reset;
}

QString CommonFunc::readFile(const QString &path)
{
    QFile file(path);
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "ReadFile() failed to open" << path;
            return "";
        }
        const QString &str = QString::fromUtf8(file.readAll());
        file.close();
        return str;
    }
    else {
        qWarning() << "ReadFileContent() file not found: " << path;
        return "";
    }
}

QPixmap CommonFunc::getRoundRectPixmap(QPixmap srcPixMap, const QSize & size, int radius)
{
    //不处理空数据或者错误数据
    if (srcPixMap.isNull()) {
        return srcPixMap;
    }

    //获取图片尺寸
    int imageWidth = size.width();
    int imageHeight = size.height();

    //处理大尺寸的图片,保证图片显示区域完整
    QPixmap newPixMap = srcPixMap.scaled(imageWidth, (imageHeight == 0 ? imageWidth : imageHeight), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QPixmap destImage(imageWidth, imageHeight);
    destImage.fill(Qt::transparent);
    QPainter painter(&destImage);
    // 抗锯齿
    painter.setRenderHints(QPainter::Antialiasing, true);
    // 图片平滑处理
    painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
    // 将图片裁剪为圆角
    QPainterPath path;
    QRect rect(0, 0, imageWidth, imageHeight);
    path.addRoundedRect(rect, radius, radius);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, imageWidth, imageHeight, newPixMap);
    return destImage;
}

const QPixmap CommonFunc::renderPixmap(const QString &path)
{
    static QHash<QByteArray, QPair<qreal, QPixmap>> PIXMAP_CACHE;
    const QByteArray& md5 { QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Md5) };

    if (PIXMAP_CACHE.contains(md5)) {
        const QPair<qreal, QPixmap>& result = PIXMAP_CACHE[md5];
        if (qFuzzyCompare(result.first, qApp->devicePixelRatio())) {
            return result.second;
        }
    }

    QImageReader reader;
    QPixmap pixmap;
    reader.setFileName(path);
    if (reader.canRead()) {
        const qreal ratio = qApp->devicePixelRatio();
        reader.setScaledSize(reader.size() * ratio);
        pixmap = QPixmap::fromImage(reader.read());
        pixmap.setDevicePixelRatio(ratio);
    }
    else {
        pixmap.load(path);
    }

    PIXMAP_CACHE[md5] = { qApp->devicePixelRatio(), pixmap };

    return pixmap;
}

QString CommonFunc::getUIDefaultFont() {
    // TODO: maybe make this a configurable value in the future? since other
    // editions may have different fonts installed.
    return getSettingsString("DI_UI_DEFAULT_FONT_CONFIG");
}

bool CommonFunc::oemHasConfigEntry(const QString &entry)
{
    bool bpxemode = false;
    QString output;
    if(CommonFunc::command("/bin/bash", {"-c", "cat /proc/cmdline"}, output)) {
        bpxemode = output.contains("nfsroot");
    }

    QString overrideSettings = QString("%1/os/%2/%3.ini").arg(getSystemEnv(DI_CONFIG_DIR_ENV), getCurrentOsType(), getCurrentPlatform());
    QString oemSettings = QString("%1/oem/settings.ini").arg(getSystemEnv(DI_LIVE_DIR_ENV));
    QString extraSettings = QString("%1/pxemode/settings.ini").arg(getSystemEnv(DI_LIVE_DIR_ENV));
    QString pxeSettings = QString("%1/pxemode/settings.ini-pxe").arg(getSystemEnv(DI_LIVE_DIR_ENV));

    QStringList settingList{overrideSettings, oemSettings, extraSettings};
    if(bpxemode) {
        settingList.append(pxeSettings);
    }

    for (auto &file: settingList) {
        if(QFile::exists(file)) {
            QSettings exSetting(file, QSettings::IniFormat);
            if(exSetting.contains(entry)) {
                qInfo() << "find oem in:" << file << entry;
                return true;
            }
        }
    }

    return false;
}

QStringList CommonFunc::getPartitionMountPoints(const SLVMInfo &free)
{
    QStringList reset = {"/", "/boot", "/home", "/tmp", "/var", "/srv", "/opt", "/usr/local"};

    qint64 rootASize = CommonFunc::getSettingsInt("DI_ROOTA_SIZE_CONFIG");
    if (free.size < rootASize &&
            qAbs(free.size - rootASize) > gPartitionDelta) {
        reset.removeAll("/");
    }

    reset.removeAll("/boot");   // 当前引导分区不支持lvm不支持
    reset.append(getEmptyMountPoint());

    return reset;
}

QStringList CommonFunc::getPartitionMountPoints(const SPartitionInfo &free)
{
    QStringList reset = {"/", "/boot", "/home", "/tmp", "/var", "/srv", "/opt", "/usr/local"};

    if (isUEFI()) {
        reset.append("/boot/efi");
    }

    reset.append(getEmptyMountPoint());

    return reset;
}

QString CommonFunc::getPartitionMountText(const QString text)
{
    QString reset = text;
    if (text.isEmpty()) {
        reset = getEmptyMountPoint();
    }

    return reset;
}

QString CommonFunc::labelToMountPoint(const QString &label)
{
    QString reset;
    if (label == gEFILabel) {
        reset = "/boot/efi";
    } else if (label == gBootLabel) {
        reset = "/boot";
    } else if (label == gRootaLabel || label == gRootLabel) {
        reset = "/";
    } else if (label == gSWAPLabel) {
        reset = "swap";
    } else if (label == gDataLabel) {
        reset = CommonFunc::getSettingsString("DI_DATA_MOUNT_POINT_CONFIG", CommonFunc::getCurrentOsVersion());
    } else if (label == gBackupLabel) {
        reset = "/recovery";
    }

    return reset;
}

QString CommonFunc::getPartitionMountPoint(const QString text)
{
    QString reset = text;
    if (getEmptyMountPoint() == text) {
        reset = QString();
    }

    return reset;
}

void CommonFunc::setLabelTextAndTooltip(QLabel *label, const QString& strText, const int& textwidth)
{
    // 字符省略号思路的来源是AutoElideLabel控件
    QFontMetrics fontWidth(label->font());
    QString elideNote = fontWidth.elidedText(strText, Qt::ElideRight, textwidth);
    label->setText(elideNote);
    if (elideNote.indexOf("…") != -1) {
        label->setToolTip(strText);

    } else {
        label->setToolTip("");

    }
}

bool CommonFunc::isStartDisk(const QList<SPartitionInfo> &parts)
{
    for (SPartitionInfo in : parts) {
        // 删除掉目录中的路径分割符后再做比较
        QString mountPoint = QString(in.mountPoint).remove(QDir::separator());
        QString startDiskPath = QString(getSystemEnv(DI_LIVE_DIR_ENV)).remove(QDir::separator());

        if (mountPoint == startDiskPath) {
            return true;
        }
    }

    return false;
}

qint64 CommonFunc::strGetInt(const QString &str)
{
    QRegularExpression rx("(\\d+\\.\\d+|(\\d+))");
    QRegularExpressionMatch match = rx.match(str);
    if (match.hasMatch()) {
        return qint64(match.captured(0).toDouble());
    }

    return 0.0;
}

bool CommonFunc::getLVMPESize(const QString &vgName, qint64 &peSize, QString &err)
{
    QString cmd = QString("vgdisplay %1 | grep -iE \'PE[ ]{0,}Size\'").arg(vgName);

    QString out;
    if (!command("/bin/bash", {"-c", cmd}, out)) {
        err = QString("Command: %1 failed. %2").arg(cmd, out);
        return false;
    }

    qInfo() << QString("Command: %1 succeed. out: %2").arg(cmd, out);
    peSize = strGetInt(out);
    qInfo() << "vgName: " << vgName << "  PE Size:" << peSize;
    return true;
}

bool CommonFunc::getLVMPETotal(const QString &vgName, qint64 &peTotal, QString &err)
{
    QString cmd = QString("vgdisplay %1 | grep -iE \'Total[ ]{0,}PE\'").arg(vgName);

    QString out;
    if (!command("/bin/bash", {"-c", cmd}, out)) {
        err = QString("Command: %1 failed. %2").arg(cmd, out);
        return false;
    }

    qInfo() << QString("Command: %1 succeed. out: %2").arg(cmd, out);
    peTotal = strGetInt(out);
    qInfo() << "vgName: " << vgName << "  PE Size:" << peTotal;
    return true;
}

bool CommonFunc::getLVMFreePETotal(const QString &vgName, qint64 &freePeTotal, QString &err)
{
    QString cmd = QString("vgdisplay %1 | grep -iE \'Free[ ]{0,}PE\'").arg(vgName);

    QString out;
    if (!command("/bin/bash", {"-c", cmd}, out)) {
        err = QString("Command: %1 failed. %2").arg(cmd, out);
        return false;
    }

    qInfo() << QString("Command: %1 succeed. out: %2").arg(cmd, out);
    freePeTotal = strGetInt(out);
    qInfo() << "vgName: " << vgName << "  Free PE Size:" << freePeTotal;
    return true;
}

bool CommonFunc::createPV(const QString &pvPath, QString &err)
{
    QString cmd = QString("pvcreate %1 -ffy").arg(pvPath);

    if (!command("/bin/bash", {"-c", cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(cmd, err);
        return false;
    }

    qInfo() << QString("Command: %1 succeed.").arg(cmd);
    return true;
}

bool CommonFunc::createVG(const SVGInfo &vg, QString &err)
{
    QString cmd = QString("vgcreate %1 %2").arg(vg.name).arg(vg.pvIds.join(" "));

    if (!command("/bin/bash", {"-c", cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(cmd, err);
        return false;
    }

    qInfo() << QString("Command: %1 succeed.").arg(cmd);
    return true;
}

bool CommonFunc::deleteVG(const SVGInfo &vg, QString &err)
{
    // 查找vg下面所有的lvm
    QString cmd = QString("lablk -lpno NAME,TYPE | grep lvm | grep %1 | awk '{print $1}'").arg(vg.name);
    QStringList lvms = {};
    if (!getCmdListReturn(cmd, lvms, err)) {
        return false;
    }

    // 删除vg下面所有的lvm
    for (const QString &lvmItem : lvms) {
        // 卸载磁盘上的挂载点
        if (!umountOnDevice(lvmItem, err)) {
            return false;
        }

        // 关闭加密卷占用
        if (!closeCryptOnDevice(lvmItem, err)) {
            return false;
        }

        // 删除lvm分区
        cmd = QString("lvremove %1 --yes").arg(lvmItem);
        if (!command("/bin/bash", {"-c", cmd}, err)) {
            err = QString("Command: %1 failed. %2").arg(cmd, err);
            return false;
        }
    }

    // 删除vg
    cmd = QString("vgremove %1 --yes").arg(vg.name);
    if (!command("/bin/bash", {"-c", cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(cmd, err);
        return false;
    }

    qInfo() << QString("Command: %1 succeed.").arg(cmd);

    return true;
}

bool CommonFunc::createLVM(SLVMInfo &info, QString &err)
{
    // 根据PE Size对齐lv的空间大小
    qint64 peSize = 0;
    if (!getLVMPESize(info.vgName, peSize, err)) {
        return false;
    }

    qint64 freePeTotal = 0;
    if (!getLVMFreePETotal(info.vgName, freePeTotal, err)) {
        return false;
    }

    qint64 alignSize = alignPE(info.size, peSize, freePeTotal);
    if (alignSize <= 0) {
        err = QString("lv adjustment error. <lv size:%1> <PE Size:%2> <PE Total:%3> <aling Size:%4>")
                .arg(QString::number(info.size), QString::number(peSize), QString::number(freePeTotal),
                     QString::number(alignSize));
        return false;
    } else {
        info.size = alignSize;
    }

    // 处理lv分区的创建
    QString cmd = QString("lvcreate --wipesignatures y -n %1 -L %2M %3 --yes")
            .arg(info.name)
            .arg(info.size)
            .arg(info.vgName);

    if (!command("/bin/bash", {"-c", cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(cmd, err);
        return false;
    }

    qInfo() << QString("Command: %1 succeed.").arg(cmd);

    info.path = QString("/dev/%1/%2").arg(info.vgName).arg(info.name);

    if (info.isNeedFormat) {
        qInfo() << QString("Need format: %1").arg(info.toString().toStdString().c_str());
        if (!formatPartitionOK(info, err)) {
            return false;
        }
    }

    return true;
}

bool CommonFunc::deleteLVM(const SLVMInfo &info, QString &err)
{
    if (!checkDeviceExist(info.path, err)) {
        qWarning() << err;
        return true;
    }

    // 卸载磁盘上的挂载点
    if (!umountOnDevice(info.path, err)) {
        return false;
    }

    // 关闭加密卷占用
    if (!closeCryptOnDevice(info.path, err)) {
        return false;
    }

    // 删除lvm分区
    QString cmd = QString("lvremove /dev/%1/%2 --yes")
            .arg(info.vgName, info.name);

    if (!command("/bin/bash", {"-c", cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(cmd, err);
        return false;
    }

    qInfo() << QString("Command: %1 succeed.").arg(cmd);

    return true;
}

bool CommonFunc::editLVM(const SLVMInfo &info, QString &err)
{
    if (!checkDeviceExist(info.path, err)) {
        qWarning() << err;
        return true;
    }

    QString path = QString("/dev/%1/%2").arg(info.vgName, info.name);
    if (info.isNeedFormat) {
        // 卸载磁盘上的挂载点
        if (!umountOnDevice(info.path, err)) {
            return false;
        }

        // 关闭加密卷占用
        if (!closeCryptOnDevice(info.path, err)) {
            return false;
        }

        qInfo() << QString("Need format: %1").arg(info.toString().toStdString().c_str());
        SLVMInfo infoFormat = info;
        if (!formatPartitionOK(infoFormat, err)) {
            return false;
        }
    }
    return true;
}

SPartitionInfo CommonFunc::find(const QString& id, const QList<SPartitionOperate> &infos)
{
    for (SPartitionOperate in : infos) {
        if (in.partition.id == id) {
            return in.partition;
        }
    }

    return SPartitionInfo();
}

void CommonFunc::readFSTab(const QString &tabFile, QList<FSTabInfo> &tabList)
{
    QFile file(tabFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "open file failed:" << tabFile;
        return;
    }

    QTextStream readStream(&file);
    QString line;
    while (readStream.readLineInto(&line)) {
        if(!line.isEmpty() && !line.startsWith('#')) {
            QTextStream lineStream(&line);
            FSTabInfo tabInfo;
            lineStream >> tabInfo.spec >> tabInfo.mountPoint >> tabInfo.fstype >>
                    tabInfo.mountOption >> tabInfo.dump >> tabInfo.fsck;
            tabList.append(tabInfo);
        }
    }
}

bool CommonFunc::isFullDiskbyFSTab(const QList<FSTabInfo> &tabList)
{
    bool hasRecovery = false;
    bool hasData = false;
    for (const auto &tabInfo: tabList) {
        if(tabInfo.mountPoint == "/recovery") {
            hasRecovery = true;
        } else if (tabInfo.mountPoint == "/data") {
            hasData = true;
        }
    }

    return hasRecovery && hasData;
}

bool CommonFunc::rebootSystem()
{
    if (!installWorkflow().canRestart())
        return false;
    QString error;
    if (!command("/usr/bin/systemctl", {"reboot"}, error)) {
        qWarning() << "Unable to request restart:" << error;
        return false;
    }
    return true;
}

SPartitionInfo CommonFunc::getEFIPartitionInfo(const SDeviceInfo &dev)
{
    SPartitionInfo info;
    info.device = dev.device;
    info.filesystem = "vfat";
    info.mountPoint = "/boot/efi";
    info.label = gEFILabel;
    info.isStartPoint = true;
    info.index = -1;
    info.sectorSize = dev.sectors;
    info.phySectorSize = dev.physectors;
    info.size = CommonFunc::getSettingsInt("DI_EFI_SIZE_CONFIG");
    info.byteSize = CommonFunc::mToSectors(info.size, info.sectorSize) * info.sectorSize;
    info.isEFIPartition = true;

    info.partitionType = EPartitionType::PartitionTypePrimary;

    return info;
}

SPartitionInfo CommonFunc::getBootPartitionInfo(const SDeviceInfo &dev)
{
    SPartitionInfo info;
    info.id = QUuid::createUuid().toString();
    info.usage = ESpaceUsage::USE;
    info.device = dev.device;
    info.deviceSize = dev.size;
    info.filesystem = "ext4";
    info.mountPoint = "/boot";
    info.label = gBootLabel;
    info.isStartPoint = true;
    info.sectorSize = dev.sectors;
    info.phySectorSize = dev.physectors;
    info.index = -1;
    info.size = CommonFunc::getSettingsInt("DI_BOOT_SIZE_CONFIG");
    info.byteSize = info.size * giDeviceUnitSize * giDeviceUnitSize;

    info.partitionType = EPartitionType::PartitionTypePrimary;

    return info;
}

QString CommonFunc::bootloaderToString(bool isUefi)
{
    return QString(isUefi ? "UEFI" : "legacy");
}

//QJsonObject转枚举 new;delete;edit
EPartitionOperate CommonFunc::qstringToEnumOperate(const QString &strParttitonOperate)
{
    EPartitionOperate partitionOperate = EPartitionOperate();
    if (!strParttitonOperate.isEmpty()) {
        if (!strParttitonOperate.compare(gNewOption)) {
            partitionOperate = EPartitionOperate::PartitionCreate;
        } else if (!strParttitonOperate.compare(gDeleteOption)) {
            partitionOperate = EPartitionOperate::PartitionDelete;
        } else if (!strParttitonOperate.compare(gEditOption)) {
            partitionOperate = EPartitionOperate::PartitionEdit;
        } else if (!strParttitonOperate.compare(gNormalOption)) {
            partitionOperate = EPartitionOperate::PartitionNormal;
        }
    }

    return partitionOperate;
}

//QJsonObject转枚举 primary;logical,;extended
EPartitionType CommonFunc::qstringToEnumType(const QString &type)
{
    QMap<QString, EPartitionType> map = {
        {gPrimary, EPartitionType::PartitionTypePrimary},
        {gLogical, EPartitionType::PartitionTypeLogical},
        {gExtended, EPartitionType::PartitionTypeExtension}
    };

    if (!map.contains(type)) {
        qWarning() << QString("Invalid partition type %1").arg(type);
        return EPartitionType();
    }

    return map.value(type);
}

QString CommonFunc::enumOperateToString(EPartitionOperate parttitonOperate)
{
    switch (parttitonOperate) {
        case EPartitionOperate::PartitionCreate: return gNewOption;
        case EPartitionOperate::PartitionDelete: return gDeleteOption;
        case EPartitionOperate::PartitionEdit: return gEditOption;
        default: return QString();
    }
}

QString CommonFunc::enumTypeToString(EPartitionType parttitonType)
{
    switch (parttitonType) {
        case EPartitionType::PartitionTypePrimary: return gPrimary;
        case EPartitionType::PartitionTypeLogical: return gLogical;
        case EPartitionType::PartitionTypeExtension: return gExtended;
        default: return QString();
    }
}

QString CommonFunc::getPartitionLabel(const SPartitionInfo &part)
{
    QString label;
    if (part.mountPoint.isEmpty()) {
        if (part.label.isEmpty()) {
            static long num = getLabelStartNum();
            label = QString("inst_%1").arg(QString::number(num++));
        } else {
            label = part.label;
        }
    }
    else if (part.mountPoint == "/") {
        label = gRootLabel;

    } else if (part.mountPoint == "/boot") {
        label = gBootLabel;

    } else if (part.mountPoint == "/boot/efi") {
        label = gEFILabel;

    } else if (part.mountPoint == "/persistent" || part.mountPoint == "/data") {
        label = gDataLabel;

    } else if (part.mountPoint == "swap") {
        label = gSWAPLabel;

    } else if (part.mountPoint == "/recovery") {
        label = gBackupLabel;

    } else {
        if (part.label.isEmpty()) {
            if (part.mountPoint == "/home") {
                label = QString("_dde_%1").arg(QString(part.mountPoint).replace("/", "_")).replace("__", "_").mid(0, 12);
            } else {
                label = QString("dde_%1").arg(QString(part.mountPoint).replace("/", "_")).replace("__", "_").mid(0, 12);
            }
        } else {
            label = part.label;
        }
    }

    if (gSwapFileSystems.contains(part.filesystem)) {
        label = gSWAPLabel;
    }

    if (part.partitionType == EPartitionType::PartitionTypeExtension) {
        label = "";
    }

    return label;
}

long CommonFunc::getLabelStartNum() {
    long beginNum = 1;
    QString output;
    QString cmd = QString("lsblk -no LABEL | grep -E \"^inst_[0-9]+$\"");
    if (!CommonFunc::command("/bin/bash", {"-c", cmd}, output)) {
        qWarning() << QString("Command: %1 failed. %2").arg(cmd, output);
    } else {
        QStringList result = qstringSplitToQStringList(output, "\n");
        result.sort();
        QString lastNum = result.last();
        beginNum = lastNum.mid(lastNum.indexOf("_") + 1).toLong() + 1;
    }

    return beginNum;
}

QString CommonFunc::getPartitionLabel(const SLVMInfo &part)
{
    QString label;
    if (part.mountPoint.isEmpty()) {
        if (part.label.isEmpty()) {
            static long num = getLabelStartNum();
            label = QString("inst_%1").arg(QString::number(num++));
        } else {
            label = part.label;
        }
    }
    else if (part.mountPoint == "/") {
        label = gRootLabel;

    } else if (part.mountPoint == "/boot") {
        label = gBootLabel;

    } else if (part.mountPoint == "/boot/efi") {
        label = gEFILabel;

    } else if (part.mountPoint == "/persistent" || part.mountPoint == "/data") {
        label = gDataLabel;

    } else if (part.mountPoint == "/recovery") {
        label = gBackupLabel;

    } else if (part.mountPoint == "swap") {
        label = gSWAPLabel;

    } else {
        if (part.label.isEmpty()) {
            if (part.mountPoint == "/home") {
                label = QString("_dde_%1").arg(QString(part.mountPoint).replace("/", "_")).replace("__", "_").mid(0, 12);
            } else {
                label = QString("dde_%1").arg(QString(part.mountPoint).replace("/", "_")).replace("__", "_").mid(0, 12);
            }
        } else {
            label = part.label;
        }
    }

    if (gSwapFileSystems.contains(part.filesystem)) {
        label = gSWAPLabel;
    }

    return label;
}

QString CommonFunc::spaceUsageToString(ESpaceUsage usage)
{
    if (usage == ESpaceUsage::USE) {
        return "use";
    } else if (usage == ESpaceUsage::FREE) {
        return "free";
    } else {
        return QString();
    }
}

QMap<QString, EPartitionType> CommonFunc::partitionTypeToText(EPartitionType partitionType)
{
    if (partitionTypeToText().values().contains(partitionType)) {
        return QMap<QString, EPartitionType>{{partitionTypeToText().key(partitionType), partitionType}};
    } else {
        return QMap<QString, EPartitionType>();
    }
}

QMap<QString, EPartitionType> CommonFunc::partitionTypeToText()
{
    return QMap<QString, EPartitionType>{
            {::QObject::tr("Primary partition"), EPartitionType::PartitionTypePrimary},
            {::QObject::tr("Logical partition"), EPartitionType::PartitionTypeLogical}
        };
}

QMap<QString, EOpeartType> CommonFunc::opeartTypeToText(EOpeartType opeartType)
{
    if (opeartTypeToText().values().contains(opeartType)) {
        return QMap<QString, EOpeartType>{{opeartTypeToText().key(opeartType), opeartType}};
    } else {
        return QMap<QString, EOpeartType>();
    }
}

QMap<QString, EOpeartType> CommonFunc::opeartTypeToText()
{
    return QMap<QString, EOpeartType>{
            {::QObject::tr("Edit"), EOpeartType::Edit},
            {::QObject::tr("Delete"), EOpeartType::Delete},
            {::QObject::tr("New"), EOpeartType::Add},
            {::QObject::tr("Encrypt"), EOpeartType::Encrypt},
            {::QObject::tr("Decrypt"), EOpeartType::Decrypt}
        };
}

QString CommonFunc::getOpeartTypeText(EOpeartType opeartType)
{
    return opeartTypeToText().key(opeartType);
}

QString CommonFunc::boolToString(bool value)
{
    return value ? "true" : "false";
}

QStringList CommonFunc::getAvatars()
{
    QStringList ret;
    QString avatarConfig = CommonFunc::getSettingsString("DI_DDE_AVATAR_DIR_CONFIG");
    QString osVersion = CommonFunc::getCurrentOsVersion();
    if (osVersion == "V23" || osVersion == "V25") {
        QStringList avatarDirs;
        avatarDirs.append(avatarConfig + "/animal");
        avatarDirs.append(avatarConfig + "/emoji");
        avatarDirs.append(avatarConfig + "/human");
        avatarDirs.append(avatarConfig + "/illustration");

        for (auto avatarDir : avatarDirs) {
            for (auto it : getDirs(avatarDir, {"*.png", "*.jpg"})) {
                if (!it.contains("default.") && !it.contains("guest.")) {
                    ret << it;
                }
            }
        }
    } else {
        // 忽略 "default.png" 和 "guest.png"
        for (auto it : getDirs(avatarConfig, {"*.png", "*.jpg"})) {
            if (!it.contains("default.") && !it.contains("guest.")) {
                ret << it;
            }
        }
    }

    return ret;
}

QString CommonFunc::get2AlphaCountryCode(const QString &locale)
{
    int index = locale.indexOf('_');
    if (index < 0) {
        return QString();
    }

    return locale.mid(index + 1).toUpper();
}

EDisklabelType CommonFunc::qstringToEnumDisklabel(const QString &strDisklabel)
{
    EDisklabelType disklabeltype = EDisklabelType();
    // 读取磁盘格式信息
    if ( !strDisklabel.compare("gpt") ) {
        disklabeltype = EDisklabelType::GPT;
    } else if (!strDisklabel.compare("dos") || !strDisklabel.compare("msdos")) {
        disklabeltype = EDisklabelType::MsDos;
    } else {
        disklabeltype = EDisklabelType::Others;
    }
    return disklabeltype;
}

QString CommonFunc::diskLabelToString(EDisklabelType type)
{
    // 读取磁盘格式信息
    if (type == EDisklabelType::GPT) {
        return "gpt";
    } else if (type == EDisklabelType::MsDos) {
        return "msdos";
    } else {
        return "Others";
    }
}

QPixmap CommonFunc::loadSvgImage(const QString& strPath, int iWidth, int iHeight)
{
    QPixmap origPix = QPixmap(strPath);

    if (!origPix.isNull()) {
        origPix = origPix.scaled(iWidth, iHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QWidget testwidget;
        origPix.setDevicePixelRatio(testwidget.devicePixelRatioF());
    }

    return origPix;
}

QPixmap CommonFunc::renderSVG(const QString &path, const QSize &size)
{
    QImageReader reader;
    QPixmap pixmap;
    reader.setFileName(path);
    if (reader.canRead()) {
        const qreal ratio = qApp->devicePixelRatio();
        reader.setScaledSize(size * ratio);
        pixmap = QPixmap::fromImage(reader.read());
        pixmap.setDevicePixelRatio(ratio);
    }
    else {
        pixmap.load(path);
    }
    return pixmap;
}

const QPixmap CommonFunc::hidpiPixmap(const QString &path, const QSize &sz)
{
    const auto ratio = qApp->devicePixelRatio();
    QPixmap iconPix = renderSVG(path, sz);
    iconPix.setDevicePixelRatio(ratio);

    return iconPix;
}

EGhostCode CommonFunc::checkUimagePath(const QString &uimagePath, const QString &devs, QString &out)
{
    QProcess process;
    QString cmd = getSettingsString("DI_GHOST_EXEC_CONFIG");
    QStringList args = {"-m", "check", "-f", uimagePath, "-d", devs};
    process.setProgram(cmd);
    process.setArguments(args);

    process.start();
    // Wait for process to finish without timeout.
    if (!process.waitForStarted(-1)) {
        out = QString("Command: %1 failed. QProcess::ProcessError:%2. ").arg(cmd + args.join(" ")).arg(process.error());
        return EGhostCode::Forbid;
    }

    process.waitForFinished(-1);
    out = QString("Command: %1 failed. info: %2 ; waring: %3").arg(cmd + args.join(" "), QString(process.readAllStandardOutput()), QString(process.readAllStandardError()));
    return static_cast<EGhostCode>(process.exitCode());
}

QString CommonFunc::getFileSystem(const QString &fileSysytem)
{
    if (gSwapFileSystems.contains(fileSysytem, Qt::CaseInsensitive)) {
        return gLinuxSwapFileSystem;
    } else {
        return fileSysytem;
    }
}

void CommonFunc::createFile(const QString &fileSysytem)
{
    QFile info(fileSysytem);
    if (!info.open(QFile::ReadWrite|QFile::Text)) {
        qWarning() << "create file " + fileSysytem + " failed";
    }
    info.close();
}

bool systemdDetectVirt() {
    QString output;
    bool ret = CommonFunc::command("/usr/bin/systemd-detect-virt", {}, output);
    return ret ? !output.contains("none") : ret;
}

bool CommonFunc::isVirtualMachine()
{
    // Check "hypervisor" flag is added in /proc/cpuinfo.
    return readFile("/proc/cpuinfo").contains("hypervisor") || systemdDetectVirt();
}


qint64 CommonFunc::availSizeFromMountPoint(const QString &mountPoint, QString &err)
{
    QString cmd = QString("lsblk -lbpn -o MOUNTPOINT,FSAVAIL | grep \"%1 \" | awk '{print $NF}'").arg(mountPoint);
    if (!command("/bin/bash", {"-c", cmd}, err)) {
        err = QString("Command: %1 failed. %2").arg(cmd).arg(err);
        return 0;
    }

    QString availSizeStr = err;
    availSizeStr.replace(" ", "");
    availSizeStr.replace("\n", ";");
    availSizeStr += ";";
    availSizeStr = availSizeStr.left(availSizeStr.indexOf(";"));
    if (availSizeStr.isEmpty()) {
        err = "availSizeFromMountPoint get " + mountPoint + " availSize is 0";
        return 0;
    }
    return availSizeStr.toLongLong();
}

QStringList CommonFunc::qstringSplitToQStringList(const QString &srcString, const QString &splitString)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    return srcString.split(splitString, Qt::SkipEmptyParts);
#else
    return srcString.split(splitString, QString::SkipEmptyParts);
#endif
};

bool CommonFunc::isInDesktop()
{
    // 判断lightdm的配置中是否有安装器的启动设置，如果有表示当前不在live桌面环境
    QString lightdmConfPath = "/etc/lightdm/lightdm.conf";
    QFile file(lightdmConfPath);
    if (file.open(QIODevice::Text | QIODevice::ReadOnly)) {
        QByteArray fileContent = file.readAll();
        bool hasInstaller = fileContent.contains("fedora-dde-installer");
        file.close();
        return hasInstaller;
    } else {
        qWarning() << QString("could't open json file : %1 err : %2").arg(lightdmConfPath).arg(file.errorString());
        return false;
    }

}


void CommonFunc::setRecoveryKey()
{
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    uuid = uuid.replace("-", "");

    if (uuid.length() > 24) {
        uuid = uuid.left(24);
    }
    
    QString keyFilePath = "/tmp/" + CommonFunc::getSettingsString("DI_DISK_KEY_FILE_CONFIG").arg(QString("_%1").arg(QDateTime::currentSecsSinceEpoch()));
    QFile file(keyFilePath);
    if (file.open(QIODevice::Text | QIODevice::WriteOnly)) {
        file.write(uuid.toLocal8Bit());
        file.close();
    } else {
        keyFilePath.clear();
    }


    CommonFunc::setSettingsValue("DI_CRYPT_RECOVERY_KEY", keyFilePath);
    
}

QString CommonFunc::wrapWords(const QString &srcStr, int distWidth, const QFontMetrics &fm)
{
    QString wrappedText;
    QString currentLine;

    if (srcStr.contains(' ')) {
        // 包含空格的情况
        for (const QString &word : srcStr.split(' ')) {
            QString testLine = currentLine.isEmpty() ? word : currentLine + " " + word;
            if (fm.horizontalAdvance(testLine) > distWidth) {
                if (!currentLine.isEmpty()) {
                    wrappedText += currentLine + "\n";
                }
                currentLine = word;
            } else {
                currentLine = testLine;
            }
        }
    } else {
        // 不包含空格的情况
        for (int i = 0; i < srcStr.length(); ++i) {
            QString testLine = currentLine + srcStr[i];
            if (fm.horizontalAdvance(testLine) > distWidth) {
                if (!currentLine.isEmpty()) {
                    wrappedText += currentLine + "\n";
                }
                currentLine = srcStr[i];
            } else {
                currentLine = testLine;
            }
        }
    }

    if (!currentLine.isEmpty()) {
        wrappedText += currentLine;
    }

    return wrappedText;
}

QStringList CommonFunc::findOldUsers(const QString &homePath)
{
    QDir dir(homePath);
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    QStringList users = dir.entryList();

    for (auto it = users.begin(); it != users.end();) {
    if (*it == "lost+found" || it->contains("@") || !QFile::exists(homePath + "/" + *it + "/Desktop")) {
        qInfo() << "ignore user:" << *it;
        it = users.erase(it);
    } else {
        ++it;
    }
    }
    qInfo() << "find users:" << users;
    return users;
}
