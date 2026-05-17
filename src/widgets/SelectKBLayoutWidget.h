#pragma once
#include "CommonFunc.h"
#include "KeyboardLayoutManager.h"

#include <QWidget>
#include <QStandardItemModel>
#include <DListWidget>
#include <DListView>

DWIDGET_USE_NAMESPACE
class SelectKBLayoutWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SelectKBLayoutWidget(QWidget *parent = nullptr);
    ~SelectKBLayoutWidget();
    void setItemHeight(const int height);

private slots:
//  选择键盘布局视图点击触发的槽函数
    void onSelectKBLayoutViewClicked(const QModelIndex index);

private:
//  配置选择键盘布局视图的模型
    void initSelectKBLayoutModel();
//  配置选择键盘布局视图
    void initSelectKBLayoutListView();
//  本地化键盘布局名称
    void localizeVariantList(const QString &locale);

protected:
    void showEvent(QShowEvent *event) override;

private:
    QStringList m_layoutList;
    QStringList m_list;
    DListView *m_listView = nullptr;
    QStandardItem *m_prevItem = nullptr;
    QStandardItemModel *m_model = nullptr;
    LayoutVariantsMap m_layoutMap;
    XKbLayoutVariantList m_variantList;
};
