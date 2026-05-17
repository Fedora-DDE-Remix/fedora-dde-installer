/*

 * Copyright (C) 2019 ~ 2022 Deepin Technology Co., Ltd.

 *

 * Author:     dongpengpeng <dongpengpeng@uniontech.com>

 *

 * Maintainer: dongpengpeng <dongpengpeng@uniontech.com>

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

#ifndef DISKINFOWIDGET_H
#define DISKINFOWIDGET_H

#include "structs.h"

#include <DLabel>

DWIDGET_USE_NAMESPACE

class DeviceSizeBar;
class DeviceInfoBar;
class DiskinfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DiskinfoWidget(QWidget *parent = Q_NULLPTR);
    void setSystemSize(qint64 size);
    void setDataSize(qint64 size);
    void setDeviceSize(qint64 size);
    SDeviceSizeInfo toSDeviceSizeInfo(const QString &color, qint64 size, const QString &text = Q_NULLPTR, bool edit = false);

    void setEditEnable(bool isEnable);
    void updateTs();

signals:
    void updateDeviceBar(const SDeviceSizeInfo &deviceInfo);
    void editBtnClicked();

private:
    DLabel *m_sizeLabel = Q_NULLPTR;
    DeviceInfoBar *m_deviceInfoLabel = Q_NULLPTR;
    DeviceSizeBar *m_deviceSizeLabel = Q_NULLPTR;

    QList<SDeviceInfo> m_deviceInfoList = {};

    qint64 m_showSystemSize = 0;
    qint64 m_showDataSize = 0;
    qint64 m_showTotalSize = 0;

};

#endif // DISKINFOWIDGET_H
