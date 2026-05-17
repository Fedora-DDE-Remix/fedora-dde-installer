#include "PopupMenu.h"
#include "CommonFunc.h"

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QPainter>
#include <QScrollBar>

#include <DStyle>
#include <DApplication>
#include <DGuiApplicationHelper>

const int kMenuViewHorizontalMargins = 20;
const int kMenuViewMinimumWidth = 60;

const int kMenuViewMinimumHeight = 10;
const int kMenuViewMaximumHeight = 300;

const int kMenuVerticalScrollBarWidth = 8;

const int kSelectedLeftMargin = 10;
const int kSelectedTopMargin = 4;
const int kTextLeftMargin = 35;

const int kDialogRadius = 16;

PopupMenuDelegate::PopupMenuDelegate(QWidget* parent)
    : QStyledItemDelegate(parent)
    , m_isShowCheckIcon(true)
{
}

void PopupMenuDelegate::setCurrentIndex(const QModelIndex &index)
{
    m_selectRowIndex = index;
}

QSize PopupMenuDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    QSize itemSize = QStyledItemDelegate::sizeHint(option, index);
    if(m_customHeight != -1) {
        itemSize.setHeight(m_customHeight);
    }

    return itemSize;
}

void PopupMenuDelegate::setItemHeight(int height)
{
    m_customHeight = height;
}

void PopupMenuDelegate::setShowCheckIcon(const bool show)
{
    m_isShowCheckIcon = show;
}

bool PopupMenuDelegate::isShowCheckIcon() const
{
    return m_isShowCheckIcon;
}

void PopupMenuDelegate::paint(QPainter* painter,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const
{
    painter->save();

    QRect rect(option.rect);
    rect.setWidth(rect.width() - 10);
    QPainterPath path;
    auto *style = dynamic_cast<DStyle *>(DApplication::style());
    auto radius = style->pixelMetric(DStyle::PM_FrameRadius, &option);
    path.addRoundedRect(rect, radius, radius);
    QPixmap pixmap = CommonFunc::renderPixmap(":/images/popup_select.svg");
    pixmap = pixmap.scaled(pixmap.width(), rect.height() - 2 * kSelectedTopMargin, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    int yoffset = (rect.height() - pixmap.height()) / 2;
    QRect pixmapRect(rect.x() + kSelectedLeftMargin, rect.y() + yoffset, pixmap.width(), pixmap.height());
    if (m_selectRowIndex == index) {
        painter->fillPath(path, QColor("#0081FF"));
        painter->drawPixmap(pixmapRect, pixmap);
        painter->setPen(Qt::white);
    } else {
        painter->setPen(QPen(QColor("#434f69")));
    }

    if (option.state & QStyle::State_MouseOver && m_selectRowIndex != index) {
        painter->fillPath(path, QColor("#E0E0E0"));
    }

    // TODO(chenxiong): change text color according to frame
    QString text = index.model()->data(index, Qt::DisplayRole).toString();
    QRect textRect(rect);

    int offset = kTextLeftMargin;
    if (!m_isShowCheckIcon) {
        offset -= kSelectedLeftMargin - 2;
    }
    textRect.setX(textRect.x() + offset);

    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);

    painter->restore();
}

PopupMenu::PopupMenu(QWidget* parent)
    : DAbstractDialog(parent)
{
    m_menuModel = new ListModel(this);

    m_popupDelegate = new PopupMenuDelegate(this);

    m_menuView = new DListView(this);
    m_menuView->setFocusPolicy(Qt::TabFocus);
    m_menuView->setModel(m_menuModel);
    m_menuView->setItemDelegate(m_popupDelegate);

    m_menuView->setContentsMargins(0, 0, 0, 0);
    m_menuView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_menuView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_menuView->setUniformItemSizes(true);
    m_menuView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_menuView->setSelectionMode(QListView::SingleSelection);
    m_menuView->setEditTriggers(QListView::NoEditTriggers);
    m_menuView->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    m_menuView->horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    m_menuView->setMouseTracking(true);

    connect(m_menuView, &QListView::pressed, this, &PopupMenu::onSelected);
    connect(m_menuView, &QListView::entered, this, &PopupMenu::onSelected);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(kDialogRadius);
    mainLayout->addWidget(m_menuView, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(kDialogRadius);

    setContentsMargins(10, 0, 10, 0);
    setLayout(mainLayout);

    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
}

void PopupMenu::popup(const QPoint& pos, QWidget* widget)
{
    QPoint pt = QPoint(pos.x() - size().width() / 2, pos.y() - size().height());
    QWidget* parentWdg = nullptr;

    if (widget != nullptr) {
        parentWdg = widget->parentWidget();
    }

    if (parentWdg != nullptr) {
        QPoint parentPos = parentWdg->pos();
        if (pt.x() < parentPos.x() + 10) {
            pt.setX(parentPos.x() + 10);
        }

        if (pt.y() < parentPos.y() + 10) {
            pt.setY(parentPos.y() + 10);
        }
    }

    move(pt);
    setModal(m_popupDelegate->isShowCheckIcon());
    show();
}

void PopupMenu::popup(QWidget* widget)
{
    if (widget == nullptr) {
        return;
    }

    QPoint pt;
    pt = widget->mapToGlobal(QPoint());
    pt.setY(pt.y() - this->height() - 10);

    move(pt);
    setModal(m_popupDelegate->isShowCheckIcon());
    show();
}

void PopupMenu::setStringList(const QStringList& strings)
{
    m_menuModel->setStringList(strings);

    int itemWidth = kMenuViewMinimumWidth;
    const QFontMetrics metrics(m_menuView->font());

    for (const QString& str : strings) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        itemWidth = qMax(metrics.horizontalAdvance(str), itemWidth);
#else
        itemWidth = qMax(metrics.width(str), itemWidth);
#endif
    }

    const int width = itemWidth + kMenuViewHorizontalMargins * 2 + kMenuVerticalScrollBarWidth;

    int hgt = kMenuViewMinimumHeight;

    // TODO(chenxiong): 此处通过函数获取字体之间的间隙
    hgt = qMax(hgt, m_popupDelegate->ItemHeight() * strings.length());
    hgt = qMin(hgt, kMenuViewMaximumHeight);

    m_menuView->setFixedSize(width, hgt);
    m_menuView->adjustSize();

    setFixedSize(width + 10, hgt + 2 * kDialogRadius);
    adjustSize();
    update();
}

void PopupMenu::setCurrentRow(int row)
{
    if (row < 0 || row >= m_menuModel->stringList().length()) {
        return;
    }
    m_popupDelegate->setCurrentIndex(m_menuModel->index(row));
    m_menuView->setCurrentIndex(m_menuModel->index(row));
}

void PopupMenu::setShowCheckIcon(const bool show)
{
    m_popupDelegate->setShowCheckIcon(show);
}

void PopupMenu::setItemHeight(int height)
{
    if(m_popupDelegate) {
        m_popupDelegate->setItemHeight(height);
    }
}

void PopupMenu::hideEvent(QHideEvent* event)
{
    emit onHide();

    DAbstractDialog::hideEvent(event);
}

void PopupMenu::onSelected(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    int row = index.row();
    if (row < m_menuModel->stringList().length()) {
        setVisible(m_popupDelegate->isShowCheckIcon());
        emit menuSelected(row);
        m_popupDelegate->setCurrentIndex(m_menuModel->index(row));
    }
}

ListModel::ListModel(QObject *parent)
    : QStringListModel (parent)
{
}

void ListModel::setStringList(const QStringList &strings)
{
    beginResetModel();
    QStringListModel::setStringList(strings);
    endResetModel();
}
