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

#ifndef BASETABLEVIEW_H
#define BASETABLEVIEW_H

#include <DTreeView>

DWIDGET_USE_NAMESPACE

class BaseItemDelegate;
class QModelIndex;
class BaseTableView : public DTreeView
{
    Q_OBJECT

public:
    BaseTableView(DWidget *parent = Q_NULLPTR);

protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

private:
    BaseItemDelegate *m_itemDelegate = Q_NULLPTR;

};

#endif  // BASETABLEVIEW_H
