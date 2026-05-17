#include "SelectKBLayoutWidget.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QScrollBar>
#include <libintl.h>

SelectKBLayoutWidget::SelectKBLayoutWidget(QWidget *parent) : QWidget(parent)
{
    KeyboardLayoutManager::instance()->xkbLayoutMap(m_layoutMap);
    m_model = new QStandardItemModel();
    m_listView = new DListView();
    m_listView->setItemSize(QSize(0, 61));
    m_listView->setModel(m_model);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    m_listView->horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    m_listView->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setStyleSheet("QWidget:focus {border: 2px solid; border-radius:5px; border-color:rgb(1, 128, 255)}");

    initSelectKBLayoutListView();
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(CommonFunc::isBigScreen() ? 180 : 30);
    mainLayout->addWidget(m_listView);
    mainLayout->addSpacing(CommonFunc::isBigScreen() ? 172 : 22);
    mainLayout->addWidget(m_listView->verticalScrollBar(), Qt::AlignRight);

    setLayout(mainLayout);
    connect(m_listView, QOverload<const QModelIndex&>::of(&DListView::currentChanged),
            this, &SelectKBLayoutWidget::onSelectKBLayoutViewClicked);
}

SelectKBLayoutWidget::~SelectKBLayoutWidget()
{

}

void SelectKBLayoutWidget::setItemHeight(const int height)
{
    m_listView->setItemSize(QSize(0, height));
}

void SelectKBLayoutWidget::onSelectKBLayoutViewClicked(const QModelIndex index)
{
    Q_UNUSED(index);
    QModelIndex curr = m_listView->currentIndex();
    QStandardItem *clickItem = m_model->itemFromIndex(curr);
    if (m_prevItem != nullptr) {
        m_prevItem->setCheckState(Qt::CheckState::Unchecked);
    }
    clickItem->setCheckState(Qt::CheckState::Checked);
    m_prevItem = clickItem;

    int idx = curr.row();
    if (0 <= idx && idx < m_variantList.length()) {
        QString layout = m_variantList.at(idx).layoutName;
        QString variant = m_variantList.at(idx).name;
        if (layout == variant) {
            variant.clear();
        }
        CommonFunc::setSettingsValue("DI_LAYOUT", layout);
        CommonFunc::setSettingsValue("DI_LAYOUT_VARIANT", variant);
    }

}

void SelectKBLayoutWidget::initSelectKBLayoutListView()
{
    // TODO(chenxiong): 这里需不需要处理配置为空的情况
    // 载入配置文件里的默认的语言
    QString locale = CommonFunc::getSettingsString("DI_LOCALE");
    if (locale.isEmpty()) {
        qWarning() << "CommonFunc::getSettingsString(DI_LOCALE) is failed";
        return;
    }

    QString layoutName(CommonFunc::get2AlphaCountryCode(locale).toLower());
    m_variantList = m_layoutMap["us"];

    // 如果所选的语言没有对应的键盘，那么键盘列表不发生改变
    if (m_layoutMap.contains(layoutName)) {
        m_variantList = m_layoutMap[layoutName];
    } else {
        if (!layoutName.compare("hk", Qt::CaseInsensitive) || !layoutName.compare("tw", Qt::CaseInsensitive)) {
            m_variantList = m_layoutMap["cn"];
        }
    }

    localizeVariantList(locale);

    QString layout(CommonFunc::getSettingsString("DI_LAYOUT"));
    QString variant(CommonFunc::getSettingsString("DI_LAYOUT_VARIANT"));
    if (variant.isEmpty()) {
        variant = layout;
    }

    QStringList descList;

    if (m_model->rowCount() > 0)
        m_model->clear();

    m_prevItem = nullptr;
    for (int i = 0; i < m_variantList.length(); ++i) {
        descList << m_variantList.at(i).description;
        QStandardItem *pItem = new QStandardItem(m_variantList.at(i).description);
        pItem->setCheckable(true);
        m_model->appendRow(pItem);

        if (m_variantList.at(i).name == variant
            && m_variantList.at(i).layoutName == layout) {
            pItem->setCheckState(Qt::Checked);
            m_listView->setCurrentIndex(pItem->index());
            m_prevItem = pItem;
        }
    }
    // 如果没有默认布局，那就选第一个
    if (m_prevItem == nullptr)
    {
        m_prevItem = m_model->item(0);
        m_prevItem->setCheckState(Qt::Checked);
        m_listView->setCurrentIndex(m_prevItem->index());
    }
}

void SelectKBLayoutWidget::localizeVariantList(const QString &locale)
{
    QString oldLocale = setlocale(LC_ALL, nullptr);
    qInfo() << QString("Before localized keyboard layout, old locale is %1").arg(oldLocale);

    (void) setlocale(LC_ALL, QString(locale).append(".UTF-8").toStdString().c_str());

    const char xkbDomain[] = "xkeyboard-config";
    for (auto &iter : m_variantList) {
        iter.description = dgettext(xkbDomain, iter.description.toLocal8Bit().constData());
    }

    (void) setlocale(LC_ALL, oldLocale.toStdString().c_str());
}

void SelectKBLayoutWidget::showEvent(QShowEvent *event)
{
    initSelectKBLayoutListView();

    QWidget::showEvent(event);
}
