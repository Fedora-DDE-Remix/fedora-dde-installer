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
#include "ScreenMonitor.h"

#include <DApplication>
#include <DLog>
#include <QScreen>
#include <DTitlebar>


DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

int main(int argc, char* argv[]) {

    //for qt5platform-plugins load DPlatformIntegration or DPlatformIntegrationParent
    if (!QString(qgetenv("XDG_CURRENT_DESKTOP")).toLower().startsWith("deepin")){
        setenv("XDG_CURRENT_DESKTOP", "Deepin", 1);
    }

    // init DApplication
    DApplication::setAttribute(Qt::AA_Use96Dpi);

    DApplication app(argc, argv);
    app.setApplicationDisplayName(::QObject::tr("Deepin Installer"));
    app.setApplicationName("deepin-installer");

    // init log
#ifndef QT_DEBUG
    InstallerLogManager::instance()->redirectLog(QString("%1/%2.log").arg(gSystemLogDir, app.applicationName()));
#else
    QString output;
    CommonFunc::command(QCoreApplication::applicationDirPath() +  "/deepin-installer-config",
                        QStringList() << "init" << "/tmp/deepin-installer/deepin-installer.conf", output);
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

    qInfo() << "deepin-installer version:" + CommonFunc::getVersion();

    //install translate
    QTranslator translator;
    QString strLocale = CommonFunc::getSettingsString("DI_LOCALE");
    QString tsFile = QString("%1/deepin-installer-%2.qm").arg(DI_I18N_DIR_ENV).arg(strLocale);
    translator.load(tsFile);

    // scan devices
    DeviceManager::instance()->scanDevices(true);
    ScreenMonitor screenMonitor;
    screenMonitor.start();
    // init mainwindow
    MainWindow w;
    w.setTranslator(&translator);
    app.installTranslator(&translator);
    app.installEventFilter(&w);

    QString installMode = CommonFunc::getSettingsString("DI_INSTALL_MODE");
    if (installMode.contains("auto")) {
        w.loadPlugins("deepin_installer_plugins");
        QTimer::singleShot(100, &w, &MainWindow::showFullScreen);
    } else {
        //非自动安装，插上显示器才能进入安装界面
        QObject::connect(&screenMonitor, &ScreenMonitor::screenReady, &w, [&w](){
            if (w.isHidden()) {
                w.setPluginSize();
                w.loadPlugins("deepin_installer_plugins");
                w.showFullScreen();
            }
        });
    }

    QObject::connect(&app, &QCoreApplication::aboutToQuit, DeviceManager::instance(), &DeviceManager::quitUSBScan);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &screenMonitor, &ScreenMonitor::terminate);


    return app.exec();
}
