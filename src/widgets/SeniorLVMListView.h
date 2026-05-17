#ifndef SENIORLVMLISTVIEW_H
#define SENIORLVMLISTVIEW_H

#include "structs.h"
#include "BaseTableView.h"
#include <QItemDelegate>

DWIDGET_USE_NAMESPACE

namespace createLvm {
    const int kColumnWidth_0 = 200;
    const int kColumnWidth_1 = 180;
    const int kColumnWidth_2 = 240;
    const int kColumnWidth_3 = 180;
    const int kColumnWidth_4 = 180;

    const int kColumnWidthSmall_0 = 140;
    const int kColumnWidthSmall_1 = 130;
    const int kColumnWidthSmall_2 = 150;
    const int kColumnWidthSmall_3 = 130;
    const int kColumnWidthSmall_4 = 130;

    const int kEditActionMargin = 145;
    const int kEditActionMarginSmall = 95;

    const int kIconWidth = 32;
    const int kIconHeight = 32;
    const int kIconVerticalOffset = 10;

    const int kActionIconWidth = 24;
    const int kActionIconHeight = 24;

    const int kFirstRowOffset = 40;
    const int kFirstRowOffsetSmall = 30;


    const int kFirstColunmOffset = 30;
    const int kFirstColunmSpace = 10;
    const int kFirstColunmTextOffset = 5;
    const int kColunmOffset = 10;
    const int kColunmVerticalOffset = 30;

    const int kTooltipTime = 1000;
}

enum class  ECheckStatus {
    Uncheck,
    Checked,
    Disable
};

class QLabel;
class QAbstractTableModel;
class LVMItemModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit LVMItemModel(QObject *parent = Q_NULLPTR);

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    SDataInfo getDataInfos(int iRow);

    SPartitionInfo dataToPartition(const SDataInfo &dataInfo);

public slots:
    void setDatas(const QMap<SDeviceInfo, QList<SPartitionInfo>> &devs);

signals:
    void signalItemCountChanged(int count);
    void signalPartitionEdit(const SPartitionInfo &partinfo);
    void signalLVMEdit(const SLVMInfo &lvminfo);
    void signalPartitionDelete(const SPartitionInfo &partinfo);
    void signalLVMDelete(const SLVMInfo &lvminfo);
    void signalPartitionAdd(const SPartitionInfo &partinfo);
    void signalLVMAdd(const SLVMInfo &lvminfo);

private:
    QList<SDataInfo> toSDataInfo(const QMap<SDeviceInfo, QList<SPartitionInfo>> &devs);

private:
    QList<SDataInfo> m_dataInfo {};
};

class LVMItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    explicit LVMItemDelegate(QObject *parent = Q_NULLPTR);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;

    QMap<int, ECheckStatus> m_selected;
};


class BaseHeaderView;
class SeniorLVMListView : public BaseTableView
{
    Q_OBJECT

public:
    explicit SeniorLVMListView(QWidget *parent = Q_NULLPTR);
    QList<SPartitionInfo> getPartitionInfos();

    void resetListView();
signals:
    void existSelectedItem(bool bflag);

protected:
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void drawRow(QPainter *painter, const QStyleOptionViewItem &options, const QModelIndex &index) const override;
    bool viewportEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

protected slots:
    void selectClickedSlot(const QModelIndex &index);

private:
    LVMItemDelegate *m_itemDelegate = Q_NULLPTR;
    BaseHeaderView *m_headerView = Q_NULLPTR;
    QStringList m_actionNames;

    QModelIndex m_hover;
    QModelIndex m_pressed;
    QList<SPartitionInfo> m_partitionInfos {};

};

#endif // SENIORLVMLISTVIEW_H
