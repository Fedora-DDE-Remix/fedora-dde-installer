#ifndef GhostListView_H
#define GhostListView_H

#include "structs.h"
#include "BaseTableView.h"
#include <QStyledItemDelegate>

DWIDGET_USE_NAMESPACE

class DiskDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit DiskDelegate(QObject *parent = Q_NULLPTR);
protected:
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // GhostListView_H
