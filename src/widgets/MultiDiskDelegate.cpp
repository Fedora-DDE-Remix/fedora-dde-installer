#include "MultiDiskDelegate.h"
#include <QApplication>
#include <QPainter>

MultiDiskDelegate::MultiDiskDelegate(QObject *parent) : QStyledItemDelegate (parent)
{

}

QSize MultiDiskDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    return index.data(Qt::SizeHintRole).toSize();
}

void MultiDiskDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    QString name = index.data(Qt::DisplayRole).toString();
    QString total = index.data(Qt::UserRole + 1).toString();
    int used = index.data(Qt::UserRole + 2).toInt();
    QString path = index.data(Qt::UserRole + 3).toString();
    QWidget *embed = index.data(Qt::UserRole + 4).value<QWidget*>();
    bool bChecked = option.state & QStyle::State_Selected;
    QString sizeTip = bChecked ? QObject::tr("total/system") : tr("total");
    QString totalString = bChecked ? QString("%1GiB/").arg(total): QString("%1GiB").arg(total);
    QString usedString = bChecked ? QString("%1GiB").arg(used) : "";
    bool bHover = option.state & QStyle::State_MouseOver;
    QColor bgColor = bHover ? index.data(Qt::UserRole + 110).value<QColor>() : index.data(Qt::BackgroundRole).value<QColor>();
    QColor textColor = option.palette.color(QPalette::WindowText);
    QColor highlightColor = option.palette.color(QPalette::Highlight);
    embed->setVisible(bChecked);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->setBrush(bgColor);
    painter->drawRoundedRect(option.rect, 10, 10);
    QRect contentRect = option.rect.adjusted(10, 10, -15, -10);
    //draw checked box
    QRect checkRect;
    checkRect.setSize(QSize(20, 20));
    checkRect.moveTopLeft(QPoint(contentRect.topLeft().x(), option.rect.topLeft().y() + option.rect.height() / 2 - 10));
    QStyleOptionButton checkOption;
    checkOption.rect = checkRect;
    checkOption.iconSize = QSize(16, 16);
    checkOption.state = bChecked ? QStyle::State_On : QStyle::State_Off;
    checkOption.state |= QStyle::State_Enabled;
    QApplication::style()->drawControl(QStyle::CE_RadioButton, &checkOption, painter);
    //draw icon
    QRect iconRect;
    iconRect.setTopLeft(QPoint(checkRect.topRight().x() + 10, contentRect.topLeft().y()));
    iconRect.setSize(option.decorationSize);
    icon.paint(painter, iconRect);
    //draw detail
    QFont nameFont = painter->font();
    nameFont.setWeight(QFont::DemiBold);
    nameFont.setPixelSize(16);
    painter->setFont(nameFont);
    painter->setPen(textColor);
    QFontMetrics metrics = painter->fontMetrics();
    QRect nameBound = metrics.boundingRect(QRect(), Qt::TextSingleLine, name);
    QRect usedBound = metrics.boundingRect(QRect(), Qt::TextSingleLine, usedString);
    QRect sizeBound = metrics.boundingRect(QRect(), Qt::TextSingleLine, totalString);
    QRect nameRect;
    nameRect.moveTopLeft(iconRect.topRight() + QPoint(10, 0));
    int nameWidth = contentRect.topRight().x() - nameRect.topLeft().x();
    if(bChecked) {
        nameWidth -= embed->width() + 5;
    }
    nameRect.setWidth(nameWidth);
    nameRect.setHeight(nameBound.height());
    QRect totalRect = nameRect;

    int nameMaxLength = nameRect.width() - sizeBound.width() - usedBound.width() - 5;
    if(nameBound.width() > nameMaxLength) {
        name = metrics.elidedText(name, Qt::ElideRight, nameMaxLength);
    }
    painter->drawText(nameRect, Qt::AlignLeft | Qt::TextSingleLine, name);
    if(bChecked) {
        painter->setPen(highlightColor);
        painter->drawText(nameRect, Qt::AlignRight | Qt::TextSingleLine, usedString);
        totalRect.setWidth(nameRect.width() - usedBound.width());
    }
    painter->setPen(textColor);
    painter->drawText(totalRect, Qt::AlignRight | Qt::TextSingleLine, totalString);
    //draw embed
    if(bChecked) {
        QRect embedRect;
        embedRect.setSize(embed->size());
        embedRect.moveTopLeft(nameRect.topRight() + QPoint(5, 0));
        embed->setGeometry(embedRect);
    }

    QFont detailFont = painter->font();
    detailFont.setWeight(QFont::Normal);
    detailFont.setPixelSize(12);
    painter->setFont(detailFont);
    metrics = painter->fontMetrics();
    QRect detailBound = metrics.boundingRect(QRect(), Qt::TextSingleLine, path + sizeTip);
    QRect detailRect;
    detailRect.moveTopLeft(nameRect.bottomLeft() + QPoint(0, 10));
    detailRect.setWidth(contentRect.topRight().x() - detailRect.topLeft().x());
    detailRect.setHeight(detailBound.height());
    painter->drawText(detailRect, Qt::AlignLeft | Qt::TextSingleLine, path);
    painter->drawText(detailRect, Qt::AlignRight | Qt::TextSingleLine, sizeTip);
    painter->restore();
}
