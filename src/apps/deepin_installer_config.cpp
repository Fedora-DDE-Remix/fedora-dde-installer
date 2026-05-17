/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "globals.h"

#include <QCoreApplication>
#include <QDebug>
#include <iostream>
#include <QCommandLineParser>
#include <QSettings>
#include <QFile>
#include <QFileInfo>


QString getSystemEnv(const QString &key)
{

#ifdef QT_DEBUG
    QString reset = key;
#else
    QString reset = qgetenv(key.toStdString().c_str());
#endif // QT_DEBUG

    return reset;
}

bool isPxeInstall()
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

QString getCurrentOsType()
{
    QSettings settings("/etc/deepin-version", QSettings::IniFormat);
    settings.beginGroup("Release");

    QString type = settings.value("Type", "Desktop").toString();
    settings.endGroup();
    return type;
}

QString getCurrentPlatform()
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


void initConfig(const QString &filePath)
{
    if (QFile::exists(filePath)) {
        QFile::remove(filePath);
    }

    // Read default settings
    QString defaultSettings = QString("%1/%2/%3").arg(getSystemEnv(DI_CONFIG_DIR_ENV), "settings", "default_settings.ini");
    QString overrideSettings = QString("%1/os/%2/%3.ini").arg(getSystemEnv(DI_CONFIG_DIR_ENV), getCurrentOsType(), getCurrentPlatform());
    QString oemSettings = QString("%1/oem/settings.ini").arg(getSystemEnv(DI_LIVE_DIR_ENV));
    QString extraSettings = QString("%1/pxemode/settings.ini").arg(getSystemEnv(DI_LIVE_DIR_ENV));
    QString pxeSettings = QString("%1/pxemode/settings.ini-pxe").arg(getSystemEnv(DI_LIVE_DIR_ENV));
    QString debugSettings = QString("/tmp/oem/settings.ini");

    qInfo() << "defaultSettings = " << defaultSettings;
    qInfo() << "overrideSettings = " << overrideSettings;
    qInfo() << "oemSettings = " << oemSettings;
    qInfo() << "extraSettings = " << extraSettings;
    qInfo() << "pxeSettings = " << pxeSettings;
    QStringList settingsList{defaultSettings, overrideSettings, oemSettings, extraSettings, debugSettings};
    if (isPxeInstall()) {
        settingsList << pxeSettings;
    }
    for (const QString& file : settingsList) {
        if (!QFile::exists(file)) {
            qWarning() << "file not exist. " << file;
            continue;
        }

        qInfo() << "init config: " << file;
        QSettings settings(file, QSettings::IniFormat);
        for (const QString& key : settings.allKeys()) {
            if ( "DI_DEEPIN_SQUASHFS_VERIFY" == key  && file != defaultSettings) {
                continue;
            }
            QSettings(filePath, QSettings::IniFormat).setValue(key, settings.value(key));
        }
    }
}

QString getSettingsString(const QString &key, const QString &section, const QString &filePath)
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
    QString value;

    if (section.isEmpty()) {
        value = settings.value(key).toString();
    } else {
        settings.beginGroup(section);
        value = settings.value(key).toString();
        settings.endGroup();
    }

    return value;
}

void setSettingsString(const QString &key, const QString &value, const QString &section, const QString &filePath)
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
}



int main(int argc, char* argv[]) {
    // init DApplication
    QCoreApplication app(argc, argv);
    app.setApplicationName("deepin-installer");

    // init app
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("command", "init config or set or get value", "init/get/set");
    parser.addPositionalArgument("ini-file", "Absolute path to ini file");
    parser.addPositionalArgument("section", "Section name", "[section]");
    parser.addPositionalArgument("key", "Key name");
    parser.addPositionalArgument("value", "Value to be set");
    parser.process(app);

    QStringList cmdList{gInstallConfigInit, gInstallConfigGet, gInstallConfigSet};
    // 注意这里Qt返回的参数列表不包含命令本身，这是有别于main函数的argv参数的
    QStringList args = parser.positionalArguments();

    if (args.size() < 1) {
        parser.showHelp(-1);
    }

    QString command = args.at(0);

    if (!cmdList.contains(command)) {
        parser.showHelp(-1);
    }

    // QSettings内置了一个General组用于写特殊信息，如果想自己创建General组，那因为与内置的冲突了，QSettings把自定义的General处理成%General。
    // 读写时不指定group，操作的就是真正的General段。
    if (command == gInstallConfigInit) {
        // deepin-installer-config init ini-file
        if (args.size() != 2) {
            // Show help and exit
            parser.showHelp(-1);
        }

        initConfig(args.at(1));
    } else if (command == gInstallConfigGet) {
        // deepin-installer-config get ini-file [section] key
        if (args.size() < 3 || args.size() > 4) {
            parser.showHelp(-1);
        }

        QString section;
        QString key(args.at(2));

        if (args.size() == 4) {
            section = args.at(2);
            key = args.at(3);
        }

        std::cout << getSettingsString(key, section, args.at(1)).toStdString();
    } else if (command == gInstallConfigSet) {
        // deepin-installer-config set ini-file [section] key value
        if (args.size() < 4 || args.size() > 5) {
            parser.showHelp(-1);
        }

        QString section;
        QString key(args.at(2));
        QString value(args.at(3));

        if (args.size() == 5) {
            section = args.at(2);
            key = args.at(3);
            value = args.at(4);
        }

        setSettingsString(key, value, section, args.at(1));
    }

    return 0;
}
