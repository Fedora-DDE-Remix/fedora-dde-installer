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

#include "AvatarButton.h"

#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include "CommonFunc.h"

namespace {
const int kButtonSize = 128;
const int kIconSize = 128;
const int kIconRoundSize = 90;
}  // namespace

AvatarButton::AvatarButton(QWidget* parent, bool isListShow)
    : QPushButton (parent)
    , m_isChecked(false)
    , m_isListShow(false)
{
    setObjectName("AvatarButton");

    setCheckable(true);
    m_isListShow = isListShow;
    int nButtonSize = m_isListShow ? kButtonSize - 28 : (CommonFunc::isBigScreen() ? kButtonSize : kButtonSize - 28);
    setFixedSize(nButtonSize, nButtonSize);
}

void AvatarButton::updateIcon(const QString& icon) {
    m_icon = icon;
    update();
}

void AvatarButton::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 光圈
    const QRect ellipseRound((width() - kIconRoundSize) / 2, (height() - kIconRoundSize) /2,
                              kIconRoundSize, kIconRoundSize);
    if (isChecked()) {
        QPen pen(QColor("#209cee"));
        pen.setWidth(3);
        painter.setPen(pen);
        painter.drawEllipse(ellipseRound);
    }

    int nIconSize = m_isListShow ? kIconSize - 48 : (CommonFunc::isBigScreen() ? kIconSize : kIconSize - 28);
    // 图标
    const QRect ellipseRect((width() - nIconSize) / 2, (height() - nIconSize) /2,
                             nIconSize, nIconSize);

    QPainterPath path, path2;
    path.addEllipse(ellipseRect);
    painter.setClipPath(path);
    painter.drawPixmap(ellipseRect, QPixmap(m_icon));

    if(!m_isListShow) {
        QRect labelRect = rect();
        labelRect.adjust(0, rect().height() - 28, 0, 0);
        path2.addRect(labelRect);
        painter.fillPath(path2, QColor(0, 0, 0, 80));

        painter.setPen(QColor(Qt::white));
        QTextOption textOption;
        textOption.setAlignment(Qt::AlignCenter);
        painter.drawText(labelRect, ::QObject::tr("Edit"), textOption);
    }

    if (hasFocus()) {
        QPen pen(QColor("#209cee"));
        pen.setWidth(5);
        painter.setPen(pen);
        painter.drawEllipse(QRect(0, 0, width(), height()));
    }

    painter.end();

}
