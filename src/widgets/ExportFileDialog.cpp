#include "ExportFileDialog.h"
#include "DeviceManager.h"
#include "CommonFunc.h"
#include "WarningDialog.h"
#include <DStyle>
#include <QTimer>
#include <QHBoxLayout>
#include <QLabel>
#include <QAbstractButton>
#include <QItemSelectionModel>
#include <DPlatformWindowHandle>
#include <QScreen>

DWIDGET_USE_NAMESPACE

UDiskDelegate::UDiskDelegate(QObject *parent):QStyledItemDelegate (parent)
{
    ;
}

QSize UDiskDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    return index.data(Qt::SizeHintRole).toSize();
}

void UDiskDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    QString name = index.data(Qt::DisplayRole).toString();
    int total = index.data(Qt::UserRole + 1).toInt();
    int avail = index.data(Qt::UserRole + 2).toInt();
    QString totalString = QString(tr("Total Size:%1GiB")).arg(total);
    QString availableString = QString(tr("Availabel Size:%1GiB")).arg(avail);
    bool bHover = option.state & QStyle::State_MouseOver;
    QColor bgColor = bHover ? "#33000000" : "#19000000";
    QColor textColor = option.palette.color(QPalette::WindowText);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->setBrush(bgColor);
    painter->drawRoundedRect(option.rect, 10, 10);
    QRect contentRect = option.rect.adjusted(30, 24, -30, -16);
    //draw checked icon
    QIcon checked = qobject_cast<DStyle*>(QApplication::style())->standardIcon(DStyle::SP_IndicatorChecked);
    QRect checkIconRect;
    checkIconRect.setSize(QSize(24, 24));
    checkIconRect.moveTopRight(option.rect.topRight() + QPoint(-10, 10));
    if(option.state & QStyle::State_Selected) {
        checked.paint(painter, checkIconRect);
    }
    //draw icon
    QRect iconRect;
    iconRect.setTopLeft(contentRect.topLeft() + QPoint(-10, 0));
    iconRect.setSize(option.decorationSize);
    icon.paint(painter, iconRect);
    //draw name and size
    QFont nameFont = painter->font();
    nameFont.setWeight(QFont::DemiBold);
    nameFont.setPixelSize(16);
    painter->setFont(nameFont);
    painter->setPen(textColor);
    QFontMetrics metrics = painter->fontMetrics();
    QRect nameBound = metrics.boundingRect(QRect(), Qt::TextSingleLine, name);
    QRect nameRect;
    nameRect.moveTopLeft(iconRect.topRight() + QPoint(10, 0));
    nameRect.setWidth(contentRect.topRight().x() - nameRect.topLeft().x());
    nameRect.setHeight(nameBound.height());
    painter->drawText(nameRect, Qt::TextSingleLine, name);

    QFont detailFont = painter->font();
    detailFont.setWeight(QFont::Normal);
    detailFont.setPixelSize(12);
    painter->setFont(detailFont);
    painter->setPen(QColor("#B2000000"));
    metrics = painter->fontMetrics();
    QRect detailBound = metrics.boundingRect(QRect(), Qt::TextSingleLine, totalString + availableString);
    QRect detailRect;
    detailRect.moveTopLeft(nameRect.bottomLeft() + QPoint(0, 10));
    detailRect.setWidth(nameRect.width());
    detailRect.setHeight(detailBound.height());
    painter->drawText(detailRect, Qt::AlignLeft | Qt::TextSingleLine, totalString);
    painter->drawText(detailRect, Qt::AlignRight | Qt::TextSingleLine, availableString);
    painter->restore();
    //draw progressbar
    painter->save();
    QRect barRect;
    barRect.moveBottomRight(QPoint(nameRect.topLeft().x(), contentRect.bottomLeft().y() - 6 - 9));
    barRect.setWidth(nameRect.width());
    barRect.setHeight(6);
    QStyleOptionProgressBar optionBar;
    optionBar.rect = barRect;
    optionBar.minimum = 0;
    optionBar.maximum = total;
    optionBar.progress = total - avail;
    QApplication::style()->drawControl(QStyle::CE_ProgressBar, &optionBar, painter);
    painter->restore();
}

ExportFileDialog::ExportFileDialog(QWidget *parent):DDialog (parent)
{
    setFixedWidth(560);
    setFixedHeight(497);
    initUI();
    move(qApp->primaryScreen()->availableSize().width() / 2 - width() / 2,
         qApp->primaryScreen()->availableSize().height() / 2 - height() / 2);
}

void ExportFileDialog::setTitleAndMessage(const QString &title, const QString &message)
{
    if(!title.isEmpty()) {
        setTitle(title);
    }

    if(!message.isEmpty()) {
        m_tipLabel->setText(message);
    }
}

void ExportFileDialog::onDelUsbDevice(const QString &id)
{
    int row = 0;
    for (; row < m_devModel->rowCount(); ++row) {
        auto *item = m_devModel->item(row);
        QString deviceId = item->data(Qt::UserRole + 3).toString();
        if(deviceId == id) {
            break;
        }
    }

    if(row < m_devModel->rowCount()) {
        m_devModel->removeRow(row);
    }
}

void ExportFileDialog::setRecoveryKey(const QString &cryptKey)
{
    if(m_cryptKey != cryptKey) {
        m_cryptKey = cryptKey;
        if(m_cryptKey.isEmpty()) {
            m_labelKey->setVisible(false);
        } else {
            m_labelKey->setText(tr("Key: ") + m_cryptKey);
        }
    }
}

void ExportFileDialog::updateTs()
{
    getButton(0)->setText(::QObject::tr("Cancel"));
    getButton(1)->setText(::QObject::tr("Save"));
}

void ExportFileDialog::initUI()
{
    setOnButtonClickedClose(false);

    m_tipLabel = new DTipLabel;
    m_tipLabel->setFixedWidth(526);
    m_tipLabel->setWordWrap(true);
    m_tipLabel->setMinimumHeight(48);
    addContent(m_tipLabel, Qt::AlignHCenter);
    addSpacing(10);

    m_labelKey = new DLabel;
    DFontSizeManager::instance()->bind(m_labelKey, DFontSizeManager::T5, QFont::DemiBold);
    addContent(m_labelKey, Qt::AlignHCenter);

    m_devModel = new QStandardItemModel;
    m_devList = new QListView;
    m_devList->setFixedWidth(540);
    m_devList->setSpacing(10);
    m_devList->setMinimumHeight(280);
    m_devList->setIconSize(QSize(80, 80));
    m_devList->setStyleSheet(""
                            "QWidget:focus {"
                            "border: 2px solid;"
                            "border-radius:5px;"
                            "border-color:rgb(1, 128, 255)"
                            "}");
    m_devList->setFrameShape(QFrame::NoFrame);
    m_devList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_devList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_devList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_devList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_devList->viewport()->setAutoFillBackground(false);
    m_devList->setAutoFillBackground(false);
    m_devList->setFocusPolicy(Qt::TabFocus);
    m_devList->setBackgroundRole(m_devList->viewport()->backgroundRole());
    m_devList->setModel(m_devModel);
    m_devList->setItemDelegate(new UDiskDelegate);
    addSpacing(5);
    addContent(m_devList, Qt::AlignHCenter);

    addButton(::QObject::tr("Cancel"));
    addButton(::QObject::tr("Save"), true, ButtonRecommend);

    m_btnCancel = getButton(0);
    m_btnConfirm = getButton(1);
    m_btnConfirm->setEnabled(false);
    auto *selectModel = m_devList->selectionModel();
    connect(m_btnCancel, &QAbstractButton::clicked, this, &QDialog::reject);
    connect(selectModel, &QItemSelectionModel::currentChanged, [this](const QModelIndex &current, const QModelIndex &pre){
        if(!pre.isValid() && current.isValid()) {
            m_devList->selectionModel()->select(current, QItemSelectionModel::SelectCurrent);
        }

        this->m_btnConfirm->setEnabled(current.isValid());
    });
    connect(m_btnConfirm, &QAbstractButton::clicked, this, [this]{
        auto *item = m_devModel->itemFromIndex(m_devList->currentIndex());
        m_devSave = item->data(Qt::UserRole + 3).toString();
        m_devMount = item->data(Qt::UserRole + 4).toString();
        accept();
    });
}

void ExportFileDialog::initConnection()
{
}

void ExportFileDialog::showEvent(QShowEvent *event)
{
    DDialog::showEvent(event);
}

void ExportFileDialog::onAddUsbDevice(const QString &id, const QString &diskName, const QString &mount, int total, int avail)
{
    QStandardItem *item = new QStandardItem;
    item->setSizeHint(QSize(520, 120));
    item->setCheckable(true);
    item->setIcon(QIcon(":/images/drive-harddisk-small.svg"));
    item->setText(diskName);
    item->setData(total, Qt::UserRole + 1);
    item->setData(avail, Qt::UserRole + 2);
    item->setData(id, Qt::UserRole + 3);
    item->setData(mount, Qt::UserRole + 4);
    m_devModel->appendRow(item);
}

SaveFileToUsb::SaveFileToUsb(QObject *parent):QObject (parent)
{
    m_epFileDlg = new ExportFileDialog;
    DPlatformWindowHandle handle(m_epFileDlg);
    handle.setWindowRadius(18);
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &SaveFileToUsb::onScanOver);
    connect(m_epFileDlg, &QDialog::finished, this, [=](int result){
        m_timer->stop();
        DeviceManager::instance()->scanUSBDevices(false);
        if(result == QDialog::Accepted) {
            this->exportFileToUsb();
        }
    });
}

void SaveFileToUsb::setRecoveryKey(const QString &key)
{
    m_epFileDlg->setRecoveryKey(key);
}

void SaveFileToUsb::setSaveFileList(const QStringList &fileList)
{
    qInfo() << "set save files:" << fileList;
    m_fileList = fileList;
}

void SaveFileToUsb::updateTs()
{
    m_epFileDlg->updateTs();
}

void SaveFileToUsb::beginSaveFile()
{
    if(!m_title.isEmpty() || !m_message.isEmpty()) {
        m_epFileDlg->setTitleAndMessage(m_title, m_message);
    }

    DeviceManager::instance()->scanUSBDevices(true);
    m_timer->start();
    m_epFileDlg->exec();
}

void SaveFileToUsb::onScanOver()
{
    QStringList tempList;
    QList<SDeviceInfo> deviceInfos = DeviceManager::instance()->getUSBDeviceInfos();
    std::transform(deviceInfos.begin(), deviceInfos.end(), std::back_inserter(tempList), [](const SDeviceInfo &info){return info.device;});
    auto iter = m_devList.begin();
    for (; iter != m_devList.end(); ) {
        if(!tempList.contains(*iter)) {
            m_epFileDlg->onDelUsbDevice(*iter);
            iter = m_devList.erase(iter);
        } else {
            ++iter;
        }
    }
    //
    for (SDeviceInfo deviceInfoItem : deviceInfos) {
        // 过滤掉启动盘设备
        QString output;
        if (deviceInfoItem.isStartDevice && deviceInfoItem.mountPoint.startsWith("/usr/lib/live")) {
            QString newMountPoint = "/target";
            newMountPoint += deviceInfoItem.mountPoint;
            QDir dir(newMountPoint);
            if (!dir.exists()) {
                dir.mkpath(newMountPoint);
            }
            bool ok = CommonFunc::command("mount",
                                          QStringList() << "-v" << "--bind" << deviceInfoItem.mountPoint << newMountPoint, output);
            if (!ok) {
                qWarning() << output;
            }
        }

        // 过滤掉挂载位置不可写的设备
        if (!CommonFunc::mountPointWriteable(deviceInfoItem.mountPoint)) {
            continue;
        }

        if(!m_devList.contains(deviceInfoItem.device)) {
            m_epFileDlg->onAddUsbDevice(deviceInfoItem.device, deviceInfoItem.name,
                                      deviceInfoItem.mountPoint,
                                     (int)CommonFunc::mToG(deviceInfoItem.size),
                                     (int)CommonFunc::mToG(deviceInfoItem.availSize));
            m_devList.append(deviceInfoItem.device);
        }
    }
}

void SaveFileToUsb::exportFileToUsb()
{
    //导出成功提示弹窗
    QString deviceId;
    QString mountPoint;
    m_epFileDlg->getSelectedDevice(deviceId, mountPoint);
    //copy file
    bool bCopyResult = false;
    if(mountPoint.isEmpty()) {
        Q_EMIT copyDone(false, deviceId);
        return;
    }

    QString recoveryKey = m_epFileDlg->getRecoveryKey();
    if(m_fileList.isEmpty()) {
        m_fileList.append(CommonFunc::getSettingsString("DI_CRYPT_RECOVERY_KEY"));
    }

    bCopyResult = copyFiles(mountPoint);
    // 拷贝完日志后，执行一遍sync同步数据
    CommonFunc::sync();
    Q_EMIT copyDone(bCopyResult, deviceId);
}

bool SaveFileToUsb::copyFiles(const QString &dest)
{
    QString log = "";
    QString cmd = QString("cp -rf %1 \"%2\"").arg(m_fileList.join(" "), dest);
    if ( !CommonFunc::command("/bin/bash", {"-c", cmd}, log) ) {
        qCritical() << "cp log error cmd : " << cmd << " err : " << log;
        return false;
    } else {
        qInfo() << QString("Command: %1 succeed.").arg(cmd);
        return true;
    }
}
