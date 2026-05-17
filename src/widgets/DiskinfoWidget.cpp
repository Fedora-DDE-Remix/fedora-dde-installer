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
#include "DiskinfoWidget.h"
#include "globals.h"
#include "DeviceSizeBar.h"
#include "DeviceInfoBar.h"
#include "CommonFunc.h"

#include <QResizeEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>

DiskinfoWidget::DiskinfoWidget(QWidget *parent):
    QWidget(parent)
{
    m_sizeLabel = new DLabel(this);
    QHBoxLayout *sizeLayout = new QHBoxLayout;
    sizeLayout->setSpacing(0);
    sizeLayout->addStretch();
    sizeLayout->addWidget(m_sizeLabel);
    sizeLayout->addStretch();

    m_deviceSizeLabel = new DeviceSizeBar(this);
    m_deviceSizeLabel->setFixedSize(240, 10);
    QHBoxLayout *deviceSizeLayout = new QHBoxLayout;
    deviceSizeLayout->setSpacing(0);
    deviceSizeLayout->setContentsMargins(0, 0, 0, 0);
    deviceSizeLayout->addStretch();
    deviceSizeLayout->addWidget(m_deviceSizeLabel);
    deviceSizeLayout->addStretch();

    QHBoxLayout *deviceInfoLayout = new QHBoxLayout;
    deviceInfoLayout->setSpacing(0);

    m_deviceInfoLabel = new DeviceInfoBar(this);
    connect(m_deviceInfoLabel, &DeviceInfoBar::clicked, this, &DiskinfoWidget::editBtnClicked);
    m_deviceInfoLabel->setFixedSize(220, 46);

    deviceInfoLayout->addWidget(m_deviceInfoLabel);

    QVBoxLayout* mainlayout = new QVBoxLayout;
    mainlayout->setSpacing(0);
    mainlayout->setContentsMargins(0, 0, 0, 0);
    mainlayout->addLayout(sizeLayout);
    mainlayout->addSpacing(3);
    mainlayout->addLayout(deviceSizeLayout);
    mainlayout->addSpacing(CommonFunc::isBigScreen() ? 10 : 5);
    mainlayout->addLayout(deviceInfoLayout);
    setLayout(mainlayout);
}

void DiskinfoWidget::setSystemSize(qint64 size)
{
    m_showSystemSize = size;

    double showSize = size;
    QString unit = CommonFunc::showSizeUnit(showSize);
    m_deviceSizeLabel->addDeviceSize(toSDeviceSizeInfo("#3A75F6", showSize));
    m_deviceInfoLabel->addDeviceInfo(toSDeviceSizeInfo("#3A75F6", showSize, QString("%1%2").arg(QString::number(showSize), unit), true));
}

void DiskinfoWidget::setDeviceSize(qint64 size)
{
    m_showTotalSize = size;
    double dsize = size;
    QString unit = CommonFunc::showSizeUnit(dsize);
    m_sizeLabel->setText(QString("%1%2").arg(QString::number(dsize), unit));
}

void DiskinfoWidget::setDataSize(qint64 size)
{
    m_showDataSize = size;
    double showSystemSize = m_showSystemSize;
    QString unit = CommonFunc::showSizeUnit(showSystemSize);
    double showTotalSize = m_showTotalSize;
    QString totalUnit = CommonFunc::showSizeUnit(showTotalSize);
    double showDateSize = showTotalSize - showSystemSize;

    m_deviceSizeLabel->addDeviceSize(toSDeviceSizeInfo("#FFCF25", showDateSize));
    m_deviceInfoLabel->addDeviceInfo(toSDeviceSizeInfo("#FFCF25", showDateSize, QString("%1%2").arg(QString::number(showDateSize), unit)));
}

SDeviceSizeInfo DiskinfoWidget::toSDeviceSizeInfo(const QString &color, qint64 size, const QString &text, bool edit)
{
    SDeviceSizeInfo deviceSizeInfo;
    if (!color.isEmpty()) {
        deviceSizeInfo.color = color;
    }

    if (!text.isEmpty()) {
        deviceSizeInfo.text = text;
    }

    deviceSizeInfo.size = size;
    deviceSizeInfo.edit = edit;

    return deviceSizeInfo;
}

void DiskinfoWidget::setEditEnable(bool isEnable)
{
    m_deviceInfoLabel->setEnable(isEnable);
}

void DiskinfoWidget::updateTs()
{
    this->setSystemSize(m_showSystemSize);
    this->setDataSize(m_showDataSize);
}









