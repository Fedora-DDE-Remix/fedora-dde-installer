/*

 * Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.

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
#include "BaseTableView.h"

#include <DApplication>
#include <DGuiApplicationHelper>
#include <DPalette>
#include <DStyle>

#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QHeaderView>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QScroller>
#include <QScrollBar>

BaseTableView::BaseTableView(DWidget *parent)
    : DTreeView(parent)
{
    setRootIsDecorated(false);
    setItemsExpandable(false);
    setFrameStyle(QFrame::NoFrame);
    viewport()->setAutoFillBackground(false);
    // if set to true, qt will draw item background for you, thats not what you want here!
    setAlternatingRowColors(false);
    setAllColumnsShowFocus(false);
    setFocusPolicy(Qt::TabFocus);

    // add treeview touch scroll support
    QScroller::grabGesture(viewport(), QScroller::TouchGesture);
    verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
}

void BaseTableView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    DTreeView::currentChanged(current, previous);

    if (previous.isValid()) {
        QRect previousRect = visualRect(previous);
        previousRect.setX(0);
        previousRect.setWidth(viewport()->width());
        viewport()->update(previousRect);
    }
    if (current.isValid()) {
        QRect currentRect = visualRect(current);
        currentRect.setX(0);
        currentRect.setWidth(viewport()->width());
        viewport()->update(currentRect);
    }
}
