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
#include "InstallerLogManager.h"
#include "ScreenMonitor.h"

#include <DApplication>
#include <DLog>
#include <QScreen>

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
    app.setApplicationDisplayName(::QObject::tr("Fedora DDE Remix Installer"));
    app.setApplicationName("fedora-dde-installer-live-config");

    QFont font(app.font());
    // 设置的字体大小是跟随像素的
    font.setPixelSize(CommonFunc::getSettingsInt("DI_SYSTEM_FONT_SIZE_CONFIG"));
    font.setFamily(CommonFunc::getUIDefaultFont());
    app.setFont(font);

    InstallerLogManager::instance()->redirectLog(QString("%1/%2.log").arg(gSystemLogDir, app.applicationName()));

    DLogManager::registerConsoleAppender();

    qInfo() << "fedora-dde-installer version:" + CommonFunc::getVersion();

    //install translate
    QTranslator translator;
    QString strLocale = CommonFunc::getSettingsString("DI_LOCALE");
    QString tsFile = QString("%1/fedora-dde-installer-%2.qm").arg(DI_I18N_DIR_ENV).arg(strLocale);
    translator.load(tsFile);

    ScreenMonitor screenMonitor;
    screenMonitor.start();

    // init mainwindow
    MainWindow w;
    w.setTranslator(&translator);
    app.installTranslator(&translator);
    app.installEventFilter(&w);

    QObject::connect(&screenMonitor, &ScreenMonitor::screenReady, &w, [&w](){
        if (w.isHidden()) {
            w.setPluginSize();
            w.loadPlugins("fedora_dde_installer_live_config_plugins");
            w.setWindowFlag(Qt::WindowCloseButtonHint, false);
            w.showFullScreen();
        }
    });
    app.installEventFilter(&w);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &screenMonitor, &ScreenMonitor::terminate);

    return app.exec();
}
