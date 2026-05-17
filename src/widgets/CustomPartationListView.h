#ifndef CUSTOMPARTATIONLISTVIEW_H
#define CUSTOMPARTATIONLISTVIEW_H

#include "structs.h"
#include "BaseTableView.h"
#include <QItemDelegate>

DWIDGET_USE_NAMESPACE

namespace customPartationInstall {
    const int kNoIconRowLen = 200;

    const int kColumnWidth_0 = 160;
    const int kColumnWidth_1 = 120;
    const int kColumnWidth_2 = 120;
    const int kColumnWidth_3 = 220;
    const int kColumnWidth_4 = 140;
    const int kColumnWidth_5 = 160;

    const int kColumnWidthSmall_0 = 140;
    const int kColumnWidthSmall_1 = 100;
    const int kColumnWidthSmall_2 = 100;
    const int kColumnWidthSmall_3 = 80;
    const int kColumnWidthSmall_4 = 140;
    const int kColumnWidthSmall_5 = 120;

    const int kEditActionMargin = 110;
    const int kEditActionMarginSmall = 90;
    const int kActionSpace = 30;

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
    const int kColunmOffset_4 = 30;

    const int kTooltipTime = 1000;
}

class QLabel;
class QAbstractTableModel;
class DataModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit DataModel(QObject *parent = Q_NULLPTR);

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    SDataInfo getDataInfos(int iRow);
    void doItemAction(const SDataInfo &dataInfo, EOpeartType optType);
    void deleteItem(const QString &devName);
    void setAttributes(int index, const SDataInfo &dataInfo);

public slots:
    void setDatas(QMap<SDeviceInfo, QList<SPartitionInfo>> devs, QMap<SVGInfo, QList<SLVMInfo>> vgs);

signals:
    void signalItemCountChanged(int count);
    void signalPartitionEdit(const SPartitionInfo &partinfo);
    void signalPartitionDelete(const SPartitionInfo &partinfo);
    void signalPartitionAdd(const SPartitionInfo &partinfo);

    void signalVGDelete(const SVGInfo &vg);

private:
    QList<SDataInfo> m_dataInfo {};

    QList<SDataInfo> toSDataInfo(QMap<SDeviceInfo, QList<SPartitionInfo>> devs, QMap<SVGInfo, QList<SLVMInfo>> vgs);
    SDeviceInfo dataToDevice(const SDataInfo &dataInfo);
    SPartitionInfo dataToPartition(const SDataInfo &dataInfo);
    SLVMInfo dataToLVM(const SDataInfo &dataInfo);
    SVGInfo dataToVG(const SDataInfo &dataInfo);
};

class DataDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    explicit DataDelegate(QObject *parent = Q_NULLPTR);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
};


class CustomPartationListView : public BaseTableView
{
    Q_OBJECT

public:
    explicit CustomPartationListView(QWidget *parent = Q_NULLPTR);

protected:
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void drawRow(QPainter *painter, const QStyleOptionViewItem &options, const QModelIndex &index) const override;
    bool viewportEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    DataDelegate *m_itemDelegate = Q_NULLPTR;
    QStringList m_actionNames;

    QModelIndex m_hover;
    QModelIndex m_pressed;

};

#endif // CUSTOMPARTATIONLISTVIEW_H
