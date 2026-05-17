#include "SaveDeviceWidget.h"
#include "CommonFunc.h"
#include "DeviceManager.h"
#include "TitleWidget.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <QDebug>

const int titleLabelWidth = 540;
const int btnWidth = 180;
const int btnHeight = 36;

SaveDeviceWidget::SaveDeviceWidget(QWidget *parent) : DDialog(parent)
{
    this->initUI();
}

void SaveDeviceWidget::saveList(const QStringList &fileList)
{
    m_fileList = fileList;
}

void SaveDeviceWidget::setTitleText(const QString &text, const QString &style)
{
    m_titleWidget->setTitleLabelProperty(text, style);
}

void SaveDeviceWidget::setTipsText(const QString &text, const QString &strStyleSheet, bool isUseDefaultWidth)
{
    m_titleWidget->setTipsLabelProperty(text, strStyleSheet, isUseDefaultWidth);
}

QString SaveDeviceWidget::getCurrentSelectDevice()
{
    if (m_deviceList->currentItem() != nullptr) {
        return m_deviceList->currentItem()->data(Qt::UserRole).value<QStringList>().first();
    } else {
        return QString();
    }
}

bool SaveDeviceWidget::getSaveStatus()
{
    return m_isExecSave;
}

void SaveDeviceWidget::saveBtnClickedSlot()
{
    // 直接拷贝日志到对应目录下
    if (m_deviceList->currentItem() != nullptr) {
        QString mountPoint = m_deviceList->currentItem()->data(Qt::UserRole).value<QStringList>().last();
        if(!mountPoint.isEmpty() && !m_fileList.isEmpty()) {
            QString log = "";
            QString cmd = QString("cp -rf %1 \"%2\"").arg(m_fileList.join(" ")).arg(mountPoint);
            if ( !CommonFunc::command("/bin/bash", {"-c", cmd}, log) ) {
                qCritical() << "cp log error cmd : " << cmd << " err : " << log;
                close();
                m_isExecSave = false;
                return;
            }

            qInfo() << QString("Command: %1 succeed.").arg(cmd);
        }

        // 拷贝完日志后，执行一遍sync同步数据
        CommonFunc::sync();
        m_isExecSave = true;
    }
    close();
}

void SaveDeviceWidget::cancelBtnClickedSlot()
{
    clearItemSelectStatus();
    close();
}

void SaveDeviceWidget::deviceListItemClickedSlot()
{
    // 设置项的选中状态
    QListWidgetItem *item = m_deviceList->currentItem();
    if (item != nullptr) {
        m_currSelectDeviceName = item->data(Qt::UserRole).value<QStringList>().first();
        setItemSelectStyle(item, true);
        for (int i = 0; i < m_deviceList->count(); i++) {
            if (!m_deviceList->item(i)->isSelected()) {
                setItemSelectStyle(m_deviceList->item(i), false);
            }
        }

        m_saveBtn->setEnabled(true);
    }
}

void SaveDeviceWidget::initUI()
{
    // 标题栏
    m_titleWidget = new TitleWidget(this);
    m_titleWidget->setTipsLabelWidth(CommonFunc::getShowSize().width() / 2);

    //保存
    m_saveBtn = new QPushButton;
    m_saveBtn->setStyleSheet("QPushButton{color:#0081FF;}");

    m_saveBtn->setText(::QObject::tr("Save", "button"));
    m_saveBtn->setMaximumSize(btnWidth, btnHeight);
    m_saveBtn->setMinimumSize(btnWidth, btnHeight);
    m_saveBtn->setEnabled(false);
    m_saveBtn->setFocusPolicy(Qt::TabFocus);
    m_saveBtn->setDefault(true);

    connect(m_saveBtn, &QPushButton::clicked, this, &SaveDeviceWidget::saveBtnClickedSlot);

    // 返回
    m_cancelBtn = new QPushButton;
    m_cancelBtn->setText(::QObject::tr("Back", "button"));
    m_cancelBtn->setMaximumSize(btnWidth, btnHeight);
    m_cancelBtn->setMinimumSize(btnWidth, btnHeight);
    m_cancelBtn->setFocusPolicy(Qt::TabFocus);
    m_cancelBtn->setDefault(true);
    connect(m_cancelBtn, &QPushButton::clicked, this, &SaveDeviceWidget::cancelBtnClickedSlot);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->addWidget(m_cancelBtn);
    buttonsLayout->addSpacing(20);
    buttonsLayout->addWidget(m_saveBtn);
    QWidget* buttonsWidget = new QWidget;
    buttonsWidget->setLayout(buttonsLayout);

    // 存储设备列表
    m_deviceList = new QListWidget;
    m_deviceList->setObjectName("DeviceList");
    m_deviceList->setSpacing(30);
    m_deviceList->setViewMode(QListView::IconMode);
    m_deviceList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_deviceList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_deviceList->setDragEnabled(false);
    m_deviceList->setSelectionMode (QAbstractItemView::SingleSelection);
    m_deviceList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_deviceList->setStyleSheet("QListWidget#DeviceList{ background-color:rgba(255, 255, 255, 150); border:1px solid; border-color:rgb(255, 255, 255); border-radius:15px; padding:2px 4px; }");
    connect(m_deviceList, &QListWidget::itemPressed, this, &SaveDeviceWidget::deviceListItemClickedSlot);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(60, 20, 60, 10);
    mainLayout->addWidget(m_titleWidget, 0, Qt::AlignHCenter);
    mainLayout->addWidget(m_deviceList);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(buttonsWidget, 0, Qt::AlignHCenter);

    QWidget *pInnerWidget = new QWidget(this);
    pInnerWidget->setObjectName("pInnerWidget");
    pInnerWidget->setLayout(mainLayout);

    this->addContent(pInnerWidget);
    this->setStyleSheet("background-color:rgba(255, 255, 255, 150);");
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &SaveDeviceWidget::updateDevicesList);
}

void SaveDeviceWidget::updateDevicesList()
{
    m_deviceList->clear();

    QList<SDeviceInfo> deviceInfos = DeviceManager::instance()->getUSBDeviceInfos();
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
            bool ok = CommonFunc::command("mount", QStringList() << "-v"
            << "--bind"
            << deviceInfoItem.mountPoint
            << newMountPoint,
            output);

            if (!ok) {
                qWarning() << output;
            }
        }

        // 过滤掉挂载位置不可写的设备
        if (!CommonFunc::mountPointWriteable(deviceInfoItem.mountPoint)) {
            continue;
        }

        auto item = addInfoListItem(deviceInfoItem.name, deviceInfoItem.mountPoint, deviceInfoItem.size, deviceInfoItem.availSize);
        if (m_currSelectDeviceName == deviceInfoItem.name) {
            m_deviceList->setCurrentItem(item);
            deviceListItemClickedSlot();
        }
    }
}

QListWidgetItem* SaveDeviceWidget::addInfoListItem(const QString &devicePath, const QString &mountPoint, qint64 devSize, qint64 devFreeSize)
{
    // 图标 label
    QLabel* deviceInfoIcoLabel = new QLabel;
    deviceInfoIcoLabel->setContentsMargins(0, 0, 0, 0);
    QPixmap tmpPixmap = QPixmap(":/images/drive-harddisk-128px.svg");
    deviceInfoIcoLabel->setPixmap(tmpPixmap.scaled(50, 50));
    deviceInfoIcoLabel->setFixedSize(QSize(50, 50));
    QVBoxLayout* deviceInfoIcoLayout = new QVBoxLayout;
    deviceInfoIcoLayout->setContentsMargins(0,5,0,0);
    deviceInfoIcoLayout->addWidget(deviceInfoIcoLabel, 0, Qt::AlignVCenter);
    //
    QLabel* deviceInfoIcoLeftLabel = new QLabel;
    deviceInfoIcoLeftLabel->setFixedSize(QSize(32, 32));
    QVBoxLayout* deviceInfoIcoLeftLayout = new QVBoxLayout;
    deviceInfoIcoLeftLayout->setContentsMargins(0,0,0,0);
    deviceInfoIcoLeftLayout->addWidget(deviceInfoIcoLeftLabel);
    deviceInfoIcoLeftLayout->addStretch();
    //
    QLabel* deviceInfoIcoRightLabel = new QLabel;
    deviceInfoIcoRightLabel->setObjectName("checkSelectIco");
    deviceInfoIcoRightLabel->setFixedSize(QSize(32, 32));
    QVBoxLayout* deviceInfoIcoRightLayout = new QVBoxLayout;
    deviceInfoIcoRightLayout->setContentsMargins(0,0,0,0);
    deviceInfoIcoRightLayout->addWidget(deviceInfoIcoRightLabel);
    deviceInfoIcoRightLayout->addStretch();

    QHBoxLayout* deviceIcoLayout = new QHBoxLayout;
    deviceIcoLayout->setSpacing(0);
    deviceIcoLayout->setContentsMargins(0,0,0,0);
    deviceIcoLayout->addLayout(deviceInfoIcoLeftLayout);
    deviceIcoLayout->addLayout(deviceInfoIcoLayout);
    deviceIcoLayout->addLayout(deviceInfoIcoRightLayout);
    QWidget* deviceIcoWidget = new QWidget;
    deviceIcoWidget->setLayout(deviceIcoLayout);

    // name label
    QLabel* deviceNameTextLabel = new QLabel;
    deviceNameTextLabel->setObjectName("nameLabel");
    QFontMetrics nameFontWidth(deviceNameTextLabel->font());
    QString deviceNameText = nameFontWidth.elidedText(devicePath.right(devicePath.length() - devicePath.lastIndexOf("/") - 1), Qt::ElideRight, 60);
    deviceNameTextLabel->setText(deviceNameText);

    // size label
    QLabel* deviceSizeTextLabel = new QLabel;
    deviceSizeTextLabel->setObjectName("sizeLabel");
    QString deviceSizeText = CommonFunc::mToG(devSize) < 1 
                            ? QString("%1/%2MiB").arg(devSize - devFreeSize).arg(devSize)
                            : QString("%1/%2GiB").arg(CommonFunc::mToG(devSize - devFreeSize)).arg(CommonFunc::mToG(devSize));
    deviceSizeTextLabel->setText(deviceSizeText);

    // size bar
    QProgressBar* deviceUsageBar = new QProgressBar;
    deviceUsageBar->setContentsMargins(0,0,0,0);
    deviceUsageBar->setRange(0, devSize);
    deviceUsageBar->setValue(devSize - devFreeSize);
    deviceUsageBar->setFixedHeight(8);
    deviceUsageBar->setTextVisible(false);
    deviceUsageBar->setStyleSheet("QProgressBar{ border:none; background-color:rgba(0,0,0,0.1); border-radius:3px;}"\
                 "QProgressBar::chunk{ background-color:rgba(0,129,255,1); border-radius:3px; padding:1px 1px; background:qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0,110,245,1), stop:0.8 rgba(0,110,245,1), stop:1 #9ACBF9);}");

    QVBoxLayout* deviceInfoBtnLayout = new QVBoxLayout;
    deviceInfoBtnLayout->setSpacing(1);
    deviceInfoBtnLayout->setContentsMargins(0, 5, 0, 8);
    deviceInfoBtnLayout->addWidget(deviceIcoWidget, 0, Qt::AlignHCenter);
    deviceInfoBtnLayout->addWidget(deviceNameTextLabel, 0, Qt::AlignHCenter);
    deviceInfoBtnLayout->addWidget(deviceSizeTextLabel, 0, Qt::AlignHCenter);
    deviceInfoBtnLayout->addWidget(deviceUsageBar, 0, Qt::AlignHCenter);
    QWidget* deviceInfoWidget = new QWidget;
    deviceInfoWidget->setObjectName("deviceInfoWidget");
    deviceInfoWidget->setLayout(deviceInfoBtnLayout);
    deviceInfoWidget->setFixedSize(120, 120);
    deviceInfoWidget->setStyleSheet("QWidget#deviceInfoWidget{ border:none; background-color:rgba(255, 255, 255, 0); }"\
                                    "QWidget#deviceInfoWidget:hover{background-color:rgba(194, 193, 195, 150); border:none; border-radius:15px; padding:2px 4px;}");

    QListWidgetItem* deviceInfoItem = new QListWidgetItem;
    deviceInfoItem->setSizeHint(QSize(deviceInfoWidget->width(), deviceInfoWidget->height()));
    deviceInfoItem->setData(Qt::UserRole, QStringList({devicePath, mountPoint}));

    m_deviceList->addItem(deviceInfoItem);
    m_deviceList->setItemWidget(deviceInfoItem, deviceInfoWidget);
    return deviceInfoItem;
}

// 设置选中样式
void SaveDeviceWidget::setItemSelectStyle(QListWidgetItem *item, bool isSelect)
{
    QWidget* deviceInfoWidget = m_deviceList->itemWidget(item);
    if (deviceInfoWidget == nullptr) {
        return;
    }

    QLabel* checkSelectIcoLabel = deviceInfoWidget->findChild<QLabel*>("checkSelectIco");
    QLabel* nameLabel = deviceInfoWidget->findChild<QLabel*>("nameLabel");
    QLabel* sizeLabel = deviceInfoWidget->findChild<QLabel*>("sizeLabel");

    if (isSelect) {
        if (checkSelectIcoLabel != nullptr) {
            checkSelectIcoLabel->setPixmap(QPixmap(":/images/select_white.svg"));
        }
        if (nameLabel != nullptr) {
            nameLabel->setStyleSheet("QLabel#nameLabel{ color:rgb(255,255,255); }");
        }
        if (sizeLabel != nullptr) {
            sizeLabel->setStyleSheet("QLabel#sizeLabel{ color:rgb(255,255,255); }");
        }
        deviceInfoWidget->setStyleSheet("QWidget#deviceInfoWidget{background-color:rgba(0,129,255,1); border:none; border-radius:15px; padding:2px 4px;}");
    } else {
        QLabel styleLabel;
        if (checkSelectIcoLabel != nullptr) {
            checkSelectIcoLabel->setPixmap(QPixmap());
        }
        if (nameLabel != nullptr) {
            nameLabel->setStyleSheet(styleLabel.styleSheet());
        }
        if (sizeLabel != nullptr) {
            sizeLabel->setStyleSheet(styleLabel.styleSheet());
        }
        deviceInfoWidget->setStyleSheet("QWidget#deviceInfoWidget{ border:none; background-color:rgba(255, 255, 255, 0); }"\
                                    "QWidget#deviceInfoWidget:hover{background-color:rgba(194, 193, 195, 150); border:none; border-radius:15px; padding:2px 4px;}");
    }
}

void SaveDeviceWidget::clearItemSelectStatus() {
    // 扫盘时设置所有项为不勾选状态
    for (int i = 0; i < m_deviceList->count(); i++) {
        setItemSelectStyle(m_deviceList->item(i), false);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        m_deviceList->item(i)->setSelected(false);
#else
        m_deviceList->setItemSelected(m_deviceList->item(i), false);
#endif
    }
    m_deviceList->setCurrentItem(nullptr);
    m_currSelectDeviceName.clear();
    // 禁用保存按钮
    m_saveBtn->setEnabled(false);
}

void SaveDeviceWidget::showEvent(QShowEvent *event)
{
    m_timer.start();
    DeviceManager::instance()->scanUSBDevices(true);
    return DDialog::showEvent(event);
}

void SaveDeviceWidget::hideEvent(QHideEvent *event)
{
    m_timer.stop();
    DeviceManager::instance()->scanUSBDevices(false);
    return DDialog::hideEvent(event);
}
