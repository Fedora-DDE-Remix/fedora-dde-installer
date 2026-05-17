#ifndef APPDELEGATE_H
#define APPDELEGATE_H

#include "structs.h"
#include <QStyledItemDelegate>
#include <QWidget>
#include <QListView>
#include <QStandardItemModel>

class AppDelegate: public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit AppDelegate(QObject *parent = nullptr);
protected:
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class AppViewWidget:public QWidget
{
    Q_OBJECT
public:
    explicit AppViewWidget(QWidget *parent = nullptr);

    void initModelData(const QString &categoryPath);
    QList<AppInfo> getSelectedAppInfo();
private:
    QListView *m_appView;
    QStandardItemModel *m_model;
    QString m_categoryPath;

};

#endif // APPDELEGATE_H
