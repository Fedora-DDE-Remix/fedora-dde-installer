#include <DFrame>
#include <DSearchEdit>
#include <DFontSizeManager>
#include <DListView>
#include <DMessageManager>
#include <DPaletteHelper>
#include <QItemSelectionModel>
#include <QListView>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <DBlurEffectWidget>
#include <QScrollBar>
#include <QKeyEvent>

#include "SearchableListViewDialog.h"
#include "CommonFunc.h"

SearchableListViewDialog::SearchableListViewDialog(QWidget *parent):
    QDialog (parent)
{
    setObjectName("SearchableListViewDialog");
    //setAttribute(Qt::WA_ShowModal, true);
    setWindowFlag(Qt::FramelessWindowHint);

    InitPage();
    initConnection();

    setFixedSize(300, 500);

    qApp->installEventFilter(this);
}

void SearchableListViewDialog::setListData(const QStringList &dataList, const QStringList &dataEnglishList, const QString &defaultText)
{
    m_model->clear();
    for (int i = 0; i < dataList.size(); ++i) {
        QStandardItem *standardItem = new QStandardItem(dataList.at(i));
        standardItem->setSizeHint(QSize(260, 30));
        standardItem->setData(dataEnglishList.at(i), Qt::UserRole + 1);
        m_model->appendRow(standardItem);
    }

    for (int i = 0; i < m_model->rowCount() ; ++i) {
        QModelIndex index = m_model->index(i, 0);
        if (m_model->itemFromIndex(index)->text() == defaultText) {
            m_listView->setCurrentIndex(index);
            m_currentIndex = index;
            break;
        }
    }

    if (!m_currentIndex.isValid() && m_model->rowCount() > 0) {
        m_currentIndex = m_model->index(0, 0);
        m_listView->setCurrentIndex(m_currentIndex);
    }
}

int SearchableListViewDialog::getCurrentListDataIndex()
{
    return m_currentIndex.row();
}

void SearchableListViewDialog::initConnection()
{
    // 连接搜索框的文本变化信号到过滤模型的过滤器
    connect(m_searchEdit, &DSearchEdit::textChanged, m_filterModel, &QSortFilterProxyModel::setFilterFixedString);
    connect(m_listView, &DListView::clicked, this, &SearchableListViewDialog::onCurrentListItemChanged);
}

void SearchableListViewDialog::InitPage()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 10, 0, 10);

    m_searchEdit = new DSearchEdit(this);
    //m_searchEdit->setPlaceHolder(::QObject::tr("search"));
    m_searchEdit->setObjectName("SearchableListViewDialog_searchEdit");
    m_searchEdit->setPlaceHolder("");
    m_searchEdit->setFixedWidth(280);
    m_layout->addWidget(m_searchEdit, 0, Qt::AlignHCenter);

    QHBoxLayout *listViewLayout = new QHBoxLayout;
    listViewLayout->setContentsMargins(10, 0, 2, 10);
    listViewLayout->setSpacing(0);
    m_listView = new DListView(this);
    m_listView->setFixedWidth(280);
    m_listView->setObjectName("SearchableListViewDialog_listView");
    m_listView->setBackgroundType(DStyledItemDelegate::NoBackground);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    m_listView->horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    listViewLayout->addWidget(m_listView);
    listViewLayout->addWidget(m_listView->verticalScrollBar());
    m_layout->addLayout(listViewLayout);

    m_model = new QStandardItemModel(this);

    m_filterModel = new QSortFilterProxyModel(this);
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setFilterKeyColumn(0); // 过滤第0列
    m_filterModel->setFilterRole(Qt::UserRole + 1);// 过滤角色

    m_listView->setModel(m_filterModel);

    setLayout(m_layout);
}


void SearchableListViewDialog::onCurrentListItemChanged(const QModelIndex &current)
{
    QStandardItem *item = m_model->itemFromIndex(m_filterModel->mapToSource(current));
    if (item != nullptr) {
        m_currentIndex = m_filterModel->mapToSource(current);
        Q_EMIT textChanged(item->text());
    }
}


bool SearchableListViewDialog::eventFilter(QObject *watched, QEvent *event)
{
   if (this->isHidden()) {
       return QDialog::eventFilter(watched, event);
   }

    if (watched->objectName() == "SearchableListViewDialog_searchEdit" || watched->objectName() == "SearchableListViewDialog_listView") {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *key = dynamic_cast<QKeyEvent*>(event);
            if (key != nullptr) {
                int key_value = key->key();
                switch (key_value) {
                    case Qt::Key_Return: {
                        if (m_listView->currentIndex().isValid()) {
                            onCurrentListItemChanged(m_listView->currentIndex());
                            close();
                        }
                    }
                    default: qInfo() << "key pressed not thing to do";
                }
            }
        }
    }

    return QDialog::eventFilter(watched, event);
}


void SearchableListViewDialog::showEvent(QShowEvent *event)
{
    m_searchEdit->clear();
    if (m_currentIndex.isValid()) {
        m_listView->setCurrentIndex(m_filterModel->mapFromSource(m_currentIndex));
        m_listView->scrollTo(m_filterModel->mapFromSource(m_currentIndex));
    } else {
        qWarning() << "m_currentIndex is invalid";
    }
   
    QWidget::showEvent(event);
}
