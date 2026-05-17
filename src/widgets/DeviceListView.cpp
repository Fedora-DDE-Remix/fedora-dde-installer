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
#include "DeviceListView.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QVBoxLayout>

#include <DGuiApplicationHelper>
#include <DApplication>

DeviceListView::DeviceListView(QWidget *parent)
    : DListView(parent)
    , m_currentIndex(-1)
{
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::NoSelection);
    setFrameShape(QFrame::NoFrame);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void DeviceListView::resetStatus(const QModelIndex &index)
{
    m_currentIndex = index.row();
    DListView::clearSelection();
    DListView::setSelectionMode(DListView::NoSelection);
    DListView::setCurrentIndex(index);
}

void DeviceListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    DListView::setSelectionMode(DListView::NoSelection);
    DListView::currentChanged(current, previous);
}

QModelIndex DeviceListView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    DListView::setSelectionMode(DListView::NoSelection);
    return DListView::moveCursor(cursorAction, modifiers);
}

void DeviceListView::keyPressEvent(QKeyEvent *event)
{

    if (event->key() == Qt::Key_Up) {
        QModelIndex nextIndex = model()->index(m_currentIndex - 1, 0);
        if (nextIndex.isValid() ) {
            if (isRowHidden(m_currentIndex - 1)) {
                if ( m_currentIndex -1 <= 0) {
                    return;
                }
                m_currentIndex--;
                keyPressEvent(event);
                return;
            }
            setCurrentIndex(nextIndex);
            Q_EMIT clicked(nextIndex);
        }
        return;
    } else if (event->key() == Qt::Key_Down) {
        QModelIndex nextIndex = model()->index(m_currentIndex + 1, 0);
        if (nextIndex.isValid()) {
            if (isRowHidden(m_currentIndex + 1)) {
                if (model()->rowCount()-1 <= m_currentIndex + 1) {
                    return;
                }
                m_currentIndex++;
                keyPressEvent(event);
                return;
            }
            setCurrentIndex(nextIndex);
            Q_EMIT clicked(nextIndex);
        }
        return;
    }
    return DListView::keyPressEvent(event);
}



