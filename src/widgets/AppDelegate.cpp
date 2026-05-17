#include "AppDelegate.h"
#include <DStyle>
#include <QPainter>
#include <QHBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QList>

DWIDGET_USE_NAMESPACE

AppDelegate::AppDelegate(QObject *parent):QStyledItemDelegate (parent)
{

}

QSize AppDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    return index.data(Qt::SizeHintRole).toSize();
}

void AppDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    QString text = index.data(Qt::DisplayRole).toString();
    bool hovered = option.state & QStyle::State_MouseOver;
    bool bSelected = option.state & QStyle::State_Selected;
    QIcon checkIcon = qobject_cast<DStyle*>(QApplication::style())->standardIcon(DStyle::SP_IndicatorChecked);
    QColor bgColor = hovered ? "#33000000" : "#19000000";
    QColor textColor = option.palette.color(QPalette::WindowText);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->setPen(Qt::NoPen);
    painter->setBrush(bgColor);
    painter->drawRoundedRect(option.rect, 18, 18);
    //
    if(bSelected) {
        QRect checkIconRect;
        checkIconRect.setSize(QSize(20, 20));
        checkIconRect.moveTopRight(option.rect.topRight() + QPoint(-10, 10));
        checkIcon.paint(painter, checkIconRect);
    }
    //
    QRect iconRect;
    iconRect.setSize(option.decorationSize);
    iconRect.moveCenter(QPoint(option.rect.center().x(), option.rect.topLeft().y() + 32 + iconRect.height() / 2));
    icon.paint(painter, iconRect);
    //
    QRect textRect;
    textRect.setTopLeft(QPoint(option.rect.topLeft().x() + 5, iconRect.bottomLeft().y() + 16));
    textRect.setWidth(option.rect.width() - 10);
    textRect.setHeight(option.rect.bottomLeft().y() - 5 - textRect.topLeft().y());
    painter->setPen(textColor);
    painter->drawText(textRect, Qt::TextWordWrap | Qt::AlignHCenter, text);
    painter->restore();
    //
    if (option.state & QStyle::State_HasFocus) {
        painter->save();
        QStyleOptionFocusRect o;
        o.QStyleOption::operator=(option);
        o.rect = qApp->style()->subElementRect(QStyle::SE_ItemViewItemFocusRect, &option);
        o.state |= QStyle::State_KeyboardFocusChange;
        o.state |= QStyle::State_Item;
        QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled)
                ? QPalette::Normal : QPalette::Disabled;
        o.backgroundColor = option.palette.color(cg, (option.state & QStyle::State_Selected)
                                                 ? QPalette::Highlight : QPalette::Window);
        qApp->style()->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter);
        painter->restore();
    }
}

AppViewWidget::AppViewWidget(QWidget *parent):QWidget (parent)
{
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    m_appView = new QListView;
    m_model = new QStandardItemModel;
    m_appView->setFocusPolicy(Qt::TabFocus);
    m_appView->setViewMode(QListView::IconMode);
    m_appView->setGridSize(QSize(220, 220));
    m_appView->setIconSize(QSize(64, 64));
    m_appView->setResizeMode(QListView::Adjust);
    m_appView->setFrameShape(QFrame::NoFrame);
    m_appView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_appView->setEditTriggers(QListView::NoEditTriggers);
    m_appView->setSelectionMode(QAbstractItemView::MultiSelection);
    m_appView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_appView->viewport()->setAutoFillBackground(false);
    m_appView->setAutoFillBackground(false);
    m_appView->setBackgroundRole(m_appView->viewport()->backgroundRole());
    m_appView->setModel(m_model);
    m_appView->setItemDelegate(new AppDelegate);
    mainLayout->addWidget(m_appView);
    setLayout(mainLayout);
}

void AppViewWidget::initModelData(const QString &categoryPath)
{
    m_categoryPath = categoryPath;
    QList<QFileInfo> apps = QDir(categoryPath).entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs);

    for (auto &app: apps) {
        QString appPath = app.absoluteFilePath();
        if (appPath.contains(" ") || appPath.contains(";") || appPath.contains(":")
        || appPath.contains("\"") || appPath.contains("\\")) {
            continue;
        }
        QStandardItem *item = new QStandardItem;
        item->setSizeHint(QSize(160, 160));
        item->setText(app.fileName());
        auto icons = QDir(app.absoluteFilePath()).entryInfoList({"*.png", "*.jpg", "*.jpeg", "*.svg"});
        if (icons.isEmpty()) {
            item->setIcon(QIcon(":/images/deb.svg"));
        } else {
            item->setIcon(QIcon(icons.first().absoluteFilePath()));
        }

        m_model->appendRow(item);
    }
}

QList<AppInfo> AppViewWidget::getSelectedAppInfo()
{
    auto selectList = m_appView->selectionModel()->selectedIndexes();
    QList<AppInfo> appInfos;
    for ( auto i: selectList ) {
        if (i.isValid()) {
            AppInfo appInfo;
            appInfo.appName = m_model->itemFromIndex(i)->text();
            appInfo.appPath = m_categoryPath + "/" + appInfo.appName;
            auto icons = QDir(appInfo.appPath).entryInfoList({"*.png", "*.jpg", "*.jpeg", "*.svg"});
            if (!icons.isEmpty()) {
                appInfo.appIcon = icons.first().absoluteFilePath();
            }
            appInfo.installed = false;
            appInfos.append(appInfo);
        }
    }
    return  appInfos;
}

