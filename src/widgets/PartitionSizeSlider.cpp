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

#include "PartitionSizeSlider.h"
#include "structs.h"
#include "CommonFunc.h"
#include "SystemInfoTip.h"

#include <QHBoxLayout>
#include <QRegularExpressionValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QDebug>

PartitionSizeSlider::PartitionSizeSlider(QWidget* parent)
    : QFrame(parent)
{
    this->initUI();
    this->initConnection();
}

qint64 PartitionSizeSlider::value()
{
    return mSlider->value();
}

qint64 PartitionSizeSlider::maximum()
{
    return mSlider->maximum();
}

qint64 PartitionSizeSlider::minimum()
{
    return mSlider->minimum();
}

void PartitionSizeSlider::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath PaintPath;
    for (int i = 0; i < 10; i++)
    {
        const QColor fullAreaColor(0, 0, 0, 5);

        PaintPath.addRoundedRect(rect(), 10, 10);
        painter.fillPath(PaintPath, fullAreaColor);
    }
}

void PartitionSizeSlider::setMaximum(qint64 maximum_size)
{
    mSlider->setMaximum(int(maximum_size));
}

void PartitionSizeSlider::setMinimum(qint64 minimumSize)
{
    mSlider->setMinimum(int(minimumSize));
}

void PartitionSizeSlider::setValue(qint64 size)
{
    mSlider->setValue(int(size));
}

void PartitionSizeSlider::initConnection()
{
    connect(mSlider, &DSlider::valueChanged, [this](int value){
        mEditor->setText(QString::number(value));
        Q_EMIT this->valueChanged(value);
    });
    connect(mEditor, &DLineEdit::textChanged, [this](const QString &value){
        mSlider->setValue(value.toInt());
    });
}

void PartitionSizeSlider::initUI()
{
    mSlider = new DSlider(Qt::Horizontal, this);
    mSlider->slider()->setFocusPolicy(Qt::NoFocus);

    mEditor = new DLineEdit(this);
    mEditor->lineEdit()->setValidator(new QRegularExpressionValidator(QRegularExpression("^[1-9]+[0-9]{1,}$")));
    // Disable context menu.
    mEditor->setContextMenuPolicy(Qt::NoContextMenu);
    mEditor->setText(QString::number(mSlider->value()));

    m_sizelabel = new QLabel(this);
    m_sizelabel->setText(CommonFunc::getSettingsString("DI_SIZE_MUNIT_CONFIG"));

    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins(0, 4, 0, 4);
    layout->setSpacing(0);
    layout->addWidget(mSlider, 5);
    layout->addSpacing(5);
    layout->addWidget(mEditor, 4);
    layout->addSpacing(5);
    layout->addWidget(m_sizelabel);
    this->setLayout(layout);
    this->setContentsMargins(10, 0, 10, 0);
}

