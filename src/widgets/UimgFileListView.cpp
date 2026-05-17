#include "UimgFileListView.h"
#include "CommonFunc.h"
#include "globals.h"

#include <DLabel>
#include <DApplication>
#include <DGuiApplicationHelper>

#include <QAbstractTableModel>
#include <QJsonObject>
#include <QToolTip>
#include <QMouseEvent>
#include <QTimer>
#include <QHeaderView>
#include <QPainterPath>

//表的列数
#define ITEMCOLUMN 3

//角色
#define DATA_ITEM_ROLE_ID Qt::UserRole + 1

//item各项json名称
#define ITEMICON  "icon"
#define ITEMDEVICENAME "devicename"
#define ITEMFILEPATH "filepath"

//item高度
#define ITEM_Height 50

static QPixmap getActionPixmap(bool isChecked)
{
    QString iconpath = isChecked ? ":/images/radio_button.svg" : ""/*":/images/radio-button-off-out.svg"*/;
    return CommonFunc::loadSvgImage(iconpath, uimgFileList::kActionIconWidth, uimgFileList::kActionIconHeight);
}

static QRect getItemActionRect(const QRect &itemRect, bool isChecked)
{
    QMap<int, QRect> actions;
    QPixmap pixmap = getActionPixmap(isChecked);
    QRect rect(itemRect.right() - uimgFileList::kEditActionMargin - pixmap.width(),
               itemRect.top() + (itemRect.height() - pixmap.height()) / 2,
               pixmap.width(), pixmap.height());

    return rect;
}

UimgFileDelegate::UimgFileDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}

void UimgFileDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem newoption = option;

    QItemDelegate::paint(painter, newoption, index);

    QFontMetrics fm(option.widget->font());

    QJsonObject jsonObj = index.data().toJsonObject();
    bool isChecked = m_selected.value(index.row());

    if (index.column() == 0) {
        newoption.rect.setLeft(uimgFileList::kFirstColunmOffset);
        QString strDeviceName = jsonObj.value(ITEMDEVICENAME).toString();
        QPixmap pixIcon(CommonFunc::loadSvgImage(jsonObj.value(ITEMICON).toString(), uimgFileList::kIconWidth, uimgFileList::kIconHeight));
        painter->drawPixmap(newoption.rect.x(), uimgFileList::kIconVerticalOffset + newoption.rect.y(), uimgFileList::kIconWidth, uimgFileList::kIconHeight, pixIcon);
        strDeviceName = fm.elidedText(strDeviceName, Qt::ElideRight, uimgFileList::kColumnWidth_0 - uimgFileList::kFirstColunmOffset - uimgFileList::kIconWidth - uimgFileList::kFirstColunmSpace * 3);
        painter->drawText(uimgFileList::kFirstColunmOffset + uimgFileList::kIconWidth + uimgFileList::kFirstColunmSpace, uimgFileList::kColunmVerticalOffset + newoption.rect.y(), strDeviceName);
    }

    if (index.column() == 1) {
        newoption.rect.setLeft(uimgFileList::kColumnWidth_0 + uimgFileList::kColunmOffset);
        QString strFilePath = fm.elidedText(jsonObj.value(ITEMFILEPATH).toString(), Qt::ElideRight, uimgFileList::kColumnWidth_1 - uimgFileList::kFirstColunmOffset);
        painter->drawText(newoption.rect.x(), uimgFileList::kColunmVerticalOffset + newoption.rect.y(), strFilePath);
    }

    if (index.column() == 2) {
        newoption.rect.setLeft(uimgFileList::kColumnWidth_0 + uimgFileList::kColumnWidth_1 + uimgFileList::kColunmOffset);
        QRect rec = getItemActionRect(newoption.rect, isChecked);
        QPixmap pixmap = getActionPixmap(isChecked);
        painter->drawPixmap(rec, pixmap);
    }

    painter->save();
    painter->restore();
}

QSize UimgFileDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int icolumn= index.column();
    if (0 == icolumn) {
        return QSize(uimgFileList::kColumnWidth_0, ITEM_Height);
    } else if (1 == icolumn) {
        return QSize(uimgFileList::kColumnWidth_1, ITEM_Height);
    } else if (2 == icolumn) {
        return QSize(uimgFileList::kColumnWidth_2, ITEM_Height);
    }

    return option.rect.size();
}

UimgFileModel::UimgFileModel(QObject *parent)
    : QAbstractTableModel(parent)
{

}

void UimgFileModel::setDatas(const QList<SUimgFileInfo> &files)
{
    beginResetModel();
    m_dataInfo.clear();
    m_dataInfo = files;
    endResetModel();
}

int UimgFileModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_dataInfo.count();
}

int UimgFileModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ITEMCOLUMN;
}

QVariant UimgFileModel::data(const QModelIndex &index, int role) const
{
    int iRow = index.row();
    int icol = index.column();

    if (iRow >= m_dataInfo.count()) {
        return QVariant();
    }

    SUimgFileInfo dataInfo = m_dataInfo[iRow];

    if (!index.isValid() || role != Qt::DisplayRole) {
        if (DATA_ITEM_ROLE_ID == role) {
            return dataInfo.path;
        }
        return QVariant();
    }

    QJsonObject jsonObj;

    if (icol == 0) {
        QString strDeviceName = QString("%1").arg(dataInfo.device);;
        QString iconPath = ":/images/uimg.svg";
        jsonObj.insert(ITEMICON, iconPath);
        jsonObj.insert(ITEMDEVICENAME, strDeviceName);
        return jsonObj;
    } else if (icol == 1) {
        jsonObj.insert(ITEMFILEPATH, dataInfo.path);
        return jsonObj;
    } else if (icol == 2) {
        return jsonObj;
    }

    return QVariant();
}

SUimgFileInfo UimgFileModel::getDataInfos(int iRow)
{
    if (iRow >= m_dataInfo.count()) {
        return SUimgFileInfo();
    }

    return m_dataInfo[iRow];
}


UimgFileListView::UimgFileListView(QWidget *parent)
    : BaseTableView(parent)
    , m_itemDelegate(Q_NULLPTR)
{
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);
    setContextMenuPolicy(Qt::NoContextMenu);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::NoSelection);
    setFrameShape(QFrame::NoFrame);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setHeaderHidden(true);

    m_itemDelegate = new UimgFileDelegate();
    setItemDelegate(m_itemDelegate);

    m_notFoundLabel = new DLabel(::QObject::tr("No uimg files found"), this);
    m_notFoundLabel->setAlignment(Qt::AlignCenter);
    DPalette pe = m_notFoundLabel->palette();
    pe.setColor(DPalette::WindowText, QColor("#FF7400"));
    m_notFoundLabel->setBackgroundRole(DPalette::Window);
    m_notFoundLabel->setPalette(pe);

    connect(this, &UimgFileListView::clicked, this, &UimgFileListView::selectClickedSlot);
}

QString UimgFileListView::getSelectFilePath()
{
    return m_selectFilePath;
}

void UimgFileListView::clearSelectStatus()
{
    m_selectFilePath = Q_NULLPTR;
    m_itemDelegate->m_selected.clear();
}

void UimgFileListView::setNotFoundLabelHidden(bool isHidden)
{
    m_notFoundLabel->setHidden(isHidden);
}

void UimgFileListView::drawRow(QPainter *painter, const QStyleOptionViewItem &options, const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    DPalette::ColorGroup cg = DPalette::Normal;

    auto *style = dynamic_cast<DStyle *>(DApplication::style());

    auto radius = style->pixelMetric(DStyle::PM_FrameRadius, &options);
    auto margin = style->pixelMetric(DStyle::PM_ContentsMargins, &options);
    auto palette = options.palette;
    QBrush background;
    QStyleOptionViewItem opt(options);
    opt.rect.adjust(0, 0, 10, -10);
    if (options.state & DStyle::State_Enabled) {
        if (selectionModel()->isSelected(index)) {
            background = palette.color(cg, DPalette::Highlight);
            // #ref: DStyle::generatedBrush
            if (m_pressed.isValid() && m_pressed.row() == index.row()) {
                // pressed
                background = style->adjustColor(background.color(), 0, 0, -10);
                opt.state = options.state | QStyle::State_Sunken;
            } else if (m_hover.isValid() && m_hover.row() == index.row()) {
                // hovered
                background = style->adjustColor(background.color(), 0, 0, 20);
            }
        } else {
            auto baseColor = palette.color(DPalette::Active, DPalette::Base);

            if (m_pressed.isValid() && m_pressed.row() == index.row()) {
                // pressed
                background = style->adjustColor(baseColor, 0, 0, 60, 0, 0, 20, 5);
            } else if (m_hover.isValid() && m_hover.row() == index.row()) {
                // hovered
                DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();

                if (themeType == DGuiApplicationHelper::LightType) {
                    background = style->adjustColor(baseColor, 0, 0, -10);
                }
                else if (themeType == DGuiApplicationHelper::DarkType) {
                    background = style->adjustColor(baseColor, 0, 0, 20);
                }
            }
            else {
                background = palette.color(DPalette::Normal, DPalette::AlternateBase);
            }
        }
    }

    // draw row background
    QPainterPath path;
    QRect rowRect { options.rect.x() - header()->offset(),
                    options.rect.y(),
                    header()->length() - header()->sectionPosition(0),
                    options.rect.height() - 10};
    rowRect.setX(rowRect.x() + margin);
    rowRect.setWidth(rowRect.width() - margin);
    path.addRoundedRect(rowRect, radius, radius);
    painter->fillPath(path, background);

    BaseTableView::drawRow(painter, opt, index);

    // draw focus
    if (hasFocus() && currentIndex().row() == index.row()) {
        QStyleOptionFocusRect o;
        o.QStyleOption::operator=(options);
        o.state |= QStyle::State_KeyboardFocusChange | QStyle::State_HasFocus;
        o.rect = style->visualRect(layoutDirection(), viewport()->rect(), rowRect);
        style->drawPrimitive(DStyle::PE_FrameFocusRect, &o, painter);
    }

    painter->restore();
}

void UimgFileListView::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    painter.save();
    painter.setRenderHints(QPainter::Antialiasing);
    painter.setOpacity(1);
    painter.setClipping(true);

    QWidget *wnd = DApplication::activeWindow();
    DPalette::ColorGroup cg;
    if (!wnd) {
        cg = DPalette::Inactive;
    } else {
        cg = DPalette::Active;
    }

    auto style = dynamic_cast<DStyle *>(DApplication::style());
    auto palette = DPalette();

    QBrush bgBrush(palette.color(cg, DPalette::Window));
    QStyleOptionFrame option;
    initStyleOption(&option);
    int radius = style->pixelMetric(DStyle::PM_FrameRadius, &option);
    QRect rect = viewport()->rect();
    QPainterPath pPath;
    pPath.addRoundedRect(rect, radius, radius);
    painter.fillPath(pPath, bgBrush);
    painter.restore();
    BaseTableView::paintEvent(event);
}

void UimgFileListView::resizeEvent(QResizeEvent *event)
{
    BaseTableView::resizeEvent(event);
    m_notFoundLabel->setGeometry(0, 0, event->size().width(), event->size().height());
}

bool UimgFileListView::viewportEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::HoverLeave: {
        auto rect = visualRect(m_hover);
        rect.setX(0);
        rect.setWidth(viewport()->width());
        m_hover = QModelIndex();
        viewport()->update(rect);
        break;
    }
    case QEvent::HoverEnter:
    case QEvent::HoverMove: {
        auto *hev = dynamic_cast<QHoverEvent *>(event);
        m_hover = indexAt(hev->position().toPoint());
        auto rect = visualRect(m_hover);
        rect.setX(0);
        rect.setWidth(viewport()->width());
        viewport()->update(rect);
        break;
    }
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress: {
        auto *mev = dynamic_cast<QMouseEvent *>(event);
        auto newIndex = indexAt(mev->pos());
        QRegion region;
        QRect rect;
        if (m_pressed.isValid()) {
            rect = visualRect(m_pressed);
            rect.setX(0);
            rect.setWidth(viewport()->width());
            region += rect;
        }
        if (newIndex.isValid() && newIndex.row() != m_pressed.row()) {
            rect = visualRect(newIndex);
            rect.setX(0);
            rect.setWidth(viewport()->width());
            region += rect;
        }
        m_pressed = (mev->button() == Qt::LeftButton &&
                     (mev->type() == QEvent::MouseButtonPress ||
                      mev->type() == QEvent::MouseButtonDblClick)) ? newIndex : QModelIndex();
        viewport()->update(region);
        break;
    }
    default:
        break;
    }
    return BaseTableView::viewportEvent(event);
}

void UimgFileListView::mouseMoveEvent(QMouseEvent *event)
{
    BaseTableView::mouseMoveEvent(event);
    QModelIndex index = indexAt(event->pos());
    if (index.row() < 0){
        return;
    }

    UimgFileModel *itemModel = static_cast<UimgFileModel *>(model());
    QFontMetrics fm(this->font());
    SUimgFileInfo dataInfo = itemModel->getDataInfos(index.row());

    if (index.column() == 0) {
        QString strName = QString("%1").arg(dataInfo.device);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        if (fm.horizontalAdvance(strName) > (uimgFileList::kColumnWidth_0 - uimgFileList::kFirstColunmOffset - uimgFileList::kIconWidth - uimgFileList::kFirstColunmSpace * 3)) {
#else
        if (fm.width(strName) > (uimgFileList::kColumnWidth_0 - uimgFileList::kFirstColunmOffset - uimgFileList::kIconWidth - uimgFileList::kFirstColunmSpace * 3)) {
#endif
            QToolTip::showText(QCursor::pos(), strName, this, this->rect(), uimgFileList::kTooltipTime);
        }
    }

    if (index.column() == 1) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        if (fm.horizontalAdvance(dataInfo.path) > (uimgFileList::kColumnWidth_1 - uimgFileList::kFirstColunmOffset)) {
#else
        if (fm.width(dataInfo.path) > (uimgFileList::kColumnWidth_1 - uimgFileList::kFirstColunmOffset)) {
#endif
            QToolTip::showText(QCursor::pos(), dataInfo.path, this, this->rect(), uimgFileList::kTooltipTime);
        }
    }
}

void UimgFileListView::selectClickedSlot(const QModelIndex &index)
{
    clearSelectStatus();
    selectionModel()->model()->layoutChanged();
    if (m_itemDelegate->m_selected.keys().indexOf(index.row()) == -1) {
        m_itemDelegate->m_selected[index.row()] = true;
    } else {
        if (m_itemDelegate->m_selected[index.row()]) {
            m_itemDelegate->m_selected[index.row()] = false;
        } else {
            m_itemDelegate->m_selected[index.row()] = true;
        }
    }

    UimgFileModel *itemModel = static_cast<UimgFileModel *>(model());
    SUimgFileInfo dataInfo = itemModel->getDataInfos(index.row());
    if (m_itemDelegate->m_selected[index.row()]) {
        m_selectFilePath = dataInfo.path;
        Q_EMIT existSelectedItem(true);
    } else {
        m_selectFilePath = Q_NULLPTR;
        Q_EMIT existSelectedItem(false);
    }
}


