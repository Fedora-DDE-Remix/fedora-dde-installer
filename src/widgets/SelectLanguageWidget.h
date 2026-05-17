#ifndef SELECTLANGUAGEWIDGET_H
#define SELECTLANGUAGEWIDGET_H
#include "CommonFunc.h"

#include <QWidget>
#include <QStandardItemModel>
#include <DListWidget>
#include <DListView>

DWIDGET_USE_NAMESPACE
class SelectLanguageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SelectLanguageWidget(QWidget *parent = nullptr);
    ~SelectLanguageWidget();
    SLanguageItem getCurrentItem();
    void setItemHeight(const int height);
    void setVerticalBar(QScrollBar *vBar);
signals:
//  选择语言视图被点击会发的信号
    void selectLanguageViewClicked(const SLanguageItem item);
private slots:
//  选择语言视图点击触发的槽函数
    void onSelectLanguageViewClicked(const QModelIndex preIndex);

private:
//  配置选择语言视图的模型
    void initSelectLanguageModel();
//  配置选择语言视图
    void initSelectLanguageListView();

protected:
    void showEvent(QShowEvent *event) override;

private:
    LanguageList m_languageList;
    QStringList m_localList;
    SLanguageItem m_currentItem;
    DListView *m_selectLanguageListView = nullptr;
    QStandardItemModel *m_selectLanguageModel = nullptr;
    QStandardItem *m_previousItem = nullptr;
};

#endif // SELECTLANGUAGEWIDGET_H
