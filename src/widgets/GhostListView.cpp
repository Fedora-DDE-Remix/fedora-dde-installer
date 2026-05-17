#include "GhostListView.h"
#include "CommonFunc.h"

#include <DLabel>
#include <DApplication>
#include <DGuiApplicationHelper>
#include <QAbstractTableModel>
#include <QJsonObject>
#include <QMouseEvent>
#include <QHeaderView>
#include <QPainterPath>
#include <QCheckBox>

//表的列数
#define ITEMCOLUMN 1

//角色
#define DATA_ITEM_ROLE_ID Qt::UserRole + 1

//item各项json名称
#define ITEMICON  "icon"
#define ITEMDEVICENAME "devicename"
#define ITEMTOTALSIZE "totalsize"

//item高度
#define ITEM_Height 50

DiskDelegate::DiskDelegate(QObject *parent): QStyledItemDelegate(parent)
{
}

QSize DiskDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    return index.data(Qt::SizeHintRole).toSize();
}

void DiskDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    QString name = index.data(Qt::DisplayRole).toString();
    QString devPath = index.data(Qt::UserRole + 100).toString();
    QIcon checkIcon = qobject_cast<DStyle*>(QApplication::style())->standardIcon(DStyle::SP_IndicatorChecked);
    bool bChecked = option.state & QStyle::State_Selected;
    bool bHovered = option.state & QStyle::State_MouseOver;
    QColor bgColor = bHovered ? index.data(Qt::UserRole + 110).value<QColor>() : index.data(Qt::BackgroundRole).value<QColor>();
    QColor textColor = option.palette.color(QPalette::WindowText);
    QColor tipColor = index.data(Qt::UserRole + 111).value<QColor>();
    auto *model = index.model();
    int count = model->rowCount();
    int radius = 8;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->setBrush(bgColor);
    if(count == 1) {
        painter->drawRoundedRect(option.rect, radius, radius);
    } else if(index.row() == 0) {
        DDrawUtils::drawRoundedRect(painter, option.rect, radius, radius, DDrawUtils::TopLeftCorner | DDrawUtils::TopRightCorner);
    } else if (index.row() == count - 1) {
        DDrawUtils::drawRoundedRect(painter, option.rect, radius, radius, DDrawUtils::BottomLeftCorner | DDrawUtils::BottomRightCorner);
    } else {
        painter->drawRect(option.rect);
    }

    QRect contentRect = option.rect.adjusted(10, 0, -10, 0);
    //draw icon
    QRect iconRect;
    iconRect = QApplication::style()->alignedRect(Qt::LeftToRight, Qt::AlignLeft | Qt::AlignVCenter, option.decorationSize, contentRect);
    icon.paint(painter, iconRect);
    //draw indicate icon
    QRect checkRect;
    checkRect = QApplication::style()->alignedRect(Qt::LeftToRight, Qt::AlignRight | Qt::AlignVCenter, QSize(20, 20), contentRect);
    if(bChecked) {
        checkIcon.paint(painter, checkRect);
    }
    //draw name and path
    QRect nameRect;
    nameRect.setX(iconRect.right() + 10);
    nameRect.setY(contentRect.y() + 5);
    //nameRect.setTopLeft(iconRect.topRight() + QPoint(10, 5));
    nameRect.setWidth(checkRect.topLeft().x() - iconRect.topRight().x() - 15);

    QFont nameFont = painter->font();
    nameFont.setWeight(QFont::DemiBold);
    nameFont.setPixelSize(12);
    painter->setFont(nameFont);
    painter->setPen(textColor);
    QFontMetrics metrics = painter->fontMetrics();
    QRect nameBound = metrics.boundingRect(QRect(), Qt::TextSingleLine, name);
    nameRect.setHeight(nameBound.height());

    int nameMaxLength = nameRect.width();
    if(nameBound.width() > nameMaxLength) {
        name = metrics.elidedText(name, Qt::ElideRight, nameMaxLength);
    }
    painter->drawText(nameRect, Qt::AlignLeft | Qt::TextSingleLine, name);

    QFont detailFont = painter->font();
    detailFont.setWeight(QFont::Thin);
    detailFont.setPixelSize(11);
    painter->setFont(detailFont);
    painter->setPen(tipColor);
    metrics = painter->fontMetrics();
    QRect detailBound = metrics.boundingRect(QRect(), Qt::TextSingleLine, devPath);
    QRect detailRect;
    detailRect.moveTopLeft(nameRect.bottomLeft() + QPoint(0, 2));
    detailRect.setWidth(nameRect.width());
    detailRect.setHeight(detailBound.height());
    painter->drawText(detailRect, Qt::AlignLeft | Qt::TextSingleLine, devPath);
    painter->restore();
}
