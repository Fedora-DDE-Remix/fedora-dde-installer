#include "AppResultDialog.h"
#include <QHBoxLayout>
#include <QPainter>
#include <DStyleHelper>
#include <DPaletteHelper>
#include <DPlatformWindowHandle>
#include <QSettings>
#include <QScreen>

DWIDGET_USE_NAMESPACE

AppResultDelegate::AppResultDelegate(QObject *parent):QStyledItemDelegate (parent)
{
    ;
}

QSize AppResultDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    return index.data(Qt::SizeHintRole).toSize();
}

void AppResultDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    QString name = index.data(Qt::DisplayRole).toString();
    bool bSuccess = index.data(Qt::UserRole + 100).toBool();
    QString stateText = bSuccess ? tr("Install Success") : tr("Install Failed");
    auto *model = index.model();
    int count = model->rowCount();
    int radius = 8;
    QColor bgColor = "#0C000000";
    QColor textColor = option.palette.color(QPalette::WindowText);

    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(bgColor));
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
    //draw appIcon
    QRect iconRect;
    iconRect = QApplication::style()->alignedRect(Qt::LeftToRight, Qt::AlignLeft | Qt::AlignVCenter, option.decorationSize, contentRect);
    icon.paint(painter, iconRect);
    //draw state text
    painter->setPen(textColor);
    QFont stateFont = painter->font();
    stateFont.setPixelSize(14);
    painter->setFont(stateFont);
    QFontMetrics metrics = painter->fontMetrics();
    QRect nameBound = metrics.boundingRect(QRect(), Qt::TextSingleLine, stateText);
    QRect tipRect;
    tipRect = QApplication::style()->alignedRect(Qt::LeftToRight, Qt::AlignRight | Qt::AlignVCenter, nameBound.size(), contentRect);
    painter->drawText(tipRect, Qt::TextSingleLine, stateText);
    //draw state appIcon
    QIcon stateIcon = bSuccess ? QIcon::fromTheme("icon_ok") : QIcon::fromTheme("icon_warning");
    QRect stateIconRect;
    QSize iconSize(30, 30);
    stateIconRect.setSize(iconSize);
    stateIconRect.moveCenter(QPoint(tipRect.topLeft().x() - iconSize.width() / 2 - 5, tipRect.center().y()));
    stateIcon.paint(painter, stateIconRect);
    //draw text
    QFont nameFont = painter->font();
    nameFont.setWeight(QFont::DemiBold);
    nameFont.setPixelSize(14);
    painter->setFont(nameFont);
    metrics = painter->fontMetrics();

    int nameMaxWidth = stateIconRect.topLeft().x() - iconRect.topRight().x() - 10;
    nameBound = metrics.boundingRect(QRect(), Qt::TextSingleLine, name);
    QRect nameRect;
    nameRect.setWidth(nameMaxWidth);
    nameRect.setHeight(nameBound.height());
    nameRect.moveTopLeft(QPoint(iconRect.topRight().x() + 10, iconRect.center().y() - nameRect.height() / 2));
    if(nameBound.width() > nameMaxWidth) {
        name = metrics.elidedText(name, Qt::ElideRight, nameMaxWidth);
    }

    painter->drawText(nameRect, Qt::TextSingleLine, name);
    painter->restore();
}

AppResultDialog::AppResultDialog(QWidget *parent) : DDialog (parent)
{
    DPlatformWindowHandle *windowHandle = new DPlatformWindowHandle(this);
    windowHandle->setWindowRadius(18);

    setFixedWidth(440);
    setTitle(::QObject::tr("App(s) installation result"));
    setMessage(::QObject::tr("If app installation failed, please try again in system."));

    m_model = new QStandardItemModel;
    m_appview = new QListView;
    m_appview->setModel(m_model);
    m_appview->setIconSize(QSize(32, 32));
    m_appview->setSpacing(1);
    m_appview->setFrameShape(QFrame::NoFrame);
    m_appview->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_appview->setSelectionMode(QAbstractItemView::NoSelection);
    m_appview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_appview->setSizeAdjustPolicy(QAbstractItemView::AdjustToContents);
    m_appview->setMinimumHeight(240);
    m_appview->viewport()->setAutoFillBackground(false);
    m_appview->setAutoFillBackground(false);
    m_appview->setBackgroundRole(m_appview->viewport()->backgroundRole());
    m_appview->setItemDelegate(new AppResultDelegate);

    addSpacing(20);
    addContent(m_appview);
    addSpacing(10);
    addButton(::QObject::tr("Confirm"), true, DDialog::ButtonNormal);

    move(qApp->primaryScreen()->availableSize().width() / 2 - width() / 2,
         qApp->primaryScreen()->availableSize().height() / 2 - height() / 2);
}

void AppResultDialog::loadAppResult()
{
    m_model->clear();
    QList<AppInfo> appInfos;
#ifdef QT_DEBUG
    QSettings settings("/etc/fedora-dde-installer/app-info.ini", QSettings::IniFormat);
#else
    QSettings settings("/target/etc/fedora-dde-installer/app-info.ini", QSettings::IniFormat);
#endif
    auto groups = settings.childGroups();
    for (auto &group : groups) {
        settings.beginGroup(group);
        AppInfo appInfo;
        appInfo.appName = settings.value("appName").toString();
        appInfo.appPath = settings.value("appPath").toString();
        appInfo.appIcon = settings.value("appIcon").toString();
        appInfo.installed = settings.value("installed").toBool();
        settings.endGroup();

        QStandardItem *item = new QStandardItem;
        item->setSizeHint(QSize(-1, 52));
        if (appInfo.appIcon.isEmpty()) {
            item->setIcon(QIcon(":/images/deb.svg"));
        } else {
            item->setIcon(QIcon(appInfo.appIcon));
        }
        item->setText(appInfo.appName);
        item->setData(appInfo.installed, Qt::UserRole + 100);
        m_model->appendRow(item);
    }
}
