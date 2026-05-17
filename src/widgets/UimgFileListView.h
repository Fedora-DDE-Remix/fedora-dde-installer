#ifndef UIMGFILELISTVIEW_H
#define UIMGFILELISTVIEW_H

#include "structs.h"
#include "BaseTableView.h"
#include <QItemDelegate>

#include <DLabel>

DWIDGET_USE_NAMESPACE

namespace uimgFileList {
    const int kColumnWidth_0 = 120;
    const int kColumnWidth_1 = 200;
    const int kColumnWidth_2 = 60;

    const int kIconWidth = 24;
    const int kIconHeight = 24;
    const int kIconVerticalOffset = 8;

    const int kEditActionMargin = 20;

    const int kActionIconWidth = 20;
    const int kActionIconHeight = 20;

    const int kFirstRowOffset = 55;

    const int kFirstColunmOffset = 20;
    const int kFirstColunmSpace = 10;
    const int kFirstColunmTextOffset = 5;
    const int kColunmOffset = 10;
    const int kColunmVerticalOffset = 25;

    const int kTooltipTime = 1000;
}

class QAbstractTableModel;
class UimgFileModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit UimgFileModel(QObject *parent = Q_NULLPTR);

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    SUimgFileInfo getDataInfos(int iRow);

public slots:
    void setDatas(const QList<SUimgFileInfo> &files);

private:
    QList<SUimgFileInfo> m_dataInfo {};
};

class UimgFileDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    explicit UimgFileDelegate(QObject *parent = Q_NULLPTR);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;

    QMap<int, bool> m_selected;
};


class BaseHeaderView;
class UimgFileListView : public BaseTableView
{
    Q_OBJECT

public:
    explicit UimgFileListView(QWidget *parent = Q_NULLPTR);
    QString getSelectFilePath();
    void clearSelectStatus();
    void setNotFoundLabelHidden(bool isHidden);

signals:
    void existSelectedItem(bool bflag);

protected:
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void drawRow(QPainter *painter, const QStyleOptionViewItem &options, const QModelIndex &index) const override;
    bool viewportEvent(QEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

protected slots:
    void selectClickedSlot(const QModelIndex &index);

private:
    UimgFileDelegate *m_itemDelegate = Q_NULLPTR;
    DLabel *m_notFoundLabel = Q_NULLPTR;

    QModelIndex m_hover;
    QModelIndex m_pressed;
    QString m_selectFilePath = Q_NULLPTR;

};

#endif // UIMGFILELISTVIEW_H
