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

#include "CMainWindow.h"
#include "CommonFunc.h"
#include "globals.h"
#include "DeviceManager.h"
#include "InstallerLogManager.h"

#include <DApplication>
#include <DLog>
#include <QScreen>
#include <QTimer>
#include <DTitlebar>
#include <DDialog>
#include <cstdio>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>


DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

int main(int argc, char* argv[]) {
    umask(0077);
    // Password widgets can hold transient plaintext before it reaches the
    // sealed descriptor. Disable dumps before constructing any GUI object.
    // PR_SET_DUMPABLE also covers pipe-based systemd-coredump handlers, which
    // can ignore RLIMIT_CORE. This does not promise erasure of all RAM copies.
    const struct rlimit coreLimit = {0, 0};
    if (setrlimit(RLIMIT_CORE, &coreLimit) != 0
            || prctl(PR_SET_DUMPABLE, 0, 0, 0, 0) != 0) {
        std::fprintf(stderr, "Fedora DDE Installer: cannot disable password-bearing process dumps.\n");
        return 1;
    }

    //for qt5platform-plugins load DPlatformIntegration or DPlatformIntegrationParent
    if (!QString(qgetenv("XDG_CURRENT_DESKTOP")).toLower().startsWith("deepin")){
        setenv("XDG_CURRENT_DESKTOP", "Deepin", 1);
    }

    // init DApplication
    DApplication::setAttribute(Qt::AA_Use96Dpi);

    DApplication app(argc, argv);
    app.setApplicationName("fedora-dde-installer");

    // init log
#ifndef QT_DEBUG
    InstallerLogManager::instance()->redirectLog(QString("%1/%2.log").arg(gSystemLogDir, app.applicationName()));
#else
    QString output;
    CommonFunc::command(QCoreApplication::applicationDirPath() +  "/fedora-dde-installer-config",
                        QStringList() << "init" << "/run/fedora-dde-installer/installer.conf", output);
#endif // QT_DEBUG

    QFont font(app.font());
    // 设置的字体大小是跟随像素的
    font.setPixelSize(CommonFunc::getSettingsInt("DI_SYSTEM_FONT_SIZE_CONFIG"));
    font.setFamily(CommonFunc::getUIDefaultFont());
    app.setFont(font);

    DLogManager::registerConsoleAppender();

    qputenv("DTK_USE_SEMAPHORE_SINGLEINSTANCE", "1");
    if (!app.setSingleInstance(app.applicationName(), DApplication::UserScope)) {
        exit(0);
    }

    qInfo() << "fedora-dde-installer version:" + CommonFunc::getVersion();

    //install translate
    QTranslator translator;
    QString strLocale = CommonFunc::getSettingsString("DI_LOCALE");
    QString tsFile = QString("%1/fedora-dde-installer-%2.qm").arg(DI_I18N_DIR_ENV).arg(strLocale);
    translator.load(tsFile);

    // scan devices
    DeviceManager::instance()->scanDevices(true);
    // init mainwindow
    MainWindow w;
    w.setTranslator(&translator);
    app.installTranslator(&translator);
    app.setApplicationDisplayName(::QObject::tr("Fedora DDE Remix Installer"));
    app.installEventFilter(&w);

    // One interactive workflow: install, configure the mounted target, finish.
    CommonFunc::setSettingsValue("DI_INSTALL_MODE", gInstallModeDefault);
    CommonFunc::setSettingsValue("DI_REBOOT_AFTER_SETUP", false);
    CommonFunc::setSettingsValue("DI_PASSWORD_ENCRYPTION_CONFIG", false);
    CommonFunc::setSettingsValue("DI_ROOT_USER_LOAD", false);
    CommonFunc::setSettingsValue("DI_IS_GHOST_MODE", false);
    QTimer::singleShot(0, &w, [&w] {
        w.setPluginSize();
        if (!w.loadPlugins("fedora_dde_installer_plugins")) {
            DDialog dialog;
            dialog.setTitle(QObject::tr("The installer could not start"));
            dialog.setMessage(QObject::tr("A required page or configuration is missing. Rebuild the Live image with the complete Fedora DDE Remix Installer package."));
            dialog.setWordWrapMessage(true);
            dialog.addButton(QObject::tr("Close"));
            dialog.exec();
            QCoreApplication::exit(EXIT_FAILURE);
            return;
        }
        w.showFullScreen();
    });

    QObject::connect(&app, &QCoreApplication::aboutToQuit, DeviceManager::instance(), &DeviceManager::quitUSBScan);


    return app.exec();
}
