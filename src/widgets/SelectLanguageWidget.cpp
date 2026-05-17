#include "SelectLanguageWidget.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QScrollBar>

SelectLanguageWidget::SelectLanguageWidget(QWidget *parent) : QWidget(parent)
{
    m_selectLanguageModel = new QStandardItemModel();
    m_selectLanguageListView = new DListView();
    m_selectLanguageListView->setItemSize(QSize(0, 61));
    m_selectLanguageListView->setModel(m_selectLanguageModel);
    m_selectLanguageListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_selectLanguageListView->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    m_selectLanguageListView->horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    m_selectLanguageListView->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_selectLanguageListView->setStyleSheet(""
                                  "QWidget:focus {"
                                  "border: 2px solid;"
                                  "border-radius:5px;"
                                  "border-color:rgb(1, 128, 255)"
                                  "}");
    m_selectLanguageListView->setSelectionMode(QAbstractItemView::SingleSelection);
    initSelectLanguageModel();
    initSelectLanguageListView();
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_selectLanguageListView);
    setLayout(mainLayout);
    connect(m_selectLanguageListView, QOverload<const QModelIndex&>::of(&DListView::currentChanged),
            this, &SelectLanguageWidget::onSelectLanguageViewClicked);
}

SelectLanguageWidget::~SelectLanguageWidget()
{

}

SLanguageItem SelectLanguageWidget::getCurrentItem()
{
    return m_currentItem;
}

void SelectLanguageWidget::setItemHeight(const int height)
{
    m_selectLanguageListView->setItemSize(QSize(0, height));
}

void SelectLanguageWidget::setVerticalBar(QScrollBar *vBar)
{
    if(m_selectLanguageListView) {
        m_selectLanguageListView->setVerticalScrollBar(vBar);
    }
}

void SelectLanguageWidget::onSelectLanguageViewClicked(const QModelIndex preIndex)
{
    Q_UNUSED(preIndex);
    QModelIndex curr = m_selectLanguageListView->currentIndex();
    QStandardItem *clickItem = m_selectLanguageModel->itemFromIndex(curr);
    if (m_previousItem != nullptr) {
        m_previousItem->setCheckState(Qt::CheckState::Unchecked);
    }
    clickItem->setCheckState(Qt::CheckState::Checked);
    m_previousItem = clickItem;
    m_currentItem = m_languageList.at(curr.row());
    emit selectLanguageViewClicked(m_currentItem);
}


void SelectLanguageWidget::initSelectLanguageModel()
{
    m_languageList = CommonFunc::getLanguageList();
    for (auto languageItem : m_languageList) {
        QStandardItem *item = new QStandardItem(languageItem.localName);
        m_localList << languageItem.locale;
        item->setCheckable(true);
        m_selectLanguageModel->appendRow(item);
    }
}

void SelectLanguageWidget::initSelectLanguageListView()
{
    // TODO(chenxiong): 这里需不需要处理配置为空的情况
    // 载入配置文件里的默认的语言
    QString locale = CommonFunc::getSettingsString("DI_LOCALE");
    if (locale.isEmpty()) {
        qWarning() << "CommonFunc::getSettingsString(DI_LOCALE) is failed";
        return;
    }

    int index = m_localList.indexOf(locale);
    if (index < 0) {
        qWarning() << QString("Cann't find %1 in locale list").arg(locale);
        return;
    }

    QString name = m_languageList.at(index).localName;
    QList<QStandardItem*> items = m_selectLanguageModel->findItems(name);
    if (items.count() < 1) {
        qWarning() << QString("Items that match the given text %1 is empty").arg(name);
        return;
    }

    if (m_previousItem != nullptr) {
        m_previousItem->setCheckState(Qt::CheckState::Unchecked);
    }

    // 设置并勾选默认的语言
    QStandardItem *cur = items.first();

    cur->setCheckState(Qt::CheckState::Checked);
    m_selectLanguageListView->setCurrentIndex(m_selectLanguageModel->index(cur->row(), cur->column()));

    m_previousItem = cur;

    m_currentItem = m_languageList.at(index);
}

void SelectLanguageWidget::showEvent(QShowEvent *event)
{
    initSelectLanguageListView();

    QWidget::showEvent(event);
}
