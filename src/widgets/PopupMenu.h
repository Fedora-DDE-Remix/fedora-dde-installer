#ifndef POPUPMENU_H
#define POPUPMENU_H

#include <QFrame>
#include <QStyledItemDelegate>
#include <QListView>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QStringListModel>

#include <DAbstractDialog>
#include <DListView>

DWIDGET_USE_NAMESPACE

class ListModel : public QStringListModel {
    Q_OBJECT

public:
    explicit ListModel(QObject *parent = nullptr);

    void setStringList(const QStringList &strings);
};

class PopupMenuDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit PopupMenuDelegate(QWidget* parent = nullptr);

    void setShowCheckIcon(const bool show);
    bool isShowCheckIcon() const;
    void setCurrentIndex(const QModelIndex &index);
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    void setItemHeight(int height);

    int ItemHeight() {
        return m_customHeight;
    }

protected:
    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

private:
    QModelIndex m_selectRowIndex = QModelIndex();
    bool m_isShowCheckIcon = true;
    int m_customHeight = 24;
};

class PopupMenu : public DAbstractDialog {
    Q_OBJECT

public:
    explicit PopupMenu(QWidget* parent = nullptr);

    // 在pos位置点show，同时不超过伴随控件父控件的范围
    void popup(const QPoint& pos, QWidget *widget = nullptr);
    void popup(QWidget *widget);

    void setStringList(const QStringList& strings);

    void setCurrentRow(int row);

    void setShowCheckIcon(const bool show);

    void setItemHeight(int height);

signals:
    // 控件隐藏时发射的信号
    void onHide();

    void menuSelected(int index);

protected:
    void hideEvent(QHideEvent* event) override;

private:
    void onSelected(const QModelIndex& index);

    DListView* m_menuView = nullptr;
    ListModel* m_menuModel = nullptr;
    PopupMenuDelegate* m_popupDelegate = nullptr;
};

#endif // POPUPMENU_H
