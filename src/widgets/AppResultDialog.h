#ifndef APPRESULTDIALOG_H
#define APPRESULTDIALOG_H

#include "structs.h"
#include <QObject>
#include <DDialog>
#include <QListView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
DWIDGET_USE_NAMESPACE

class AppResultDelegate:public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit AppResultDelegate(QObject *parent = nullptr);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class AppResultDialog : public DDialog
{
    Q_OBJECT
public:
    explicit AppResultDialog(QWidget *parent = nullptr);

    void loadAppResult();
private:
    QListView *m_appview;
    QStandardItemModel *m_model;
};

#endif // APPRESULTDIALOG_H
