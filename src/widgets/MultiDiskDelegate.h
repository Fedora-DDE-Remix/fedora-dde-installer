#ifndef MULTIDISKDELEGATE_H
#define MULTIDISKDELEGATE_H

#include <QObject>
#include <QStyledItemDelegate>

class MultiDiskDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit MultiDiskDelegate(QObject *parent = nullptr);

protected:
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // MULTIDISKDELEGATE_H
