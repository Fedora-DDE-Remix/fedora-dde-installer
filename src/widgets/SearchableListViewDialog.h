#ifndef SEARCHABLELISTVIEWDIALOG_H
#define SEARCHABLELISTVIEWDIALOG_H

#include <DDialog>
#include <DLabel>
#include <QStackedWidget>
#include <QTimer>
#include <DListView>
#include <DSearchEdit>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QSortFilterProxyModel>

DWIDGET_USE_NAMESPACE

class SearchableListViewDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SearchableListViewDialog(QWidget *parent = Q_NULLPTR);
    void setListData(const QStringList &dataList, const QStringList &dataEnglishList, const QString &defaultText);
    int getCurrentListDataIndex();

signals:
    void textChanged(const QString &currentText);

private:
    void InitPage();
    void initConnection();
    void onCurrentListItemChanged(const QModelIndex &current);

private Q_SLOTS:

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QVBoxLayout *m_layout = nullptr;
    DSearchEdit *m_searchEdit = nullptr;
    DListView *m_listView = nullptr;
    QStandardItemModel *m_model = nullptr;
    QSortFilterProxyModel *m_filterModel = nullptr;
    QModelIndex m_currentIndex;
};

#endif // SEARCHABLELISTVIEWDIALOG_H
