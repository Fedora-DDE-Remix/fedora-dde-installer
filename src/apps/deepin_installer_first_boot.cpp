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
    app.setApplicationDisplayName("Deepin Installer Reborn");
    app.setApplicationName("deepin-installer-first-boot");

    QFont font(app.font());
    // 设置的字体大小是跟随像素的
    font.setPixelSize(CommonFunc::getSettingsInt("DI_SYSTEM_FONT_SIZE_CONFIG"));
    font.setFamily(CommonFunc::getUIDefaultFont());
    app.setFont(font);

    // init log
#ifndef QT_DEBUG
    InstallerLogManager::instance()->redirectLog(QString("%1/%2.log").arg(gSystemLogDir, app.applicationName()));
#else
    QString output;
    CommonFunc::command(QCoreApplication::applicationDirPath() +  "/deepin-installer-config",
                    QStringList() << "init" << "/tmp/deepin-installer/deepin-installer.conf", output);
#endif // QT_DEBUG

    DLogManager::registerConsoleAppender();

    //install translate
    QTranslator translator;
    QString strLocale = CommonFunc::getSettingsString("DI_LOCALE");
    QString tsFile = QString("%1/deepin-installer-%2.qm").arg(DI_I18N_DIR_ENV).arg(strLocale);
    translator.load(tsFile);
    qInfo() << QString("locale: <%1> current load ts file: <%2>").arg(strLocale, tsFile);

    ScreenMonitor screenMonitor;
    screenMonitor.start();
    // init mainwindow
    MainWindow w;
    w.setTranslator(&translator);

    QObject::connect(&screenMonitor, &ScreenMonitor::screenReady, &w, [&w](){
        if (w.isHidden()) {
            w.setPluginSize();
            w.loadPlugins("deepin_installer_first_boot_plugins");
            w.showFullScreen();
        }
    });

    app.installEventFilter(&w);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, &screenMonitor, &ScreenMonitor::terminate);

    qApp->installTranslator(&translator);

    return app.exec();
}
