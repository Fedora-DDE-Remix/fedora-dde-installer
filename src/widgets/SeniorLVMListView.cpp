#include "SeniorLVMListView.h"
#include "CommonFunc.h"
#include "BaseHeaderView.h"
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
#define ITEMCOLUMN 5

//角色
#define DATA_ITEM_ROLE_ID Qt::UserRole + 1

//item各项json名称
#define ITEMICON  "icon"
#define ITEMDEVICENAME "devicename"
#define ITEMFILESYSTEM "filesystem"
#define ITEMMOUNTPOINT "mountpoint"
#define ITEMUSESIZE "usesize"
#define ITEMOPERATE "operate"
#define FLAG  "flag"
#define ISFREE  "isfree"

//item高度
#define ITEM_Height 60

//操作种数
#define  OPERATECOUNT 1

static QPixmap getActionPixmap(int action, ECheckStatus status)
{
    Q_UNUSED(action)
    QString iconpath;
    switch (status) {
        case ECheckStatus::Uncheck: {
            iconpath = ":/images/radio-button-off-out.svg";
            break;
        }
        case ECheckStatus::Checked: {
            iconpath = ":/images/radio_button.svg";
            break;
        }
        case ECheckStatus::Disable: {
            iconpath = ":/images/radio_button_disabel.svg";
            break;
        }
    }

    return CommonFunc::loadSvgImage(iconpath, createLvm::kActionIconWidth, createLvm::kActionIconHeight);
}

static QMap<int, QRect> getItemActionRect(const QRect &itemRect, ECheckStatus status)
{
    QMap<int, QRect> actions;

    for (int i = 0; i < OPERATECOUNT; i++) {
        QPixmap pixmap = getActionPixmap(i, status);
        QRect rect(itemRect.right() - (CommonFunc::isBigScreen() ?
                   createLvm::kEditActionMargin : createLvm::kEditActionMarginSmall) - pixmap.width(),
                   itemRect.top() + (itemRect.height() - pixmap.height()) / 2,
                   pixmap.width(), pixmap.height());

        actions.insert(i, rect);
    }

    return actions;
}

LVMItemDelegate::LVMItemDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}

void LVMItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QMap<int, QRect> actions;
    QList<int> flags;
    QStyleOptionViewItem newoption = option;

    QItemDelegate::paint(painter, newoption, index);

    QFontMetrics fm(option.widget->font());

    QJsonObject jsonObj = index.data().toJsonObject();
    bool isFree= jsonObj.value(ISFREE).toBool();
    ECheckStatus status = isFree ? ECheckStatus::Disable : m_selected.value(index.row());

    if (index.column() == 0) {
        newoption.rect.setLeft(createLvm::kFirstColunmOffset);
        QString strDeviceName = jsonObj.value(ITEMDEVICENAME).toString();
        QRect rec = fm.boundingRect(strDeviceName);
        if (jsonObj.value(FLAG).toBool()) {
            QPixmap pixIcon(CommonFunc::loadSvgImage(jsonObj.value(ITEMICON).toString(), createLvm::kIconWidth, createLvm::kIconHeight));
            painter->drawPixmap(newoption.rect.x(), createLvm::kIconVerticalOffset + newoption.rect.y(), createLvm::kIconWidth, createLvm::kIconHeight, pixIcon);
            strDeviceName = fm.elidedText(strDeviceName, Qt::ElideRight, CommonFunc::isBigScreen() ?
                             (createLvm::kColumnWidth_0 - createLvm::kFirstColunmOffset - createLvm::kIconWidth - createLvm::kFirstColunmSpace * 3)
                                                                                                   : (createLvm::kColumnWidthSmall_0 - createLvm::kFirstColunmOffset - createLvm::kIconWidth - createLvm::kFirstColunmSpace * 3));
            painter->drawText(createLvm::kFirstColunmOffset + createLvm::kIconWidth + createLvm::kFirstColunmSpace, createLvm::kColunmVerticalOffset + newoption.rect.y(), strDeviceName);
        } else {
            newoption.rect.setLeft(createLvm::kFirstColunmOffset + 5);
            double showSize = jsonObj.value(ITEMUSESIZE).toVariant().toDouble();
            int offset = !index.row() ? createLvm::kFirstRowOffset : createLvm::kFirstRowOffsetSmall;
            QString showUnit = CommonFunc::showSizeUnit(showSize);
            QString strDeviceSize = fm.elidedText(QString("%1%2").arg(QString::number(showSize), showUnit), Qt::ElideRight, createLvm::kColumnWidth_0);
            painter->drawText(newoption.rect.x(), offset + newoption.rect.y(), strDeviceName);
            painter->drawText(newoption.rect.x() + rec.width() + createLvm::kFirstColunmTextOffset, offset + newoption.rect.y(), strDeviceSize);
        }
    }

    if (index.column() == 1) {
        newoption.rect.setLeft((CommonFunc::isBigScreen() ? createLvm::kColumnWidth_0 : createLvm::kColumnWidthSmall_0) + createLvm::kColunmOffset);
        QString strFileSystem = fm.elidedText(jsonObj.value(ITEMFILESYSTEM).toString(), Qt::ElideRight,
                                              CommonFunc::isBigScreen() ?
                                                  (createLvm::kColumnWidth_1 - createLvm::kFirstColunmOffset)
                                                                                                                                  : (createLvm::kColumnWidthSmall_1 - createLvm::kFirstColunmOffset));
        painter->drawText(newoption.rect.x(), createLvm::kColunmVerticalOffset + newoption.rect.y(), strFileSystem);
    }

    if (index.column() == 2) {
        newoption.rect.setLeft((CommonFunc::isBigScreen() ? (createLvm::kColumnWidth_0 + createLvm::kColumnWidth_1)
                                                          : (createLvm::kColumnWidthSmall_0 + createLvm::kColumnWidthSmall_1)) + createLvm::kColunmOffset);
        QString strMountPoint = fm.elidedText(jsonObj.value(ITEMMOUNTPOINT).toString(), Qt::ElideRight,
                                              CommonFunc::isBigScreen() ?
                                                  (createLvm::kColumnWidth_2 - createLvm::kFirstColunmOffset)
                                                                                                                                  : (createLvm::kColumnWidthSmall_2 - createLvm::kFirstColunmOffset));
        painter->drawText(newoption.rect.x(), createLvm::kColunmVerticalOffset + newoption.rect.y(), strMountPoint);
    }

    if (index.column() == 3) {
        newoption.rect.setLeft((CommonFunc::isBigScreen() ? (createLvm::kColumnWidth_0 + createLvm::kColumnWidth_1 + createLvm::kColumnWidth_2)
                                                          : (createLvm::kColumnWidthSmall_0 +createLvm:: kColumnWidthSmall_1 + createLvm::kColumnWidthSmall_2)) + createLvm::kColunmOffset);
        if (jsonObj.value(FLAG).toBool()) {
            double showSize = jsonObj.value(ITEMUSESIZE).toVariant().toDouble();
            QString showUnit = CommonFunc::showSizeUnit(showSize);
            QString strUseSize= fm.elidedText(QString("%1%2").arg(QString::number(showSize), showUnit), Qt::ElideRight,
                                              CommonFunc::isBigScreen() ?
                                                  (createLvm::kColumnWidth_3 - createLvm::kFirstColunmOffset)
                                                                                                                                                  : (createLvm::kColumnWidthSmall_3 - createLvm::kFirstColunmOffset));
            painter->drawText(newoption.rect.x(), createLvm::kColunmVerticalOffset + newoption.rect.y(), strUseSize);
        }
    }

    if (index.column() == 4) {
        if (jsonObj.value(FLAG).toBool()) {
            newoption.rect.setLeft((CommonFunc::isBigScreen() ?
            (createLvm::kColumnWidth_0 + createLvm::kColumnWidth_1 + createLvm::kColumnWidth_2 + createLvm::kColumnWidth_3)
                                                              : (createLvm::kColumnWidthSmall_0 + createLvm::kColumnWidthSmall_1 + createLvm::kColumnWidthSmall_2 + createLvm::kColumnWidthSmall_3)) + createLvm::kColunmOffset);
            actions = getItemActionRect(newoption.rect, status);
            flags = actions.keys();
            foreach (int iAction, flags) {
                QPixmap pixmap = getActionPixmap(iAction, status);
                painter->drawPixmap(actions.value(iAction), pixmap);
            }
        }
    }

    painter->save();
    painter->restore();
}

QSize LVMItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int icolumn= index.column();
    QJsonObject jsonObj = index.data().toJsonObject();

    int iheight = ITEM_Height;
    if (!jsonObj.value(FLAG).toBool()) {
        if (!index.row()) {
            return QSize(CommonFunc::isBigScreen() ? createLvm::kColumnWidth_0 : createLvm::kColumnWidthSmall_0, ITEM_Height - 10);
        }
        return QSize(CommonFunc::isBigScreen() ? createLvm::kColumnWidth_0 : createLvm::kColumnWidthSmall_0, ITEM_Height - 20);
    }

    if (0 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? createLvm::kColumnWidth_0 : createLvm::kColumnWidthSmall_0, iheight);
    } else if (1 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? createLvm::kColumnWidth_1 : createLvm::kColumnWidthSmall_1, iheight);
    } else if (2 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? createLvm::kColumnWidth_2 : createLvm::kColumnWidthSmall_2, iheight);
    } else if (3 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? createLvm::kColumnWidth_3 : createLvm::kColumnWidthSmall_3, iheight);
    } else if (4 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? createLvm::kColumnWidth_4 : createLvm::kColumnWidthSmall_4, iheight);
    }

    return option.rect.size();
}

LVMItemModel::LVMItemModel(QObject *parent)
    : QAbstractTableModel(parent)
{

}

QVariant LVMItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if (section == 0) {
            return QString("    %1").arg(::QObject::tr("Partition"));
        } else if (section == 1) {
            return ::QObject::tr("File system");
        } else if (section == 2) {
            return ::QObject::tr("Mount point");
        } else if (section == 3) {
            return ::QObject::tr("Size");
        } else if (section == 4) {
            return ::QObject::tr("Action");
        }
    }
    return QVariant();
}

QList<SDataInfo> LVMItemModel::toSDataInfo(const QMap<SDeviceInfo, QList<SPartitionInfo>> &devs)
{
    QList<SDataInfo> dataInfoList;
    for (QMap<SDeviceInfo, QList<SPartitionInfo>>::const_iterator deviceiter = devs.cbegin();
         deviceiter != devs.cend(); deviceiter++) {
        SDataInfo devData;
        devData.name = deviceiter.key().name;
        devData.size = deviceiter.key().size;
        devData.device = deviceiter.key().device;
        devData.fileSystem = deviceiter.key().filesystem;
        devData.mountPoint = deviceiter.key().mountPoint;
        devData.type = EDataType::DEVICE;
        QList<SDataInfo> partInfoList;
        for (SPartitionInfo partinfo : deviceiter.value()) {
            SDataInfo partData;
            partData.id = partinfo.id;
            partData.size = partinfo.size;
            partData.fileSystem = partinfo.filesystem;
            partData.mountPoint = partinfo.mountPoint;
            partData.type = EDataType::PARTATIONS;
            partData.usage = partinfo.usage;
            partData.device = partinfo.device;
            partData.label = partinfo.label;
            partData.path = partinfo.path;
            partData.partitionType = partinfo.partitionType;
            partData.isMergeVG = partinfo.isMergeVG;
            if (ESpaceUsage::USE == partinfo.usage && !partinfo.isMergeVG && gLVM2PVFileSystem == partinfo.filesystem) {
                partInfoList.append(partData);
            }
        }

        if (!partInfoList.isEmpty()) {
             dataInfoList.append(devData);
             dataInfoList.append(partInfoList);
        }
    }

    return dataInfoList;
}

SPartitionInfo LVMItemModel::dataToPartition(const SDataInfo &dataInfo)
{
    SPartitionInfo reset;
    reset.id = dataInfo.id;
    reset.size = dataInfo.size;
    reset.filesystem = dataInfo.fileSystem;
    reset.usage = dataInfo.usage;
    reset.mountPoint = dataInfo.mountPoint;
    reset.device = dataInfo.device;
    reset.label = dataInfo.label;
    reset.path = dataInfo.path;
    reset.isMergeVG = dataInfo.isMergeVG;

    return reset;
}

void LVMItemModel::setDatas(const QMap<SDeviceInfo, QList<SPartitionInfo>> &devs)
{
    m_dataInfo.clear();
    beginResetModel();
    m_dataInfo = toSDataInfo(devs);
    endResetModel();
    Q_EMIT signalItemCountChanged(m_dataInfo.count());
}

int LVMItemModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_dataInfo.count();
}

int LVMItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ITEMCOLUMN;
}

QVariant LVMItemModel::data(const QModelIndex &index, int role) const
{
    int iRow = index.row();
    int icol = index.column();

    if (iRow >= m_dataInfo.count()) {
        return QVariant();
    }

    SDataInfo dataInfo = m_dataInfo[iRow];

    if (!index.isValid() || role != Qt::DisplayRole) {
        if (DATA_ITEM_ROLE_ID == role) {
            return dataInfo.device;
        }
        return QVariant();
    }

    QJsonObject jsonObj;

    if (EDataType::DEVICE == dataInfo.type || EDataType::VG == dataInfo.type) {
        if (icol == 0) {
            jsonObj.insert(ITEMDEVICENAME, QString("%1").arg(dataInfo.name));
            jsonObj.insert(ITEMUSESIZE, QString("%1").arg(dataInfo.size));
            jsonObj.insert(FLAG, false);
            return jsonObj;
        } else {
            return QVariant();
        }
    }

    jsonObj.insert(FLAG, true);
    bool isFree = ESpaceUsage::FREE == dataInfo.usage ? true : false;
    jsonObj.insert(ISFREE, isFree);

    if (icol == 0) {
        QString strDeviceName;
        QString iconPath = ":/images/drive-harddisk-32px.svg";
        if (EPartitionType::PartitionTypePrimary == dataInfo.partitionType) {
            iconPath = ":/images/P.svg";
        } else if (EPartitionType::PartitionTypeLogical == dataInfo.partitionType) {
            iconPath = ":/images/L.svg";
        }

        if (!isFree) {
            if (dataInfo.path.isEmpty()) {
                strDeviceName = QString("%1").arg(dataInfo.label);
            } else {
                strDeviceName = QString("%1(%2)").arg(dataInfo.label, dataInfo.path);
            }
        } else {
            strDeviceName = ::QObject::tr("free");
        }

        jsonObj.insert(ITEMICON, iconPath);
        jsonObj.insert(ITEMDEVICENAME, strDeviceName);
        return jsonObj;
    } else if (icol == 1) {
        if (!isFree) {
            jsonObj.insert(ITEMFILESYSTEM, dataInfo.fileSystem);
        }
        return jsonObj;
    } else if (icol == 2) {
        if (!isFree) {
            jsonObj.insert(ITEMMOUNTPOINT, dataInfo.mountPoint);
        }
        return jsonObj;
    } else if (icol == 3) {
        jsonObj.insert(ITEMUSESIZE, QString("%1").arg(dataInfo.size));
        return jsonObj;
    } else if (icol == 4) {
        return jsonObj;
    }

    return QVariant();
}

SDataInfo LVMItemModel::getDataInfos(int iRow)
{
    if (iRow >= m_dataInfo.count()) {
        return SDataInfo();
    }

    return m_dataInfo[iRow];
}


SeniorLVMListView::SeniorLVMListView(QWidget *parent)
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

    m_headerView = new BaseHeaderView(Qt::Horizontal, this);
    setTabOrder(m_headerView, this);
    setHeader(m_headerView);

    m_itemDelegate = new LVMItemDelegate();
    setItemDelegate(m_itemDelegate);

    connect(this, &SeniorLVMListView::clicked, this, &SeniorLVMListView::selectClickedSlot);
}

QList<SPartitionInfo> SeniorLVMListView::getPartitionInfos()
{
    return m_partitionInfos;
}

void SeniorLVMListView::resetListView()
{
    m_itemDelegate->m_selected.clear();
    m_partitionInfos.clear();
}

void SeniorLVMListView::drawRow(QPainter *painter, const QStyleOptionViewItem &options, const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    DPalette::ColorGroup cg = DPalette::Normal;

    auto *style = dynamic_cast<DStyle *>(DApplication::style());

    auto radius = style->pixelMetric(DStyle::PM_FrameRadius, &options);

    auto palette = options.palette;
    LVMItemModel *itemModel = static_cast<LVMItemModel *>(model());
    SDataInfo dataInfo = itemModel->getDataInfos(index.row());

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
    QRect rowRect { options.rect.x() + 20,
                    options.rect.y(),
                    header()->length() - header()->sectionPosition(0),
                    options.rect.height() - 10};
    rowRect.setWidth(rowRect.width() - 40);

    if (EDataType::DEVICE != dataInfo.type && EDataType::VG != dataInfo.type) {
        path.addRoundedRect(rowRect, radius, radius);
        ECheckStatus status = m_itemDelegate->m_selected.value(index.row());
        if (status == ECheckStatus::Checked) {
            auto baseColor = palette.color(DPalette::Active, DPalette::Base);
            background = style->adjustColor(baseColor, 0, 0, 60, 0, 0, 20, 5);
            background = style->adjustColor(background.color(), 0, 0, -10);
            painter->fillPath(path, background);
        } else {
            painter->fillPath(path, background);
        }
    }

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

void SeniorLVMListView::paintEvent(QPaintEvent *event)
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
    QRectF clipRect(rect.x(), rect.y() - rect.height(), rect.width(), rect.height() * 2);
    QRectF subRect(rect.x(), rect.y() - rect.height(), rect.width(), rect.height());
    QPainterPath clipPath, subPath;
    clipPath.addRoundedRect(clipRect, radius, radius);
    subPath.addRect(subRect);
    clipPath = clipPath.subtracted(subPath);

    painter.fillPath(clipPath, bgBrush);

    painter.restore();
    BaseTableView::paintEvent(event);
}

bool SeniorLVMListView::viewportEvent(QEvent *event)
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

void SeniorLVMListView::mouseMoveEvent(QMouseEvent *event)
{
    BaseTableView::mouseMoveEvent(event);
    QModelIndex index = indexAt(event->pos());
    if (index.row() < 0){
        return;
    }

    LVMItemModel *itemModel = static_cast<LVMItemModel *>(model());
    QFontMetrics fm(this->font());
    SDataInfo dataInfo = itemModel->getDataInfos(index.row());

    if (index.column() == 0) {
        QString strName;
        if (ESpaceUsage::USE == dataInfo.usage) {
            if (dataInfo.path.isEmpty()) {
                strName = QString("%1").arg(dataInfo.label);
            } else {
                strName = QString("%1(%2)").arg(dataInfo.label, dataInfo.path);
            }

            if (EDataType::LV == dataInfo.type) {
                strName = QString("%1").arg(dataInfo.name);
            }
        } else {
            strName = ::QObject::tr("free");
        }

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        if (fm.horizontalAdvance(strName) > (CommonFunc::isBigScreen() ? (createLvm::kColumnWidth_0 - createLvm::kFirstColunmOffset - createLvm::kIconWidth - createLvm::kFirstColunmSpace * 3)
                                                           : (createLvm::kColumnWidthSmall_0 - createLvm::kFirstColunmOffset - createLvm::kIconWidth - createLvm::kFirstColunmSpace * 3))) {
#else
        if (fm.width(strName) > (CommonFunc::isBigScreen() ? (createLvm::kColumnWidth_0 - createLvm::kFirstColunmOffset - createLvm::kIconWidth - createLvm::kFirstColunmSpace * 3)
                                                           : (createLvm::kColumnWidthSmall_0 - createLvm::kFirstColunmOffset - createLvm::kIconWidth - createLvm::kFirstColunmSpace * 3))) {
#endif
            QToolTip::showText(QCursor::pos(), strName, this, this->rect(), createLvm::kTooltipTime);
        }
    }

    if (index.column() == 1) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        if (fm.horizontalAdvance(dataInfo.fileSystem) > (CommonFunc::isBigScreen() ? (createLvm::kColumnWidth_1 - createLvm::kFirstColunmOffset)
                                                                       : (createLvm::kColumnWidthSmall_1 - createLvm::kFirstColunmOffset))) {
#else
        if (fm.width(dataInfo.fileSystem) > (CommonFunc::isBigScreen() ? (createLvm::kColumnWidth_1 - createLvm::kFirstColunmOffset)
                                                                       : (createLvm::kColumnWidthSmall_1 - createLvm::kFirstColunmOffset))) {
#endif
            QToolTip::showText(QCursor::pos(), dataInfo.fileSystem, this, this->rect(), createLvm::kTooltipTime);
        }
    }

    if (index.column() == 2) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        if (fm.horizontalAdvance(dataInfo.mountPoint) > (CommonFunc::isBigScreen() ? (createLvm::kColumnWidth_2 - createLvm::kFirstColunmOffset)
                                                                       : (createLvm::kColumnWidthSmall_2 - createLvm::kFirstColunmOffset))) {
#else
        if (fm.width(dataInfo.mountPoint) > (CommonFunc::isBigScreen() ? (createLvm::kColumnWidth_2 - createLvm::kFirstColunmOffset)
                                                                       : (createLvm::kColumnWidthSmall_2 - createLvm::kFirstColunmOffset))) {
#endif
            QToolTip::showText(QCursor::pos(), dataInfo.mountPoint, this, this->rect(), createLvm::kTooltipTime);
        }
    }
}

void SeniorLVMListView::mouseReleaseEvent(QMouseEvent *event)
{
    BaseTableView::mouseReleaseEvent(event);
    QModelIndex index = indexAt(event->pos());
    if (index.row() < 0){
        return;
    }
    if (index.column() != ITEMCOLUMN -1) {
        return;
    }
}

void SeniorLVMListView::selectClickedSlot(const QModelIndex &index)
{
    selectionModel()->model()->layoutChanged();
    if (m_itemDelegate->m_selected.keys().indexOf(index.row()) == -1) {
        m_itemDelegate->m_selected[index.row()] = ECheckStatus::Checked;
    } else {
        ECheckStatus status = m_itemDelegate->m_selected.value(index.row());
        if (ECheckStatus::Checked == status) {
            m_itemDelegate->m_selected[index.row()] = ECheckStatus::Uncheck;
        } else if (ECheckStatus::Uncheck == status) {
            m_itemDelegate->m_selected[index.row()] = ECheckStatus::Checked;
        }
    }

    LVMItemModel *itemModel = static_cast<LVMItemModel *>(model());
    SDataInfo dataInfo = itemModel->getDataInfos(index.row());
    SPartitionInfo info = itemModel->dataToPartition(dataInfo);
    if (m_itemDelegate->m_selected[index.row()] == ECheckStatus::Checked) {
        m_partitionInfos.append(info);
    } else {
        m_partitionInfos.removeAll(info);
    }

    if (m_partitionInfos.size() > 0) {
        Q_EMIT existSelectedItem(true);
    } else {
        Q_EMIT existSelectedItem(false);
    }
}


