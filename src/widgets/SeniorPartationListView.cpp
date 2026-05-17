#include "SeniorPartationListView.h"
#include "CommonFunc.h"
#include "BaseHeaderView.h"

#include <DLabel>
#include <DApplication>
#include <DGuiApplicationHelper>
#include <DPaletteHelper>
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
#define ISDEV  "isdev"
#define ISVG  "isvg"
#define ISFREE  "isfree"
#define ISMERGEVG "ismergevg"
#define ISLV  "islv"
#define ISNEEDFORMAT  "isneedformat"
#define ISCANENCRYPT "canencrypt"
#define ISENCRYPT "isencrypted"

//item高度
#define ITEM_Height 60

//操作种数
#define  OPERATECOUNT 2

static QStringList getOpearts(const SDataInfo &info)
{
    QStringList opts = {};
    if (EDataType::VG == info.type) {
        opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Delete));
    } else {
        if (ESpaceUsage::FREE == info.usage) {
            opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Add));
        } else {
            opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Edit));
            opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Delete));
            if(info.isCanEncrypt) {
                //屏蔽指定分区加密按钮
                //opts.append(CommonFunc::getOpeartTypeText(info.isEncrypt ? EOpeartType::Decrypt : EOpeartType::Encrypt));
            }
        }
    }
    return opts;
}

static QMap<QString, QPixmap> getActionPixmap(const QStringList &opts)
{
   QMap<QString, QPixmap> icons = {};
   for (auto opt : opts) {
       if (!CommonFunc::getOpeartTypeText(EOpeartType::Edit).compare(opt)) {
           icons[CommonFunc::getOpeartTypeText(EOpeartType::Edit)] = QPixmap(":/images/edit.svg");
       } else if (!CommonFunc::getOpeartTypeText(EOpeartType::Delete).compare(opt)) {
           icons[CommonFunc::getOpeartTypeText(EOpeartType::Delete)] = QPixmap(":/images/partition_delete_press.svg");
       } else if (!CommonFunc::getOpeartTypeText(EOpeartType::Add).compare(opt)) {
           icons[CommonFunc::getOpeartTypeText(EOpeartType::Add)] = QPixmap(":/images/partition_new_press.svg");
       } else if (!CommonFunc::getOpeartTypeText(EOpeartType::Encrypt).compare(opt)) {
           icons[CommonFunc::getOpeartTypeText(EOpeartType::Encrypt)] = QPixmap(":/images/partition_encrypt.svg");
       } else if (!CommonFunc::getOpeartTypeText(EOpeartType::Decrypt).compare(opt)) {
           icons[CommonFunc::getOpeartTypeText(EOpeartType::Decrypt)] = QPixmap(":/images/partition_decrypt.svg");
       }
   }
   return icons;
}

static QMap<EOpeartType, QRect> getItemActionRect(const QRect &itemRect, const QStringList &opts)
{
    QMap<EOpeartType, QRect> actions = {};
    auto && opMap = CommonFunc::opeartTypeToText();
    QMap<QString, QPixmap> icons = getActionPixmap(opts);
    int iconSpace = CommonFunc::isBigScreen() ? seniorPartationInstall::kActionSpace : seniorPartationInstall::kColunmOffset;
    int startX = itemRect.right() - (CommonFunc::isBigScreen() ? seniorPartationInstall::kEditActionMargin : seniorPartationInstall::kEditActionMarginSmall);
    for (auto opt : icons.keys()) {
        QRect iconRect;
        QPixmap pixmap = icons.value(opt);
        if (!CommonFunc::getOpeartTypeText(EOpeartType::Edit).compare(opt)) {
            iconRect = QRect(startX - pixmap.width(),
                             itemRect.top() + (itemRect.height() - pixmap.height()) / 2,
                             pixmap.width(), pixmap.height());
        } else if(opt == CommonFunc::getOpeartTypeText(EOpeartType::Add) ||
                  opt == CommonFunc::getOpeartTypeText(EOpeartType::Delete)){
            iconRect = QRect(startX + iconSpace,
                             itemRect.top() + (itemRect.height() - pixmap.height()) / 2,
                             pixmap.width(), pixmap.height());
        } else if (opt == CommonFunc::getOpeartTypeText(EOpeartType::Encrypt) ||
                   opt == CommonFunc::getOpeartTypeText(EOpeartType::Decrypt)) {
            iconRect = QRect(startX + 2 * iconSpace + pixmap.width(),
                           itemRect.top() + (itemRect.height() - pixmap.height()) / 2,
                           pixmap.width(), pixmap.height());
        }

        actions[opMap[opt]] = iconRect;
    }

    return actions;
}

ItemDelegate::ItemDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem newoption = option;

    QItemDelegate::paint(painter, newoption, index);

    QFontMetrics fm(option.widget->font());

    QJsonObject jsonObj = index.data().toJsonObject();
    bool isDev = jsonObj.value(ISDEV).toBool();
    bool isVG = jsonObj.value(ISVG).toBool();
    bool isFree= jsonObj.value(ISFREE).toBool();
    bool isMergeVG= jsonObj.value(ISMERGEVG).toBool();
    bool isCrypted = jsonObj.value(ISENCRYPT).toBool();
    bool canEncrypt = jsonObj.value(ISCANENCRYPT).toBool();
    if (index.column() == 0) {
        newoption.rect.setLeft(seniorPartationInstall::kFirstColunmOffset);
        QString strDeviceName = jsonObj.value(ITEMDEVICENAME).toString();
        QRect rec = fm.boundingRect(strDeviceName);
        if (isDev || isVG) {
            newoption.rect.setLeft(seniorPartationInstall::kFirstColunmOffset + 5);
            double size = jsonObj.value(ITEMUSESIZE).toVariant().toLongLong();
            int offset = !index.row() ? seniorPartationInstall::kFirstRowOffset : seniorPartationInstall::kFirstRowOffsetSmall;
            QString sizeUnit = CommonFunc::showSizeUnit(size);
            QString strDeviceSize = fm.elidedText(QString("%1%2").arg(QString::number(size), sizeUnit), Qt::ElideMiddle, seniorPartationInstall::kColumnWidth_0);
            painter->drawText(newoption.rect.x(), offset + newoption.rect.y(), strDeviceName);
            painter->drawText(newoption.rect.x() + rec.width() + seniorPartationInstall::kFirstColunmTextOffset,
                              offset + newoption.rect.y(), strDeviceSize);
        } else {
            QPixmap pixIcon(CommonFunc::loadSvgImage(jsonObj.value(ITEMICON).toString(), seniorPartationInstall::kIconWidth, seniorPartationInstall::kIconHeight));
            painter->drawPixmap(newoption.rect.x(), seniorPartationInstall::kIconVerticalOffset + newoption.rect.y(), seniorPartationInstall::kIconWidth, seniorPartationInstall::kIconHeight, pixIcon);
            strDeviceName = fm.elidedText(strDeviceName, Qt::ElideMiddle, CommonFunc::isBigScreen() ?
                                              (seniorPartationInstall::kColumnWidth_0 - seniorPartationInstall::kFirstColunmOffset - seniorPartationInstall::kIconWidth - seniorPartationInstall::kFirstColunmSpace * 3)
                                                                                                    : (seniorPartationInstall::kColumnWidthSmall_0 - seniorPartationInstall::kFirstColunmOffset - seniorPartationInstall::kIconWidth - seniorPartationInstall::kFirstColunmSpace * 3));
            painter->drawText(seniorPartationInstall::kFirstColunmOffset + seniorPartationInstall::kIconWidth + seniorPartationInstall::kFirstColunmSpace, seniorPartationInstall::kColunmVerticalOffset + newoption.rect.y(), strDeviceName);
        }
    }

    if (index.column() == 1) {
        newoption.rect.setLeft((CommonFunc::isBigScreen() ? seniorPartationInstall::kColumnWidth_0 : seniorPartationInstall::kColumnWidthSmall_0) + seniorPartationInstall::kColunmOffset);
        QString strFileSystem = fm.elidedText(jsonObj.value(ITEMFILESYSTEM).toString(), Qt::ElideMiddle,
                                              CommonFunc::isBigScreen() ?
                                                  (seniorPartationInstall::kColumnWidth_1 - seniorPartationInstall::kFirstColunmOffset)
                                                                                                                                   : (seniorPartationInstall::kColumnWidthSmall_1 - seniorPartationInstall::kFirstColunmOffset));
        painter->drawText(newoption.rect.x(), seniorPartationInstall::kColunmVerticalOffset + newoption.rect.y(), strFileSystem);
    }

    if (index.column() == 2) {
        newoption.rect.setLeft((CommonFunc::isBigScreen() ? (seniorPartationInstall::kColumnWidth_0 + seniorPartationInstall::kColumnWidth_1)
                                                          : (seniorPartationInstall::kColumnWidthSmall_0 + seniorPartationInstall::kColumnWidthSmall_1)) + seniorPartationInstall::kColunmOffset);
        QString strMountPoint = jsonObj.value(ITEMMOUNTPOINT).toString();
        int tipsWidth = 0;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        if (!strMountPoint.compare("/")) {
            tipsWidth = fm.horizontalAdvance(::QObject::tr("Install here"));
        } else if (jsonObj.value(ISNEEDFORMAT).toBool()) {
            tipsWidth = fm.horizontalAdvance(::QObject::tr("To be formatted"));
        }
#else
        if (!strMountPoint.compare("/")) {
            tipsWidth = fm.width(::QObject::tr("Install here"));
        } else if (jsonObj.value(ISNEEDFORMAT).toBool()) {
            tipsWidth = fm.width(::QObject::tr("To be formatted"));
        }
#endif

        strMountPoint = fm.elidedText(jsonObj.value(ITEMMOUNTPOINT).toString(), Qt::ElideMiddle, (CommonFunc::isBigScreen() ?
                                                  (seniorPartationInstall::kColumnWidth_2 - seniorPartationInstall::kFirstColunmOffset)
                                                                                                                            : (seniorPartationInstall::kColumnWidthSmall_2 - seniorPartationInstall::kFirstColunmOffset)) - tipsWidth);
        painter->drawText(newoption.rect.x(), seniorPartationInstall::kColunmVerticalOffset + newoption.rect.y(), strMountPoint);
        if (!strMountPoint.compare("/")) {
            painter->setPen(QPen(QColor("#FF7400")));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            painter->drawText(newoption.rect.x() + fm.horizontalAdvance(strMountPoint) + 10, seniorPartationInstall::kColunmVerticalOffset + newoption.rect.y(), ::QObject::tr("Install here"));
#else
            painter->drawText(newoption.rect.x() + fm.width(strMountPoint) + 10, seniorPartationInstall::kColunmVerticalOffset + newoption.rect.y(), ::QObject::tr("Install here"));
#endif
        } else if (jsonObj.value(ISNEEDFORMAT).toBool()) {
            painter->setPen(QPen(QColor("#FF7400")));
            if (strMountPoint.isEmpty()) {
                painter->drawText(newoption.rect.x(), seniorPartationInstall::kColunmVerticalOffset + newoption.rect.y(), ::QObject::tr("To be formatted"));
            } else {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                painter->drawText(newoption.rect.x() + fm.horizontalAdvance(strMountPoint) + 10, seniorPartationInstall::kColunmVerticalOffset + newoption.rect.y(), ::QObject::tr("To be formatted"));
#else
                painter->drawText(newoption.rect.x() + fm.width(strMountPoint) + 10, seniorPartationInstall::kColunmVerticalOffset + newoption.rect.y(), ::QObject::tr("To be formatted"));
#endif
            }
        }
    }

    if (index.column() == 3) {
        newoption.rect.setLeft((CommonFunc::isBigScreen() ? (seniorPartationInstall::kColumnWidth_0 + seniorPartationInstall::kColumnWidth_1 + seniorPartationInstall::kColumnWidth_2)
                                                          : (seniorPartationInstall::kColumnWidthSmall_0 +seniorPartationInstall:: kColumnWidthSmall_1 + seniorPartationInstall::kColumnWidthSmall_2)) + seniorPartationInstall::kColunmOffset);
        if (!isDev && !isVG) {
            double size = jsonObj.value(ITEMUSESIZE).toVariant().toLongLong();
            QString sizeUnit = CommonFunc::showSizeUnit(size);
            QString strUseSize= fm.elidedText(QString("%1%2").arg(QString::number(size), sizeUnit), Qt::ElideMiddle,
                                              CommonFunc::isBigScreen() ?
                                                  (seniorPartationInstall::kColumnWidth_3 - seniorPartationInstall::kFirstColunmOffset)
                                                                                                                                               : (seniorPartationInstall::kColumnWidthSmall_3 - seniorPartationInstall::kFirstColunmOffset));

            DPalette palette = DPaletteHelper::instance()->palette(option.widget);
            painter->setPen(QPen(palette.color(DPalette::TextTips)));
            painter->drawText(newoption.rect.x(), seniorPartationInstall::kColunmVerticalOffset + newoption.rect.y(), strUseSize);
        }
    }

    if (index.column() == 4) {
        newoption.rect.setLeft((CommonFunc::isBigScreen() ? (seniorPartationInstall::kColumnWidth_0 + seniorPartationInstall::kColumnWidth_1 + seniorPartationInstall::kColumnWidth_2 + seniorPartationInstall::kColumnWidth_3)
                                                          : (seniorPartationInstall::kColumnWidthSmall_0 + seniorPartationInstall::kColumnWidthSmall_1 + seniorPartationInstall::kColumnWidthSmall_2 + seniorPartationInstall::kColumnWidthSmall_3)) + seniorPartationInstall::kColunmOffset);
        if (!isDev) {
            QStringList opts = {};
            if (isVG) {
                opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Delete));
            } else {
                if (isFree) {
                    opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Add));
                } else {
                    opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Edit));
                    opts.append(CommonFunc::getOpeartTypeText(EOpeartType::Delete));
                    if(canEncrypt) {
                        Q_UNUSED(isCrypted)
                        //屏蔽指定分区加密按钮
                        //opts.append(CommonFunc::getOpeartTypeText(isCrypted ? EOpeartType::Decrypt : EOpeartType::Encrypt));
                    }
                }
            }

            QMap<EOpeartType, QRect> actions = getItemActionRect(newoption.rect, opts);
            QMap<QString, QPixmap> icons = getActionPixmap(opts);
            if(!isMergeVG) {
                for (auto iAction : actions.keys()) {
                    QMap<QString, EOpeartType> type = CommonFunc::opeartTypeToText(iAction);
                    QString strOpeart = type.key(iAction);
                    QRect opeartRect = actions.value(iAction);
                    if (isVG) {
                        opeartRect = QRect(opeartRect.x(), opeartRect.y() + seniorPartationInstall::kIconVerticalOffset, opeartRect.width(), opeartRect.height());
                    }
                    painter->drawPixmap(opeartRect, icons.value(strOpeart));
                }
            } else {
                painter->setPen(QPen(QColor("#FF7400")));
                QRect textRect = newoption.rect;
                textRect.adjust(-10, 0, -20, 0);
                QTextOption textOption;
                textOption.setWrapMode(QTextOption::WrapAnywhere);
                textOption.setAlignment(Qt::AlignVCenter);
                painter->drawText(textRect, tr("The physical volume has been used"), textOption);
            }
        }
    }

    painter->save();
    painter->restore();
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int icolumn= index.column();
    QJsonObject jsonObj = index.data().toJsonObject();
    bool isDev = jsonObj.value(ISDEV).toBool();
    bool isVG = jsonObj.value(ISVG).toBool();

    int iheight = ITEM_Height;
    if (isDev || isVG) {
        if (!index.row()) {
            return QSize(CommonFunc::isBigScreen() ? seniorPartationInstall::kColumnWidth_0 : seniorPartationInstall::kColumnWidthSmall_0, ITEM_Height - 10);
        }
        return QSize(CommonFunc::isBigScreen() ? seniorPartationInstall::kColumnWidth_0 : seniorPartationInstall::kColumnWidthSmall_0, ITEM_Height - 20);
    }

    if (0 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? seniorPartationInstall::kColumnWidth_0 : seniorPartationInstall::kColumnWidthSmall_0, iheight);
    } else if (1 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? seniorPartationInstall::kColumnWidth_1 : seniorPartationInstall::kColumnWidthSmall_1, iheight);
    } else if (2 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? seniorPartationInstall::kColumnWidth_2 : seniorPartationInstall::kColumnWidthSmall_2, iheight);
    } else if (3 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? seniorPartationInstall::kColumnWidth_3 : seniorPartationInstall::kColumnWidthSmall_3, iheight);
    } else if (4 == icolumn) {
        return QSize(CommonFunc::isBigScreen() ? seniorPartationInstall::kColumnWidth_4 : seniorPartationInstall::kColumnWidthSmall_4, iheight);
    }

    return option.rect.size();
}

ItemModel::ItemModel(QObject *parent)
    : QAbstractTableModel(parent)
{

}

void ItemModel::deleteItem(const QString &devName)
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

void ItemModel::setAttributes(int index, const SDataInfo &dataInfo)
{
    if (index >= m_dataInfo.count()) {
        return;
    }

    m_dataInfo[index]= dataInfo;

    //其他情况只用刷新当前任务
    Q_EMIT dataChanged(this->index(index, 0), this->index(index, ITEMCOLUMN -1));
}

void ItemModel::updateDataInfo(int iRow, const SDataInfo &dataInfo)
{
    if(iRow < m_dataInfo.count()) {
        m_dataInfo[iRow] = dataInfo;
    }
}

void ItemModel::doItemAction(const SDataInfo &dataInfo, EOpeartType optType)
{
    SPartitionInfo partinfo = dataToPartition(dataInfo);
    SLVMInfo lvminfo = dataToLVM(dataInfo);
    SVGInfo vg = dataToVG(dataInfo);

    if (EOpeartType::Edit == optType && ESpaceUsage::FREE != dataInfo.usage) {
        if (EDataType::PARTATIONS == dataInfo.type && !dataInfo.isMergeVG) {
            Q_EMIT signalPartitionEdit(partinfo);
            return;
        }

        if (EDataType::LV == dataInfo.type) {
            Q_EMIT signalLVMEdit(lvminfo);
            return;
        }
    }

    if (EOpeartType::Delete == optType ) {
        if (ESpaceUsage::FREE != dataInfo.usage) {
            if (EDataType::PARTATIONS == dataInfo.type && !dataInfo.isMergeVG) {
                Q_EMIT signalPartitionDelete(partinfo);
                return;
            }

            if (EDataType::LV == dataInfo.type) {
                Q_EMIT signalLVMDelete(lvminfo);
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

        if (EDataType::LV == dataInfo.type) {
            Q_EMIT signalLVMAdd(lvminfo);
            return;
        }
    }

    if(EOpeartType::Encrypt == optType && ESpaceUsage::FREE != dataInfo.usage) {
        if(EDataType::PARTATIONS == dataInfo.type && !dataInfo.isMergeVG) {
            Q_EMIT signalPartitionEncrypt(partinfo.id);
            return;
        }

        if(EDataType::LV == dataInfo.type) {
            Q_EMIT signalPartitionEncrypt(lvminfo.id);
            return;
        }
    }

    if(EOpeartType::Decrypt == optType && ESpaceUsage::FREE != dataInfo.usage) {
        if(EDataType::PARTATIONS == dataInfo.type && !dataInfo.isMergeVG) {
            Q_EMIT signalPartitionDecrypt(partinfo.id, partinfo.mountPoint);
            return;
        }

        if(EDataType::LV == dataInfo.type) {
            Q_EMIT signalPartitionDecrypt(lvminfo.id, lvminfo.mountPoint);
        }
    }
}

QVariant ItemModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QList<SDataInfo> ItemModel::toSDataInfo(QMap<SDeviceInfo, QList<SPartitionInfo>> devs, QMap<SVGInfo, QList<SLVMInfo>> vgs)
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

            // 隐藏加密头分区的显示
            if (partinfo.isCryptHeader) {
#ifndef QT_DEBUG
                continue;
#endif
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
            partData.isCanEncrypt = partinfo.isCanCrypt;
            partData.isEncrypt = !partinfo.cryptPassword.isEmpty();
            if (ESpaceUsage::FREE == partinfo.usage || EPartitionType::PartitionTypeExtension != partinfo.partitionType) {
                dataInfoList.append(partData);
            }
        }
    }

    QMap<SVGInfo, QList<SLVMInfo>>::iterator lvmiter;
    for (lvmiter = vgs.begin(); lvmiter != vgs.end(); ++lvmiter) {

        // 隐藏加密头vg的显示
        if (lvmiter.key().isCryptHeader) {
#ifndef QT_DEBUG
            continue;
#endif
        }

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
            partData.isCanEncrypt = lvminfo.isCanCrypt;
            partData.isEncrypt = !lvminfo.cryptPassword.isEmpty();
            dataInfoList.append(partData);
        }
    }
    return dataInfoList;
}

SDeviceInfo ItemModel::dataToDevice(const SDataInfo &dataInfo)
{
    SDeviceInfo reset;
    reset.device = dataInfo.device;

    return reset;
}

SPartitionInfo ItemModel::dataToPartition(const SDataInfo &dataInfo)
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
    reset.isNeedFormat = dataInfo.isNeedFormat;

    return reset;
}

SLVMInfo ItemModel::dataToLVM(const SDataInfo &dataInfo)
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

SVGInfo ItemModel::dataToVG(const SDataInfo &dataInfo)
{
    SVGInfo reset;
    reset.id = dataInfo.id;
    reset.name = dataInfo.device;

    return reset;
}

void ItemModel::setDatas(QMap<SDeviceInfo, QList<SPartitionInfo>> devs, QMap<SVGInfo, QList<SLVMInfo>> vgs)
{
    m_dataInfo.clear();
    beginResetModel();
    m_dataInfo = toSDataInfo(devs, vgs);
    endResetModel();
    Q_EMIT signalItemCountChanged(m_dataInfo.count());
}

int ItemModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_dataInfo.count();
}

int ItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ITEMCOLUMN;
}

QVariant ItemModel::data(const QModelIndex &index, int role) const
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
    bool isLV = EDataType::LV == dataInfo.type ?  true : false;
    bool isFree= ESpaceUsage::FREE == dataInfo.usage ? true : false;
    bool isMergeVG= dataInfo.isMergeVG ? true : false;
    jsonObj.insert(ISVG, isVG);
    jsonObj.insert(ISDEV, isDev);
    jsonObj.insert(ISLV, isLV);
    jsonObj.insert(ISFREE, isFree);
    jsonObj.insert(ISMERGEVG, isMergeVG);
    jsonObj.insert(ISCANENCRYPT, dataInfo.isCanEncrypt);
    jsonObj.insert(ISENCRYPT, dataInfo.isEncrypt);

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
        if (!isFree) {
            jsonObj.insert(ITEMFILESYSTEM, dataInfo.fileSystem);
        }
        return jsonObj;
    } else if (icol == 2) {
        if (!isFree) {
            jsonObj.insert(ITEMMOUNTPOINT, dataInfo.mountPoint);
            jsonObj.insert(ISNEEDFORMAT, dataInfo.isNeedFormat);
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

SDataInfo ItemModel::getDataInfos(int iRow)
{
    if (iRow >= m_dataInfo.count()) {
        return SDataInfo();
    }

    return m_dataInfo[iRow];
}


SeniorPartationListView::SeniorPartationListView(QWidget *parent)
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

    m_itemDelegate = new ItemDelegate();
    setItemDelegate(m_itemDelegate);

    m_actionNames << CommonFunc::getOpeartTypeText(EOpeartType::Edit);
    m_actionNames << CommonFunc::getOpeartTypeText(EOpeartType::Delete);
    m_actionNames << CommonFunc::getOpeartTypeText(EOpeartType::Add);
}

void SeniorPartationListView::drawRow(QPainter *painter, const QStyleOptionViewItem &options, const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    DPalette::ColorGroup cg = DPalette::Normal;

    auto *style = dynamic_cast<DStyle *>(DApplication::style());

    auto radius = style->pixelMetric(DStyle::PM_FrameRadius, &options);

    auto palette = options.palette;
    ItemModel *devModel = static_cast<ItemModel *>(model());
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

void SeniorPartationListView::paintEvent(QPaintEvent *event)
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

bool SeniorPartationListView::viewportEvent(QEvent *event)
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

void SeniorPartationListView::mouseMoveEvent(QMouseEvent *event)
{
    BaseTableView::mouseMoveEvent(event);
    QModelIndex index = indexAt(event->pos());
    if (index.row() < 0){
        return;
    }

    ItemModel *itemModel = static_cast<ItemModel *>(model());
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
        if (fm.horizontalAdvance(strName) > (CommonFunc::isBigScreen() ? (seniorPartationInstall::kColumnWidth_0 - seniorPartationInstall::kFirstColunmOffset - seniorPartationInstall::kIconWidth - seniorPartationInstall::kFirstColunmSpace * 3)
                                                           : (seniorPartationInstall::kColumnWidthSmall_0 - seniorPartationInstall::kFirstColunmOffset - seniorPartationInstall::kIconWidth - seniorPartationInstall::kFirstColunmSpace * 3))) {
#else
        if (fm.width(strName) > (CommonFunc::isBigScreen() ? (seniorPartationInstall::kColumnWidth_0 - seniorPartationInstall::kFirstColunmOffset - seniorPartationInstall::kIconWidth - seniorPartationInstall::kFirstColunmSpace * 3)
                                                           : (seniorPartationInstall::kColumnWidthSmall_0 - seniorPartationInstall::kFirstColunmOffset - seniorPartationInstall::kIconWidth - seniorPartationInstall::kFirstColunmSpace * 3))) {
#endif
            QToolTip::showText(QCursor::pos(), strName, this, this->rect(), seniorPartationInstall::kTooltipTime);
        }
    }

    if (index.column() == 1) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        if (fm.horizontalAdvance(dataInfo.fileSystem) > (CommonFunc::isBigScreen() ? (seniorPartationInstall::kColumnWidth_1 - seniorPartationInstall::kFirstColunmOffset)
                                                                       : (seniorPartationInstall::kColumnWidthSmall_1 - seniorPartationInstall::kFirstColunmOffset))) {
#else
        if (fm.width(dataInfo.fileSystem) > (CommonFunc::isBigScreen() ? (seniorPartationInstall::kColumnWidth_1 - seniorPartationInstall::kFirstColunmOffset)
                                                                       : (seniorPartationInstall::kColumnWidthSmall_1 - seniorPartationInstall::kFirstColunmOffset))) {
#endif
            QToolTip::showText(QCursor::pos(), dataInfo.fileSystem, this, this->rect(), seniorPartationInstall::kTooltipTime);
        }
    }

    if (index.column() == 2) {
        int tipsWidth = 0;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        if (!dataInfo.mountPoint.compare("/")) {
            tipsWidth = fm.horizontalAdvance(::QObject::tr("Install here"));
        } else if (dataInfo.isNeedFormat) {
            tipsWidth = fm.horizontalAdvance(::QObject::tr("To be formatted"));
        }

        if (fm.horizontalAdvance(dataInfo.mountPoint) > (((CommonFunc::isBigScreen() ? (seniorPartationInstall::kColumnWidth_2 - seniorPartationInstall::kFirstColunmOffset)
                                                                         : (seniorPartationInstall::kColumnWidthSmall_2 - seniorPartationInstall::kFirstColunmOffset)) - tipsWidth))) {
            QToolTip::showText(QCursor::pos(), dataInfo.mountPoint, this, this->rect(), seniorPartationInstall::kTooltipTime);
        }
#else
        if (!dataInfo.mountPoint.compare("/")) {
            tipsWidth = fm.width(::QObject::tr("Install here"));
        } else if (dataInfo.isNeedFormat) {
            tipsWidth = fm.width(::QObject::tr("To be formatted"));
        }

        if (fm.width(dataInfo.mountPoint) > (((CommonFunc::isBigScreen() ? (seniorPartationInstall::kColumnWidth_2 - seniorPartationInstall::kFirstColunmOffset)
                                                                         : (seniorPartationInstall::kColumnWidthSmall_2 - seniorPartationInstall::kFirstColunmOffset)) - tipsWidth))) {
            QToolTip::showText(QCursor::pos(), dataInfo.mountPoint, this, this->rect(), seniorPartationInstall::kTooltipTime);
        }
#endif
    }

    QRect rect = visualRect(index);
    if (EDataType::DEVICE != dataInfo.type) {
        QStringList opts = getOpearts(dataInfo);
        QMap<EOpeartType, QRect>  actionRects = getItemActionRect(rect, opts);

        //查找是否有按钮处于hover状态
        for (auto iAction: actionRects.keys()) {
            if (actionRects.value(iAction).contains(event->pos()) && index.column() == (ITEMCOLUMN -1)) {
                if ((EDataType::PARTATIONS == dataInfo.type || EDataType::LV == dataInfo.type || EDataType::VG == dataInfo.type) && !dataInfo.isMergeVG) {
                    QToolTip::showText(mapToGlobal(QPoint(actionRects.value(iAction).topRight())), CommonFunc::opeartTypeToText(iAction).key(iAction), this, actionRects.value(iAction), seniorPartationInstall::kTooltipTime);
                    setCursor(Qt::PointingHandCursor);
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

void SeniorPartationListView::mouseReleaseEvent(QMouseEvent *event)
{
    BaseTableView::mouseReleaseEvent(event);
    QModelIndex index = indexAt(event->pos());
    if (index.row() < 0){
        return;
    }
    ItemModel *itemModel = static_cast<ItemModel *>(model());
    SDataInfo dataInfo = itemModel->getDataInfos(index.row());
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
                itemModel->doItemAction(dataInfo, iAction);
                return;
            }
        }
    }
}

