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

#include "CommonFunc.h"
#include "PluginManager.h"
#include "PluginInterface.h"

#include <QCoreApplication>
#include <DLog>
#include <QDebug>

DCORE_USE_NAMESPACE

int main(int argc, char* argv[]) {
    // init DApplication
    QCoreApplication app(argc, argv);
    app.setApplicationName("deepin-installer-parted");

    // init app
    DLogManager::registerConsoleAppender();
    PluginInterface *plu = PluginsManager::instance()->getPlugin(QString("%1/libDeepinInstallerPartition.so").arg(CommonFunc::getSystemEnv(DI_INSTALL_MODULE_DIR_ENV)));
    if(plu && plu->load()) {
        TaskPluginInterface *partTask = dynamic_cast<TaskPluginInterface*>(plu);
        if (nullptr == partTask) {
            qCritical() << "deepin-installer-parted is nullptr.";
            return -1;
        }

        if (!partTask->start()) {
            qCritical() << "deepin-installer-parted program execution failure.";
            return -1;
        }
    }

    return 0;
}
