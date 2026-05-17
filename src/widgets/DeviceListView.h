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
#ifndef DEVICELISTVIEW_H
#define DEVICELISTVIEW_H

#include <QJsonObject>
#include <QItemDelegate>

#include <DListView>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class DeviceListView : public DListView
{
    Q_OBJECT
public:
    explicit DeviceListView(QWidget *parent = Q_NULLPTR);
    void resetStatus(const QModelIndex &index);

protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) Q_DECL_OVERRIDE;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;

private:
    int m_currentIndex;

};

#endif // DEVICELISTVIEW_H
