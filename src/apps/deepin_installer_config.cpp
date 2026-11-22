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
#include <sys/stat.h>
#include <cstdlib>


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
    return QStringLiteral("Fedora");
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


bool initConfig(const QString &filePath)
{
    const QString defaults = getSystemEnv(DI_CONFIG_DIR_ENV) + "/settings/default_settings.ini";
    if (!QFileInfo::exists(defaults))
        return false;
    QSettings source(defaults, QSettings::IniFormat);
    QSettings destination(filePath, QSettings::IniFormat);
    destination.clear();
    for (const QString &key : source.allKeys())
        destination.setValue(key, source.value(key));
    destination.sync();
    return source.status() == QSettings::NoError && destination.status() == QSettings::NoError
            && QFile::setPermissions(filePath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

QString getSettingsString(const QString &key, const QString &section, const QString &filePath, bool &ok)
{
    ok = false;
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

    settings.sync();
    ok = settings.status() == QSettings::NoError && QFileInfo(path).isReadable();
    return value;
}

bool setSettingsString(const QString &key, const QString &value, const QString &section, const QString &filePath)
{
    QString path(filePath);

    if (!path.isEmpty() && !QFileInfo::exists(path)) {
        qWarning() << "file not exist." << path;
        return false;
    }

    if (path.isEmpty()) {
        path = getSystemEnv(DI_CONFIG_FILE_ENV);
    }

    if (!QFileInfo::exists(path)) {
        qWarning() << "file not exist." << path;
        return false;
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
    return settings.status() == QSettings::NoError;
}



int main(int argc, char* argv[]) {
    umask(0077);
    // init DApplication
    QCoreApplication app(argc, argv);
    app.setApplicationName("fedora-dde-installer");

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
        // fedora-dde-installer-config init ini-file
        if (args.size() != 2) {
            // Show help and exit
            parser.showHelp(-1);
        }

        return initConfig(args.at(1)) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (command == gInstallConfigGet) {
        // fedora-dde-installer-config get ini-file [section] key
        if (args.size() < 3 || args.size() > 4) {
            parser.showHelp(-1);
        }

        QString section;
        QString key(args.at(2));

        if (args.size() == 4) {
            section = args.at(2);
            key = args.at(3);
        }

        bool ok = false;
        const QString value = getSettingsString(key, section, args.at(1), ok);
        if (!ok)
            return EXIT_FAILURE;
        std::cout << value.toStdString();
    } else if (command == gInstallConfigSet) {
        // fedora-dde-installer-config set ini-file [section] key value
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

        return setSettingsString(key, value, section, args.at(1)) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    return 0;
}
