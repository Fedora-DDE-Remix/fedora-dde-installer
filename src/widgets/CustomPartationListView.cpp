#include "CustomPartationListView.h"
#include "CommonFunc.h"

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
#define ITEMCOLUMN 6

//角色
#define DATA_ITEM_ROLE_ID Qt::UserRole + 1

//item各项json名称
#define ITEMICON  "icon"
#define ITEMDEVICENAME "devicename"
#define ITEMUSESIZE "usesize"
#define ITEMFILESYSTEM "filesystem"
#define ITEMMOUNTPOINT "mountpoint"
#define ITEMINSTALLTAG "installtag"
#define ITEMOPERATE "operate"
#define ISDEV  "isdev"
#define ISVG  "isvg"
#define ISFREE  "isfree"
#define ISMERGEVG "ismergevg"
#define ISLV  "islv"

//item高度
#define ITEM_Height 60

//操作种数
#define OPERATECOUNT 2

static QStringList getOpearts(const SDataInfo &info)
{
    QStringList opts = {};
    if (EDataType::VG == info.type) {
        opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Delete));
    } else if (EDataType::PARTATIONS == info.type) {
        if (ESpaceUsage::FREE == info.usage) {
            opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Add));
        } else {
            opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Edit));
            opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Delete));
        }
    }
    return opts;
}

static QMap<QString, QPixmap> getActionPixmap(const QStringList &opts)
{
   QMap<QString, QPixmap> icons = {};
   for (auto opt : opts) {
       if (!CommonFunc::getOpeartTypeText(EOpeartType::Edit).compare(opt)) {
           icons[CommonFunc::getOpeartTypeText(EOpeartType::Edit)] = CommonFunc::loadSvgImage(":/images/edit.svg", customPartationInstall::kActionIconWidth, customPartationInstall::kActionIconHeight);
       } else if (!CommonFunc::getOpeartTypeText(EOpeartType::Delete).compare(opt)) {
           icons[CommonFunc::getOpeartTypeText(EOpeartType::Delete)] = CommonFunc::loadSvgImage(":/images/partition_delete_press.svg", customPartationInstall::kActionIconWidth, customPartationInstall::kActionIconHeight);
       } else if (!CommonFunc::getOpeartTypeText(EOpeartType::Add).compare(opt)) {
           icons[CommonFunc::getOpeartTypeText(EOpeartType::Add)] = CommonFunc::loadSvgImage(":/images/partition_new_press.svg", customPartationInstall::kActionIconWidth, customPartationInstall::kActionIconHeight);
       }
   }
   return icons;
}

static QMap<EOpeartType, QRect> getItemActionRect(const QRect &itemRect, const QStringList &opts)
{
    QMap<EOpeartType, QRect> actions = {};
    QMap<QString, QPixmap> icons = getActionPixmap(opts);

    for (auto opt : icons.keys()) {
        QPixmap pixmap = icons.value(opt);
        if (!CommonFunc::getOpeartTypeText(EOpeartType::Edit).compare(opt)) {
            QRect rect(itemRect.right() - (CommonFunc::isBigScreen() ? customPartationInstall::kEditActionMargin : customPartationInstall::kEditActionMarginSmall) - pixmap.width(),
                           itemRect.top() + (itemRect.height() - pixmap.height()) / 2,
                           pixmap.width(), pixmap.height());
            actions[EOpeartType::Edit] = rect;

        } else {
            QRect rect(itemRect.right() - (CommonFunc::isBigScreen() ?
                           customPartationInstall::kEditActionMargin : customPartationInstall::kEditActionMarginSmall) + customPartationInstall::kActionSpace,
                           itemRect.top() + (itemRect.height() - pixmap.height()) / 2,
                           pixmap.width(), pixmap.height());
            if (!CommonFunc::getOpeartTypeText(EOpeartType::Delete).compare(opt)) {
                actions[EOpeartType::Delete] = rect;
            } else if (!CommonFunc::getOpeartTypeText(EOpeartType::Add).compare(opt)) {
                actions[EOpeartType::Add] = rect;
            }
        }
    }

    return actions;
}

DataDelegate::DataDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}

void DataDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem newoption = option;

    QItemDelegate::paint(painter, newoption, index);

    QFontMetrics fm(option.widget->font());

    QJsonObject jsonObj = index.data().toJsonObject();
    bool isDev = jsonObj.value(ISDEV).toBool();
    bool isVG = jsonObj.value(ISVG).toBool();
    bool isLV = jsonObj.value(ISLV).toBool();
    bool isFree= jsonObj.value(ISFREE).toBool();
    bool isMergeVG= jsonObj.value(ISMERGEVG).toBool();

    if (index.column() == 0) {
        newoption.rect.setLeft(customPartationInstall::kFirstColunmOffset);
        QString strDeviceName = jsonObj.value(ITEMDEVICENAME).toString();
        QRect rec = fm.boundingRect(strDeviceName);
        if (isDev || isVG) {
            newoption.rect.setLeft(customPartationInstall::kFirstColunmOffset - 10);
            double showSize = jsonObj.value(ITEMUSESIZE).toVariant().toDouble();
            int offset = !index.row() ? customPartationInstall::kFirstRowOffset : customPartationInstall::kFirstRowOffsetSmall;
            QString showUnit = CommonFunc::showSizeUnit(showSize);
            QString strDeviceSize = fm.elidedText(QString("%1%2").arg(QString::number(showSize), showUnit), Qt::ElideMiddle, customPartationInstall::kNoIconRowLen);
            painter->drawText(newoption.rect.x(), offset + newoption.rect.y(), strDeviceName);
            painter->setPen(QPen(QColor("#0081FF")));
            painter->drawText(newoption.rect.x() + rec.width() + customPartationInstall::kFirstColunmTextOffset, offset + newoption.rect.y(), strDeviceSize);
        } else {
            QPixmap pixIcon(CommonFunc::loadSvgImage(jsonObj.value(ITEMICON).toString(),  customPartationInstall::kIconWidth, customPartationInstall::kIconHeight));
            painter->drawPixmap(newoption.rect.x(), customPartationInstall::kIconVerticalOffset + newoption.rect.y(),  customPartationInstall::kIconWidth, customPartationInstall::kIconHeight, pixIcon);
            strDeviceName = fm.elidedText(strDeviceName, Qt::ElideMiddle, CommonFunc::isBigScreen() ?
                            (customPartationInstall::kColumnWidth_0 - customPartationInstall::kFirstColunmOffset - customPartationInstall::kIconWidth - customPartationInstall::kFirstColunmSpace * 3)
                                                                                                    : (customPartationInstall::kColumnWidthSmall_0 - customPartationInstall::kFirstColunmOffset - customPartationInstall::kIconWidth - customPartationInstall::kFirstColunmSpace * 3));
            painter->drawText(customPartationInstall::kFirstColunmOffset + customPartationInstall::kIconWidth + customPartationInstall::kFirstColunmSpace, customPartationInstall::kColunmVerticalOffset + newoption.rect.y(), strDeviceName);
        }
    }

    if (index.column() == 1) {
        newoption.rect.setLeft((CommonFunc::isBigScreen() ? customPartationInstall::kColumnWidth_0 : customPartationInstall::kColumnWidthSmall_0) + customPartationInstall::kColunmOffset);
        if (!isDev && !isVG) {
            double showSize = jsonObj.value(ITEMUSESIZE).toVariant().toDouble();
            QString showUnit = CommonFunc::showSizeUnit(showSize);
            QString strUseSize= fm.elidedText(QString("%1%2").arg(QString::number(showSize), showUnit), Qt::ElideMiddle,
                                              CommonFunc::isBigScreen() ?
                                                  (customPartationInstall::kColumnWidth_1 - customPartationInstall::kFirstColunmOffset)
                                                                                                                                                   : (customPartationInstall::kColumnWidthSmall_1 - customPartationInstall::kFirstColunmOffset));
            painter->drawText(newoption.rect.x(), customPartationInstall::kColunmVerticalOffset + newoption.rect.y(), strUseSize);
        }
    }

    if (index.column() == 2) {
        newoption.rect.setLeft((CommonFunc::isBigScreen() ? (customPartationInstall::kColumnWidth_0 + customPartationInstall::kColumnWidth_1)
                                                          : (customPartationInstall::kColumnWidthSmall_0 + customPartationInstall::kColumnWidthSmall_1)) + customPartationInstall::kColunmOffset);
        QString strFileSystem = fm.elidedText(jsonObj.value(ITEMFILESYSTEM).toString(), Qt::ElideMiddle,
                                              CommonFunc::isBigScreen() ?
                                                  (customPartationInstall::kColumnWidth_2 - customPartationInstall::kFirstColunmOffset)
                                                                                                                                   : (customPartationInstall::kColumnWidthSmall_2 - customPartationInstall::kFirstColunmOffset));
        painter->drawText(newoption.rect.x(), customPartationInstall::kColunmVerticalOffset + newoption.rect.y(), strFileSystem);
    }

    if (index.column() == 3) {
        newoption.rect.setLeft((CommonFunc::isBigScreen() ? (customPartationInstall::kColumnWidth_0 + customPartationInstall::kColumnWidth_1 + customPartationInstall::kColumnWidth_2)
                                                          : (customPartationInstall::kColumnWidthSmall_0 +customPartationInstall:: kColumnWidthSmall_1 + customPartationInstall::kColumnWidthSmall_2)) + customPartationInstall::kColunmOffset);
        QString strMountPoint = fm.elidedText(jsonObj.value(ITEMMOUNTPOINT).toString(), Qt::ElideMiddle,
                                              CommonFunc::isBigScreen() ?
                                                  (customPartationInstall::kColumnWidth_3 - customPartationInstall::kFirstColunmOffset)
                                                                                                                                   : (customPartationInstall::kColumnWidthSmall_3 - customPartationInstall::kFirstColunmOffset));
        painter->drawText(newoption.rect.x(), customPartationInstall::kColunmVerticalOffset + newoption.rect.y(), strMountPoint);
    }

    if (index.column() == 4) {
        newoption.rect.setLeft((CommonFunc::isBigScreen() ?
                                    (customPartationInstall::kColumnWidth_0 + customPartationInstall::kColumnWidth_1 + customPartationInstall::kColumnWidth_2 + customPartationInstall::kColumnWidth_3)
                                                          : (customPartationInstall::kColumnWidthSmall_0 + customPartationInstall::kColumnWidthSmall_1 + customPartationInstall::kColumnWidthSmall_2 + customPartationInstall::kColumnWidthSmall_3)) + customPartationInstall::kColunmOffset);
        painter->setPen(QPen(QColor("#FF7400")));
        QString strInstallTag= fm.elidedText(jsonObj.value(ITEMINSTALLTAG).toString(), Qt::ElideMiddle,
                                             CommonFunc::isBigScreen() ?
                                              (customPartationInstall::kColumnWidth_4 - customPartationInstall::kFirstColunmOffset)
                                                                                                                                  : (customPartationInstall::kColumnWidthSmall_4 - customPartationInstall::kFirstColunmOffset));
        int textOffset = CommonFunc::isBigScreen() ? customPartationInstall::kColunmOffset_4 : 0;
        painter->drawText(newoption.rect.x() + textOffset, customPartationInstall::kColunmVerticalOffset + newoption.rect.y(), strInstallTag);
    }

    if (index.column() == 5) {
        newoption.rect.setLeft((CommonFunc::isBigScreen() ?
                                    (customPartationInstall::kColumnWidth_0 + customPartationInstall::kColumnWidth_1 + customPartationInstall::kColumnWidth_2 + customPartationInstall::kColumnWidth_3 + customPartationInstall::kColumnWidth_4)
                                                          : (customPartationInstall::kColumnWidthSmall_0 + customPartationInstall::kColumnWidthSmall_1 + customPartationInstall::kColumnWidthSmall_2 + customPartationInstall::kColumnWidthSmall_3 + customPartationInstall::kColumnWidthSmall_4)) + customPartationInstall::kColunmOffset);
        if (!isDev && !isLV) {
            QStringList opts = {};
            if (isVG) {
                opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Delete));
            } else {
                if (isFree) {
                    opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Add));
                } else {
                    opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Edit));
                    opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Delete));
                }
            }

            QMap<EOpeartType, QRect> actions = getItemActionRect(newoption.rect, opts);
            QMap<QString, QPixmap> icons = getActionPixmap(opts);
            for (auto iAction : actions.keys()) {
                if (!isMergeVG) {
                    QMap<QString, EOpeartType> type = CommonFunc::opeartTypeToText(iAction);
                    QString strOpeart = type.key(iAction);
                    QRect opeartRect = actions.value(iAction);
                    if (isVG) {
                        opeartRect = QRect(opeartRect.x(), opeartRect.y() + customPartationInstall::kIconVerticalOffset, opeartRect.width(), opeartRect.height());
                    }
                    painter->drawPixmap(opeartRect, icons.value(strOpeart));
                }
            }
        }
    }

    painter->save();
    painter->restore();
}

QSize DataDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int icolumn= index.column();
    QJsonObject jsonObj = index.data().toJsonObject();
    bool isDev = jsonObj.value(ISDEV).toBool();
    bool isVG = jsonObj.value(ISVG).toBool();

    int iheight = ITEM_Height;
    if (isDev || isVG) {
        if (!index.row()) {
            return QSize(CommonFunc::isBigScreen() ? customPartationInstall::kColumnWidth_0 : customPartationInstall::kColumnWidthSmall_0, ITEM_Height - 10);
        }
        return QSize(CommonFunc::isBigScreen() ? customPartationInstall::kColumnWidth_0 : customPartationInstall::kColumnWidthSmall_0, ITEM_Height - 20);
    }

    if (0 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? customPartationInstall::kColumnWidth_0 : customPartationInstall::kColumnWidthSmall_0, iheight);
    } else if (1 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? customPartationInstall::kColumnWidth_1 : customPartationInstall::kColumnWidthSmall_1, iheight);
    } else if (2 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? customPartationInstall::kColumnWidth_2 : customPartationInstall::kColumnWidthSmall_2, iheight);
    } else if (3 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? customPartationInstall::kColumnWidth_3 : customPartationInstall::kColumnWidthSmall_3, iheight);
    } else if (4 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? customPartationInstall::kColumnWidth_4 : customPartationInstall::kColumnWidthSmall_4, iheight);
    } else if (5 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? customPartationInstall::kColumnWidth_5 : customPartationInstall::kColumnWidthSmall_5, iheight);
    }

    return option.rect.size();
}

DataModel::DataModel(QObject *parent)
    : QAbstractTableModel(parent)
{

}

void DataModel::deleteItem(const QString &devName)
{
    int index = 0;
    for (; index < m_dataInfo.count(); index++) {
        if (devName == m_dataInfo[index].id) {
            break;
        }
    }

    if (index >= m_dataInfo.count()) {
        return;
    }

    m_dataInfo.removeAt(index);
}

void DataModel::setAttributes(int index, const SDataInfo &dataInfo)
{
    if (index >= m_dataInfo.count()) {
        return;
    }

    m_dataInfo[index]= dataInfo;

    //其他情况只用刷新当前任务
    Q_EMIT dataChanged(this->index(index, 0), this->index(index, ITEMCOLUMN -1));
}

void DataModel::doItemAction(const SDataInfo &dataInfo, EOpeartType optType)
{
    SDeviceInfo dev = dataToDevice(dataInfo);
    SPartitionInfo partinfo = dataToPartition(dataInfo);
    SLVMInfo lvminfo = dataToLVM(dataInfo);
    SVGInfo vg = dataToVG(dataInfo);
    if (EOpeartType::Edit == optType && ESpaceUsage::FREE != dataInfo.usage) {
        if (EDataType::PARTATIONS == dataInfo.type && !dataInfo.isMergeVG) {
            Q_EMIT signalPartitionEdit(partinfo);
            return;
        }
    }

    if (EOpeartType::Delete == optType) {
        if (ESpaceUsage::FREE != dataInfo.usage) {
            if (EDataType::PARTATIONS == dataInfo.type && !dataInfo.isMergeVG) {
                Q_EMIT signalPartitionDelete(partinfo);
                return;
            }
        }

        if (EDataType::VG == dataInfo.type) {
            Q_EMIT signalVGDelete(vg);
            return;
        }
    }

    if (EOpeartType::Add == optType && ESpaceUsage::FREE == dataInfo.usage) {
        if (EDataType::PARTATIONS == dataInfo.type && !dataInfo.isMergeVG) {
            Q_EMIT signalPartitionAdd(partinfo);
            return;
        }
    }
}

QList<SDataInfo> DataModel::toSDataInfo(QMap<SDeviceInfo, QList<SPartitionInfo>> devs, QMap<SVGInfo, QList<SLVMInfo>> vgs)
{
    QList<SDataInfo> dataInfoList;
    QMap<SDeviceInfo, QList<SPartitionInfo>>::iterator deviceiter;
    for (deviceiter = devs.begin(); deviceiter != devs.end(); ++deviceiter) {
        SDataInfo devData;
        devData.name = deviceiter.key().name;
        devData.device = deviceiter.key().device;
        devData.size = deviceiter.key().size;
        devData.fileSystem = deviceiter.key().filesystem;
        devData.mountPoint = deviceiter.key().mountPoint;
        devData.type = EDataType::DEVICE;
        dataInfoList.append(devData);
        for (SPartitionInfo partinfo : deviceiter.value()) {
            if (partinfo.size <= 0) {
                continue;
            }
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
            partData.isNeedFormat = partinfo.isNeedFormat;
            if (ESpaceUsage::FREE == partinfo.usage || EPartitionType::PartitionTypeExtension != partinfo.partitionType) {
                dataInfoList.append(partData);
            }
        }
    }

    QMap<SVGInfo, QList<SLVMInfo>>::iterator lvmiter;
    for (lvmiter = vgs.begin(); lvmiter != vgs.end(); ++lvmiter) {
        SDataInfo vgData;
        vgData.name = lvmiter.key().name;
        vgData.device = lvmiter.key().name;
        vgData.size = lvmiter.key().size;
        vgData.id = lvmiter.key().id;
        vgData.type = EDataType::VG;
        dataInfoList.append(vgData);
        for (SLVMInfo lvminfo : lvmiter.value()) {
            SDataInfo partData;
            partData.id = lvminfo.id;
            partData.name = lvminfo.label;
            partData.size = lvminfo.size;
            partData.fileSystem = lvminfo.filesystem;
            partData.mountPoint = lvminfo.mountPoint;
            partData.path = lvminfo.path;
            partData.type = EDataType::LV;
            partData.device = lvminfo.vgName;
            partData.usage = lvminfo.usage;
            partData.isNeedFormat = lvminfo.isNeedFormat;
            dataInfoList.append(partData);
        }
    }
    return dataInfoList;
}

SDeviceInfo DataModel::dataToDevice(const SDataInfo &dataInfo)
{
    SDeviceInfo reset;
    reset.device = dataInfo.device;

    return reset;
}

SPartitionInfo DataModel::dataToPartition(const SDataInfo &dataInfo)
{
    SPartitionInfo reset;
    reset.id = dataInfo.id;
    reset.size = dataInfo.size;
    reset.filesystem = dataInfo.fileSystem;
    reset.usage = dataInfo.usage;
    reset.mountPoint = dataInfo.mountPoint;
    reset.device = dataInfo.device;
    reset.path = dataInfo.path;
    reset.label = dataInfo.label;
    reset.partitionType = dataInfo.partitionType;
    reset.isMergeVG = dataInfo.isMergeVG;
    reset.isNeedFormat = dataInfo.isNeedFormat;

    return reset;
}

SLVMInfo DataModel::dataToLVM(const SDataInfo &dataInfo)
{
    SLVMInfo reset;
    reset.id = dataInfo.id;
    reset.label = dataInfo.name;
    reset.size = dataInfo.size;
    reset.filesystem = dataInfo.fileSystem;
    reset.mountPoint = dataInfo.mountPoint;
    reset.path = dataInfo.path;
    reset.vgName = dataInfo.device;
    reset.isNeedFormat = dataInfo.isNeedFormat;

    return reset;
}

SVGInfo DataModel::dataToVG(const SDataInfo &dataInfo)
{
    SVGInfo reset;
    reset.id = dataInfo.id;
    reset.name = dataInfo.device;

    return reset;
}

void DataModel::setDatas(QMap<SDeviceInfo, QList<SPartitionInfo>> devs, QMap<SVGInfo, QList<SLVMInfo>> vgs)
{
    m_dataInfo.clear();
    beginResetModel();
    m_dataInfo = toSDataInfo(devs, vgs);
    endResetModel();
    Q_EMIT signalItemCountChanged(m_dataInfo.count());
}

int DataModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_dataInfo.count();
}

int DataModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ITEMCOLUMN;
}

QVariant DataModel::data(const QModelIndex &index, int role) const
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
    bool isDev = EDataType::DEVICE == dataInfo.type  ?  true : false;
    bool isVG = EDataType::VG == dataInfo.type ? true : false;
    bool isLV= EDataType::LV == dataInfo.type ?  true : false;
    bool isFree= ESpaceUsage::FREE == dataInfo.usage ? true : false;
    bool isMergeVG= dataInfo.isMergeVG ? true : false;
    jsonObj.insert(ISVG, isVG);
    jsonObj.insert(ISDEV, isDev);
    jsonObj.insert(ISLV, isLV);
    jsonObj.insert(ISFREE, isFree);
    jsonObj.insert(ISMERGEVG, isMergeVG);
    if (icol == 0) {
        if (isDev || isVG) {
            jsonObj.insert(ITEMDEVICENAME, QString("%1").arg(dataInfo.name));
            jsonObj.insert(ITEMUSESIZE, QString("%1").arg(dataInfo.size));
        } else {
            QString strDeviceName;
            QString iconPath = ":/images/drive-harddisk-32px.svg";
            if (EPartitionType::PartitionTypePrimary == dataInfo.partitionType) {
                iconPath = ":/images/P.svg";
            } else if (EPartitionType::PartitionTypeLogical == dataInfo.partitionType) {
                iconPath = ":/images/L.svg";
            }

            if (isMergeVG) {
                iconPath = ":/images/PV.svg";
            }

            if (isLV) {
                iconPath = ":/images/LV.svg";
            }

            if (!isFree) {
                if (dataInfo.path.isEmpty()) {
                    strDeviceName = QString("%1").arg(dataInfo.label);
                } else {
                    strDeviceName = QString("%1(%2)").arg(dataInfo.label, dataInfo.path);
                }

                if (isLV) {
                    strDeviceName = QString("%1").arg(dataInfo.name);
                }
            } else {
                strDeviceName = ::QObject::tr("free");
            }

            jsonObj.insert(ITEMICON, iconPath);
            jsonObj.insert(ITEMDEVICENAME, strDeviceName);
        }
        return jsonObj;
    } else if (icol == 1) {
        jsonObj.insert(ITEMUSESIZE, QString("%1").arg(dataInfo.size));
        return jsonObj;
    } else if (icol == 2) {
        if (!isFree) {
            jsonObj.insert(ITEMFILESYSTEM, dataInfo.fileSystem);
        }
        return jsonObj;
    } else if (icol == 3) {
        if (!isFree) {
            jsonObj.insert(ITEMMOUNTPOINT, dataInfo.mountPoint);
        }
        return jsonObj;
    } else if (icol == 4) {
        if (!isFree && !isDev && !isVG && !dataInfo.mountPoint.compare("/")) {
            jsonObj.insert(ITEMINSTALLTAG, ::QObject::tr("Install here"));
        } else if (!isFree && !isDev && !isVG && dataInfo.isNeedFormat) {
            jsonObj.insert(ITEMINSTALLTAG, ::QObject::tr("To be formatted"));
        }
        return jsonObj;
    } else if (icol == 5) {
        return jsonObj;
    }

    return QVariant();
}

SDataInfo DataModel::getDataInfos(int iRow)
{
    if (iRow >= m_dataInfo.count()) {
        return SDataInfo();
    }

    return m_dataInfo[iRow];
}

CustomPartationListView::CustomPartationListView(QWidget *parent)
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

    m_itemDelegate = new DataDelegate();
    setItemDelegate(m_itemDelegate);

    m_actionNames << CommonFunc::getOpeartTypeText(EOpeartType::Edit);
    m_actionNames << CommonFunc::getOpeartTypeText(EOpeartType::Delete);
    m_actionNames << CommonFunc::getOpeartTypeText(EOpeartType::Add);

}

void CustomPartationListView::drawRow(QPainter *painter, const QStyleOptionViewItem &options, const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    DPalette::ColorGroup cg = DPalette::Normal;

    auto *style = dynamic_cast<DStyle *>(DApplication::style());

    auto radius = style->pixelMetric(DStyle::PM_FrameRadius, &options);

    auto palette = options.palette;
    DataModel *devModel = static_cast<DataModel *>(model());
    SDataInfo dataInfo = devModel->getDataInfos(index.row());

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
                DGuiApplicationHelper::ColorType themEDataType = DGuiApplicationHelper::instance()->themeType();

                if (themEDataType == DGuiApplicationHelper::LightType) {
                    background = style->adjustColor(baseColor, 0, 0, -10);
                }
                else if (themEDataType == DGuiApplicationHelper::DarkType) {
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

    QPainterPath painterPath;

    if (EDataType::DEVICE != dataInfo.type && EDataType::VG != dataInfo.type) {
        path.addRoundedRect(rowRect, radius, radius);
        painter->fillPath(path, background);
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

bool CustomPartationListView::viewportEvent(QEvent *event)
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

void CustomPartationListView::paintEvent(QPaintEvent *event)
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

void CustomPartationListView::mouseMoveEvent(QMouseEvent *event)
{
    BaseTableView::mouseMoveEvent(event);
    QModelIndex index = indexAt(event->pos());
    if (index.row() < 0){
        return;
    }

    DataModel *dataModel = static_cast<DataModel *>(model());
    QFontMetrics fm(this->font());;
    SDataInfo dataInfo = dataModel->getDataInfos(index.row());

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
        if (fm.horizontalAdvance(strName) > (CommonFunc::isBigScreen() ? (customPartationInstall::kColumnWidth_0 - customPartationInstall::kFirstColunmOffset - customPartationInstall::kIconWidth - customPartationInstall::kFirstColunmSpace * 3)
                                                           : (customPartationInstall::kColumnWidthSmall_0 - customPartationInstall::kFirstColunmOffset - customPartationInstall::kIconWidth - customPartationInstall::kFirstColunmSpace * 3))) {
#else
        if (fm.width(strName) > (CommonFunc::isBigScreen() ? (customPartationInstall::kColumnWidth_0 - customPartationInstall::kFirstColunmOffset - customPartationInstall::kIconWidth - customPartationInstall::kFirstColunmSpace * 3)
                                                           : (customPartationInstall::kColumnWidthSmall_0 - customPartationInstall::kFirstColunmOffset - customPartationInstall::kIconWidth - customPartationInstall::kFirstColunmSpace * 3))) {
#endif
            QToolTip::showText(QCursor::pos(), strName, this, this->rect(), customPartationInstall::kTooltipTime);
        }
    }

    if (index.column() == 1) {
        double showSize = dataInfo.size;
        QString showUnit = CommonFunc::showSizeUnit(showSize);
        QString strSize = QString("%1%2").arg(QString::number(showSize), showUnit);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        if (fm.horizontalAdvance(strSize) > (CommonFunc::isBigScreen() ? (customPartationInstall::kColumnWidth_1 - customPartationInstall::kFirstColunmOffset)
                                                           : (customPartationInstall::kColumnWidthSmall_1 - customPartationInstall::kFirstColunmOffset))) {
#else
        if (fm.width(strSize) > (CommonFunc::isBigScreen() ? (customPartationInstall::kColumnWidth_1 - customPartationInstall::kFirstColunmOffset)
                                                           : (customPartationInstall::kColumnWidthSmall_1 - customPartationInstall::kFirstColunmOffset))) {
#endif
            QToolTip::showText(QCursor::pos(), strSize, this, this->rect(), customPartationInstall::kTooltipTime);
        }
    }

    if (index.column() == 2) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        if (fm.horizontalAdvance(dataInfo.fileSystem) > (CommonFunc::isBigScreen() ? (customPartationInstall::kColumnWidth_2 - customPartationInstall::kFirstColunmOffset)
                                                                       : (customPartationInstall::kColumnWidthSmall_2 - customPartationInstall::kFirstColunmOffset))) {
#else
        if (fm.width(dataInfo.fileSystem) > (CommonFunc::isBigScreen() ? (customPartationInstall::kColumnWidth_2 - customPartationInstall::kFirstColunmOffset)
                                                                       : (customPartationInstall::kColumnWidthSmall_2 - customPartationInstall::kFirstColunmOffset))) {
#endif
            QToolTip::showText(QCursor::pos(), dataInfo.fileSystem, this, this->rect(), customPartationInstall::kTooltipTime);
        }
    }

    if (index.column() == 3) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        if (fm.horizontalAdvance(dataInfo.mountPoint) > (CommonFunc::isBigScreen() ? (customPartationInstall::kColumnWidth_3 - customPartationInstall::kFirstColunmOffset)
                                                                       : (customPartationInstall::kColumnWidthSmall_3 - customPartationInstall::kFirstColunmOffset))) {
#else
        if (fm.width(dataInfo.mountPoint) > (CommonFunc::isBigScreen() ? (customPartationInstall::kColumnWidth_3 - customPartationInstall::kFirstColunmOffset)
                                                                       : (customPartationInstall::kColumnWidthSmall_3 - customPartationInstall::kFirstColunmOffset))) {
#endif
            QToolTip::showText(QCursor::pos(), dataInfo.mountPoint, this, this->rect(), customPartationInstall::kTooltipTime);
        }
    }

    QRect rect = visualRect(index);
    if (EDataType::DEVICE != dataInfo.type) {
        QStringList opts = getOpearts(dataInfo);
        QMap<EOpeartType, QRect>  actionRects = getItemActionRect(rect, opts);

        //查找是否有按钮处于hover状态
        for (auto iAction: actionRects.keys()) {
            if (actionRects.value(iAction).contains(event->pos()) && index.column() == (ITEMCOLUMN -1)) {
                if ((EDataType::PARTATIONS == dataInfo.type || EDataType::LV == dataInfo.type || EDataType::VG == dataInfo.type) && !dataInfo.isMergeVG) {
                    setCursor(Qt::PointingHandCursor);
                    QToolTip::showText(mapToGlobal(QPoint(actionRects.value(iAction).topRight())), CommonFunc::opeartTypeToText(iAction).key(iAction), this, actionRects.value(iAction), customPartationInstall::kTooltipTime);
                } else {
                    setCursor(Qt::ArrowCursor);
                }
                return;
            } else {
                setCursor(Qt::ArrowCursor);
            }
        }
    }
}

void CustomPartationListView::mouseReleaseEvent(QMouseEvent *event)
{
    BaseTableView::mouseReleaseEvent(event);
    QModelIndex index = indexAt(event->pos());
    if (index.row() < 0){
        return;
    }
    DataModel *dataModel = static_cast<DataModel *>(model());
    SDataInfo dataInfo = dataModel->getDataInfos(index.row());

    if (index.column() != ITEMCOLUMN -1) {
        return;
    }

    QRect rect = visualRect(index);
    if (EDataType::DEVICE != dataInfo.type) {
        QStringList opts = getOpearts(dataInfo);
        QMap<EOpeartType, QRect>  actionRects = getItemActionRect(rect, opts);

        //查找是否有按钮处于hover状态
        for (auto iAction: actionRects.keys()) {
            if (actionRects.value(iAction).contains(event->pos())) {
                dataModel->doItemAction(dataInfo, iAction);
                return;
            }
        }
    }
}
