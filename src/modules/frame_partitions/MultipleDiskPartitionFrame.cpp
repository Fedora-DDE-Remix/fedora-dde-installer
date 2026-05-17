#include "MultipleDiskPartitionFrame.h"
#include "globals.h"
#include "TitleWidget.h"
#include "CommonFunc.h"
#include "PasswdDialog.h"
#include "SizeEditDialog.h"
#include "HelpDialog.h"
#include "WarningDialog.h"
#include "HelpLabel.h"
#include "DeviceManager.h"
#include "PartitionOperateManager.h"
#include "PartitionFullDiskManager.h"
#include "MultipleDiskItem.h"
#include "DeviceListView.h"
#include "FormatDiskTipsDialog.h"
#include "UserDataDialog.h"
#include "UseLvmLogicDialog.h"
#include "MultiDiskDelegate.h"
#include "PartitionDecryptDialog.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QApplication>
#include <QScroller>
#include <QScrollBar>
#include <DIconButton>
#include <DBlurEffectWidget>
#include <DFrame>
#include <QApplication>
#include <QScreen>
#include <QButtonGroup>
#include <DPaletteHelper>
#include <QScrollArea>
#include <QtMath>

Q_DECLARE_METATYPE(SDeviceInfo)

static const int CENTER_WIDTH = 600;

MultipleDiskPartitionFrame::MultipleDiskPartitionFrame(QWidget *parent):PartitionFrameBase(parent)
{
    // 标题栏
    m_titleWidget = new TitleWidget(this);
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->addWidget(m_titleWidget);

    m_sizeEditDialog = new SizeEditDialog;

    auto tipLayout = new QHBoxLayout;
    tipLayout->setAlignment(Qt::AlignCenter);
    tipLayout->addWidget(m_tipLabel);
    m_tipLabel = new DLabel(this);
    tipLayout->addWidget(m_tipLabel);
    m_tipLabel->setIndent(10);
    m_tipLabel->setAlignment(Qt::AlignCenter);
    m_tipLabel->setForegroundRole(DPalette::TextWarning);

    // 确认按钮
    m_nextBtn = new DSuggestButton;
    m_nextBtn->setFixedSize(gnextbuttonwidth, gnextbuttonheight);
    m_nextBtn->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_nextBtn->setEnabled(true);
    m_nextBtn->setDefault(true);
    QHBoxLayout *nextLayout = new QHBoxLayout;
    nextLayout->addStretch();
    nextLayout->addWidget(m_nextBtn);
    nextLayout->addStretch();

    connect(m_nextBtn, &DSuggestButton::clicked, this, &MultipleDiskPartitionFrame::onNextButtonClicked);

    // 主布局
    m_mainlayout = new QVBoxLayout;
    m_mainlayout->setContentsMargins(0, 0, 0, 0);
    m_mainlayout->setSpacing(0);
    m_mainlayout->addLayout(titleLayout);
    m_mainlayout->addSpacing(30);
    initContent();
    m_mainlayout->addSpacing(10);
    m_mainlayout->addLayout(tipLayout);
    m_mainlayout->addSpacing(10);
    m_mainlayout->addLayout(nextLayout);
    setLayout(m_mainlayout);
    initConnection();
}

void MultipleDiskPartitionFrame::initDeviceInfo(const QList<SDeviceInfo> &infos)
{
    m_sysMinSize = CommonFunc::getSettingsInt("DI_ROOTA_SIZE_CONFIG");
    m_devs = infos;
    for (int row = 0; row < m_devInfoListModel->rowCount(); ++row) {
        auto *item = m_devInfoListModel->item(row);
        QWidget *btn = item->data(Qt::UserRole + 4).value<QWidget*>();
        if(btn) {
            delete  btn;
        }
    }
    m_devInfoListModel->clear();
    m_dataDevCombo->clear();
    m_dataDevCombo->addItem(tr("Unselect"));
    //m_dataDevCombo->setEnabled(false);
    m_btnKeepData->setVisible(!CommonFunc::isGhost());
    if(m_modeGroup->checkedId() != -1) {
        int id = m_modeGroup->checkedId();
        m_modeGroup->setExclusive(false);
        m_modeGroup->button(id)->setChecked(false);
        m_modeGroup->setExclusive(true);
        m_modeGroup->button(1)->setEnabled(false);
        m_modeGroup->button(2)->setEnabled(false);
        m_modeGroup->button(3)->setEnabled(false);
        m_modeGroup->button(4)->setEnabled(false);
    }

    PartitionFullDiskManager::instance()->delSystemDev();
    PartitionFullDiskManager::instance()->delHomeDev();
    //
    DPalette palette = DPaletteHelper::instance()->palette(m_sysDevView);
    //按照m_devs的顺序添加系统盘和数据盘，两边顺序一致
    for (int nCount = 0; nCount < m_devs.count(); nCount++) {
        SDeviceInfo info = m_devs.at(nCount);
        double totalSize = CommonFunc::mToG(info.size);
        QString totalSizeString;
        if(totalSize < 1.0) {
            totalSizeString = QString::number(totalSize, 'f', 2);
        } else {
            totalSizeString = QString::number((int)totalSize);
        }

        QString availSize = totalSizeString; //暂时初始化为总容量
        //add system disk
        QStandardItem *item = new QStandardItem;
        item->setSizeHint(QSize(500, 76));
        item->setCheckable(true);
        item->setIcon(QIcon(":/images/drive-harddisk-small.svg"));
        item->setText(info.name);
        item->setToolTip(info.name);
        item->setData(totalSizeString, Qt::UserRole + 1);
        item->setData(0, Qt::UserRole + 2); //初始化为0
        item->setData(info.device, Qt::UserRole + 3);
        item->setData(palette.color(DPalette::ItemBackground), Qt::BackgroundRole);
        item->setData(palette.color(DPalette::ObviousBackground), Qt::UserRole + 110);
        QVariant var;
        DIconButton *btn = new DIconButton(m_sysDevView->viewport());
        btn->setFixedSize(24, 24);
        btn->setFlat(true);
        btn->setIcon(QIcon::fromTheme("edit"));
        btn->setIconSize(QSize(16, 16));
        btn->setVisible(false);
        var.setValue(btn);
        item->setData(var, Qt::UserRole + 4);
        connect(btn, &QAbstractButton::clicked, this, &MultipleDiskPartitionFrame::onEditSize);
        m_devInfoListModel->appendRow(item);
        //add data disk
        QString itemText = info.device;
        itemText.append(tr(" (Available/Total:%1/%2GiB)").arg(availSize).arg(totalSizeString));
        m_dataDevCombo->addItem(itemText);
        m_dataDevCombo->setItemData(m_dataDevCombo->count() - 1, info.device, Qt::UserRole + 1);
    }
}

void MultipleDiskPartitionFrame::initConnection()
{
    connect(m_dataDevCombo, QOverload<int>::of(&DComboBox::currentIndexChanged), [=](int index){
        QString err;
        PartitionFullDiskManager::instance()->delHomeDev(err);
        if(index > 0 && index != (m_sysDevView->currentIndex().row() + 1)) {
            PartitionFullDiskManager::instance()->addHomeDisk(m_devs[index - 1], err);
        }
    });

    auto *selectModel = m_sysDevView->selectionModel();
    connect(selectModel, &QItemSelectionModel::currentChanged, this, &MultipleDiskPartitionFrame::handleSystemDiskChange);
    //some connection
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_modeGroup, QOverload<int, bool>::of(&QButtonGroup::idToggled), [=](int id, bool checked){
#else
    connect(m_modeGroup, QOverload<int, bool>::of(&QButtonGroup::buttonToggled), [=](int id, bool checked){
#endif
        if(!checked) {
            return;
        }

        switch (id) {
        case 1:
            m_modeTip_1->setVisible(true);
            m_modeTip_2->setVisible(false);
            m_modeTip_3->setVisible(false);
            m_modeTip_4->setVisible(false);
            m_modeTip_5->setVisible(false);
            break;
        case 2:
        {
            m_modeTip_1->setVisible(false);
            m_modeTip_2->setVisible(true);
            m_modeTip_3->setVisible(false);
            m_modeTip_4->setVisible(false);
            m_modeTip_5->setVisible(false);
            PasswdDialog *dialog = new PasswdDialog(this);
            dialog->setAutoDecryptVisible(CommonFunc::isTPM());
            if (dialog->exec() == DDialog::Accepted) {
                m_fullEncryptionPwd = dialog->getPasswd();
                m_autoDecrypt = dialog->autoDecryptIsChecked();
                QTimer::singleShot(1, m_nextBtn, &QAbstractButton::click);
            } else {
                m_modeGroup->button(1)->setChecked(true);
            }
            setFocus();
        }
            break;
        case 3:
            m_modeTip_1->setVisible(false);
            m_modeTip_2->setVisible(false);
            m_modeTip_3->setVisible(true);
            m_modeTip_4->setVisible(false);
            m_modeTip_5->setVisible(false);
            break;
        case 4:
        {
            bool decryptStatus = false;
            bool needDecrypt = this->checkDiskNeedDecrypt(decryptStatus);
            if(needDecrypt && !decryptStatus) {
                m_modeGroup->button(1)->setChecked(true);
                return;
            }

            if(needDecrypt) {
                m_modeTip_1->setVisible(false);
                m_modeTip_2->setVisible(false);
                m_modeTip_3->setVisible(false);
                m_modeTip_4->setVisible(false);
                m_modeTip_5->setVisible(true);
            } else {
                m_modeTip_1->setVisible(false);
                m_modeTip_2->setVisible(false);
                m_modeTip_3->setVisible(false);
                m_modeTip_4->setVisible(true);
                m_modeTip_5->setVisible(false);
            }
        }
            break;
        default:
            break;
        }

        updateSystemDiskStatus();

        m_contentWidget->adjustSize();
    });
}

bool MultipleDiskPartitionFrame::checkDiskIsOK()
{
    bool bOK = true;
    m_tipLabel->setText("");
    auto sysIndex = m_sysDevView->currentIndex();
    int dataIndex = m_dataDevCombo->currentIndex();
    do
    {
        if(!sysIndex.isValid()) {
            //m_tipLabel->setText(tr("please select a disk first"));
            bOK = false;
            break;
        }

        //user select a data disk
        if(dataIndex > 0 && (sysIndex.row() + 1) != dataIndex) {
            // 双盘系统盘大小是49G，数据盘大小是最小15G
            //sys disk and data disk not the same
            qint64 systemMinSize = CommonFunc::getSettingsInt("DI_DEVICE_MIN_SIZE_CONFIG") * giDeviceUnitSize - CommonFunc::getSettingsInt("DI_PERSISTENT_SIZE_CONFIG");
            qint64 dataMinSize = CommonFunc::getSettingsInt("DI_PERSISTENT_SIZE_CONFIG");
            
            if(m_devs.at(sysIndex.row()).size < systemMinSize) {
                DDialog dialog;
                dialog.setWindowFlags(Qt::WindowTitleHint);
                dialog.setTitle(::QObject::tr("Insufficient disk space"));
                dialog.setMessage(QString(::QObject::tr("To install this operating system, the system partition requires at least %1 of disk space").arg(QString("%1GiB").arg(CommonFunc::mToG(systemMinSize)))));
                dialog.setWordWrapMessage(true);
                dialog.addButton(::QObject::tr("Cancel"), false);
                dialog.setCloseButtonVisible(true);
                dialog.move(qApp->primaryScreen()->availableSize().width() / 2 - dialog.width() / 2,
                            qApp->primaryScreen()->availableSize().height() / 2 - dialog.height() / 2);
                dialog.exec();
                bOK = false;
                break;
            } else if (m_devs.at(dataIndex - 1).size < dataMinSize) {
                DDialog dialog;
                dialog.setWindowFlags(Qt::WindowTitleHint);
                dialog.setTitle(::QObject::tr("Insufficient disk space"));
                dialog.setMessage(QString(::QObject::tr("To install this operating system, the data partition requires at least %1 of disk space").arg(QString("%1GiB").arg(CommonFunc::mToG(dataMinSize))))) ;
                dialog.setWordWrapMessage(true);
                dialog.addButton(::QObject::tr("Cancel"), false);
                dialog.setCloseButtonVisible(true);
                dialog.move(qApp->primaryScreen()->availableSize().width() / 2 - dialog.width() / 2,
                            qApp->primaryScreen()->availableSize().height() / 2 - dialog.height() / 2);
                dialog.exec();
                bOK = false;
                break;
            }
        } else {
            qint64 validDeviceMinSize = CommonFunc::getSettingsInt("DI_DEVICE_MIN_SIZE_CONFIG") * giDeviceUnitSize;
            if (m_devs.at(sysIndex.row()).size < validDeviceMinSize) {
                DDialog dialog;
                dialog.setWindowFlags(Qt::WindowTitleHint);
                dialog.setTitle(::QObject::tr("Insufficient disk space"));
                dialog.setMessage(::QObject::tr("At least %1 of disk space is required to install this operating system").arg(QString("%1GiB").arg(CommonFunc::mToG(validDeviceMinSize))));
                dialog.setWordWrapMessage(true);
                dialog.addButton(::QObject::tr("Cancel"), false);
                dialog.setCloseButtonVisible(true);
                dialog.move(qApp->primaryScreen()->availableSize().width() / 2 - dialog.width() / 2,
                            qApp->primaryScreen()->availableSize().height() / 2 - dialog.height() / 2);
                dialog.exec();
                bOK = false;
                break;
            }
        }
    }while (false);

    if(sysIndex.isValid()) {
        if (bOK) {
            auto device = m_devs.at(sysIndex.row());
            auto efiSize = CommonFunc::isUEFI() ? PartitionFullDiskManager::instance()->getConfigPartitionSize(device.size, gEFILabel) : 0;
            auto bootSize = PartitionFullDiskManager::instance()->getConfigPartitionSize(device.size, gBootLabel);
            auto rootSize = PartitionFullDiskManager::instance()->getConfigPartitionSize(device.size, gRootaLabel);
            auto swapSize = PartitionFullDiskManager::instance()->getConfigPartitionSize(device.size, gSWAPLabel);
            qint64 deviceMinSize = efiSize + bootSize + rootSize + swapSize;
            //大于2G调整才有意义
            sysIndex.data(Qt::UserRole + 4).value<QWidget*>()->setEnabled(device.size - deviceMinSize > 2 * 1024);
        }

    }

    return bOK;
}

bool MultipleDiskPartitionFrame::checkDiskNeedDecrypt(bool &decryptStatus)
{
    QList<SPartitionInfo> decryptList;
    SDeviceInfo sysDev = PartitionFullDiskManager::instance()->getSystemDev();
    SDeviceInfo homeDev = PartitionFullDiskManager::instance()->getHomeDev();
    PartitionFullDiskManager::instance()->getDeviceCryptPartitions(sysDev, decryptList);
    PartitionFullDiskManager::instance()->getDeviceCryptPartitions(homeDev, decryptList);
    if(!decryptList.isEmpty()) {
        decryptStatus = handleDecrpytDisk(decryptList);
    }

    return !decryptList.isEmpty();
}

bool MultipleDiskPartitionFrame::handleDecrpytDisk(const QList<SPartitionInfo> &partList)
{
    PartitionDecryptDialog decrypDlg(partList);
    if(QDialog::Accepted != decrypDlg.exec()) {
        return false;
    } else {
        Q_EMIT this->decryptSuccess();
        m_devs = PartitionOperateManager::instance()->getCurrentDevice();

        QString err;
        // 更新系统盘、数据盘
        SDeviceInfo testSystemDev = PartitionFullDiskManager::instance()->getSystemDev();
        if (!testSystemDev.device.isEmpty()) {
            auto systemDevIter = std::find_if(m_devs.begin(), m_devs.end(), [=](const SDeviceInfo& dev){ return dev.device == testSystemDev.device; });
            if (systemDevIter != m_devs.end()) {
                PartitionFullDiskManager::instance()->delSystemDev();
                PartitionFullDiskManager::instance()->addSystemDisk(*systemDevIter, err);
            }
        }

        SDeviceInfo testDataDev = PartitionFullDiskManager::instance()->getHomeDev();
        if (!testDataDev.device.isEmpty()) {
            auto dataDevIter = std::find_if(m_devs.begin(), m_devs.end(), [=](const SDeviceInfo& dev){ return dev.device == testDataDev.device; });
            if (dataDevIter != m_devs.end()) {
                PartitionFullDiskManager::instance()->delHomeDev(err);
                PartitionFullDiskManager::instance()->addHomeDisk(*dataDevIter, err);
            }
        }

        return true;
    }
}

void MultipleDiskPartitionFrame::adjustSystemSize(qint64 sysSize, bool bEnable)
{
    auto index = m_sysDevView->currentIndex();
    auto *item = m_devInfoListModel->itemFromIndex(index);
    item->setData((int)CommonFunc::mToG(sysSize), Qt::UserRole + 2);
    item->data(Qt::UserRole + 4).value<QWidget*>()->setEnabled(bEnable);
    m_sysDevView->update(item->index());
    PartitionFullDiskManager::instance()->resizeSystemPartition(sysSize - m_sysMinSize);
}

void MultipleDiskPartitionFrame::updateListState()
{
    this->setControlEnable();
}

void MultipleDiskPartitionFrame::itemClicked(const QModelIndex &currentIndex)
{
    SDeviceInfo devInfo = getItemDevInfo(currentIndex.row());
    if(!this->updateDiskInfo(devInfo)) {
        return;
    }

    SDeviceInfo system = PartitionFullDiskManager::instance()->getSystemDev();
    qint64 systemSize = PartitionFullDiskManager::instance()->getSystemSpaceSize();
    qint64 dataSize = PartitionFullDiskManager::instance()->getDataSpaceSize();
    qint64 dataMinSize = CommonFunc::getSettingsInt("DI_PERSISTENT_SIZE_CONFIG");

    if (!system.device.isEmpty()) {
        if (devInfo.size < dataMinSize) {
            SDeviceInfo data = PartitionFullDiskManager::instance()->getHomeDev();
            // 取消操作
            if (!data.device.isEmpty()) {
                QString err = "";
                PartitionFullDiskManager::instance()->delHomeDev(err);
            }
            this->updateTipInfo(QString("%1 %2GiB").arg(::QObject::tr("Less than the minimum data space requirement")).arg(dataMinSize / gDeviceUnitSize));
            this->setControlEnable();
            return;
        } else {
            this->updateTipInfo("");
        }
    }

    m_sizeEditDialog->setValueRange(systemSize, dataSize + systemSize, dataMinSize);
    m_sizeEditDialog->setDefaultSize(systemSize);

    disconnect(m_editbtn, &DIconButton::clicked, nullptr, nullptr);
    connect(m_editbtn, &DIconButton::clicked, this, [=] {
        qint64 systemSize = PartitionFullDiskManager::instance()->getSystemSpaceSize();
        qint64 dataSize = PartitionFullDiskManager::instance()->getDataSpaceSize();
        m_sizeEditDialog->setDefaultSize(systemSize);
        m_sizeEditDialog->setDataSpaceSize(dataSize);
        if (m_sizeEditDialog->exec() == DDialog::Accepted) {
            PartitionFullDiskManager::instance()->resizeSystemPartition(m_sizeEditDialog->getSize());
        }
    });

    this->updateListState();
}

void MultipleDiskPartitionFrame::showAutoSelectDialog() {
    QMap<SDeviceInfo, QList<SPartitionInfo>> deviceInfos = DeviceManager::instance()->getDeviceInfos();
    if (deviceInfos.size() == 0) {
        return;
    }

    // 判断如果已经选择了磁盘，就不需要弹窗提示了
    if ((!PartitionFullDiskManager::instance()->getSystemDev().device.isEmpty()) ||
        (!PartitionFullDiskManager::instance()->getHomeDev().device.isEmpty())) {
        return;
    }

    // 双盘场景选中磁盘并弹窗提示，单盘直接选中不提示
    QString err;
    PartitionOperateManager::instance()->setDeviceInfo(deviceInfos);
    QList<SDeviceInfo> devices = PartitionOperateManager::instance()->getCurrentDevice();
    QList<SDeviceInfo> sortDevices = CommonFunc::sortDevice(devices);
    SDeviceInfo firstDisk, secondDisk;

    if (sortDevices.size() < 1) {
        qWarning() << "no device find to auto select !";

    } else if (sortDevices.size() >= 2) {
        qint64 deviceMinSize = CommonFunc::getSettingsInt("DI_DEVICE_MIN_SIZE_CONFIG");
        // 查找符合条件的nvme磁盘
        auto nvmeDisk = std::find_if(sortDevices.begin(), sortDevices.end(), [=](SDeviceInfo infoItem) {
            if (infoItem.transporttype.contains("nvme")) {
                return infoItem.size >= CommonFunc::gToM(deviceMinSize);
            }
            return false;
        });

        // 查找符合条件的数据盘
        QList<SDeviceInfo>::iterator normalDisk = sortDevices.begin();
        for (; normalDisk != sortDevices.end(); ++normalDisk) {
            if ((*normalDisk).transporttype.contains("sata") && (nvmeDisk != sortDevices.end())) {
                qint64 dataMinSize = CommonFunc::getSettingsInt("DI_PERSISTENT_SIZE_CONFIG");
                if ((*normalDisk).size >= dataMinSize) {
                    break;
                }
            }
        }

        if (nvmeDisk != sortDevices.end()) {
            firstDisk = *nvmeDisk;
            if (normalDisk != sortDevices.end()) { // 存在两个磁盘空间都满足要求的情况下，触发弹窗提示
                secondDisk = *normalDisk;

                DDialog *dialog = new DDialog(this);
                dialog->setOnButtonClickedClose(true);
                dialog->setIcon(QIcon::fromTheme("dialog-warning"));
                dialog->setMessage(::QObject::tr("The \"%1\" disk will be the system disk and the \"%2\" disk will be data disk by default.")
                                          .arg(firstDisk.device, secondDisk.device));
                dialog->addButton(::QObject::tr("Cancel"));
                dialog->addButton(::QObject::tr("Confirm"), true, DDialog::ButtonRecommend);
                connect(dialog, &DDialog::buttonClicked, [=](int index, const QString &text) {
                    Q_UNUSED(text);
                    if (index == 1) {
                        for (int i = 0; i < m_devInfoListModel->rowCount(); i++) {
                            SDeviceInfo devItem;
                            devItem.device = m_devInfoListModel->item(i)->data(Qt::UserRole + 3).toString();
                            if (devItem.device == firstDisk.device) {
                                m_sysDevView->setCurrentIndex(m_devInfoListModel->index(i, 0));
                                qWarning() << "set disk:" << firstDisk.device << "as system disk !";
                            }
                        }

                        for (int j = 0; j < m_dataDevCombo->count(); ++j) {
                            QString device = m_dataDevCombo->itemData(j, Qt::UserRole + 1).toString();
                            if (device == secondDisk.device) {
                                m_dataDevCombo->setCurrentIndex(j);
                                qWarning() << "set disk:" << secondDisk.device << "as data disk !";
                            }
                        }
                    }
                });
                connect(dialog, &DDialog::visibleChanged, [=](bool visilble){
                    if(visilble) {
                        dialog->moveToCenter();
                    }
                });
                dialog->open();
            } else { // 只有一个磁盘空间满足要求的情况下，不触发弹窗提示，默认选中该磁盘
                for (int i = 0; i < m_devInfoListModel->rowCount(); i++) {
                    SDeviceInfo devItem;
                    devItem.device = m_devInfoListModel->item(i)->data(Qt::UserRole + 3).toString();
                    if (devItem.device == firstDisk.device) {
                        m_sysDevView->setCurrentIndex(m_devInfoListModel->index(i, 0));
                        qWarning() << "set disk : " << firstDisk.device << "as system disk !";
                        break;
                    }
                }
            }
        }
    }
}

void MultipleDiskPartitionFrame::onEditSize()
{
    auto index = m_sysDevView->currentIndex();
    auto *item = m_devInfoListModel->itemFromIndex(index);
    MultidiskSizeDialog sizeDlg;
    int totalSize = item->data(Qt::UserRole + 1).toInt();
    int sysSize = item->data(Qt::UserRole + 2).toInt();
    int otherSize = (int)CommonFunc::mToG(PartitionFullDiskManager::instance()->getOtherPartitionSize(
                                             CommonFunc::gToM((qint64)totalSize)));
    sizeDlg.initDiskSize(totalSize, (int)CommonFunc::mToG(m_sysMinSize), sysSize, otherSize);
    if(QDialog::Accepted == sizeDlg.exec()) {
        int sysSize = sizeDlg.getSysDiskSize();
        item->setData(sysSize, Qt::UserRole + 2);
        m_sysDevView->update(item->index());
        PartitionFullDiskManager::instance()->resizeSystemPartition(CommonFunc::gToM((qint64)sysSize) - m_sysMinSize);
    }
}

void MultipleDiskPartitionFrame::onNextButtonClicked()
{
    CommonFunc::setSettingsValue("DI_OLD_USERNAME_LIST", "");
    if (!checkDiskIsOK()) {
        return;
    }

    QString err;
    //检查保留用户数据
    int checkId = m_modeGroup->checkedId();
    PartitionFullDiskManager::instance()->setPartitionMode(FullDiskMode);
    PartitionFullDiskManager::instance()->resetDisk();
    auto sysDev = PartitionFullDiskManager::instance()->getSystemDev();
    auto homeDev = PartitionFullDiskManager::instance()->getHomeDev();
    sysDev.isCrypt = m_btnEncrypt->isChecked();
    homeDev.isCrypt = m_btnEncrypt->isChecked();
    if(sysDev == homeDev) {
        //system disk an data disk is the same
        PartitionFullDiskManager::instance()->delHomeDev();
        PartitionFullDiskManager::instance()->addSystemDisk(sysDev, err);
    } else {
        PartitionFullDiskManager::instance()->addSystemDisk(sysDev, err);
        PartitionFullDiskManager::instance()->addHomeDisk(homeDev, err);
    }

    if (checkId == 4) {
        QString oldVersion;
        auto errType = PartitionFullDiskManager::instance()->checkSaveUserData(oldVersion);
        if (errType != KeepDataError::NO_ERROR) {
            UserDataDialog userDlg(errType, oldVersion);
            userDlg.exec();

            // 设置默认安装模式
            m_modeGroup->button(1)->setChecked(true);
            checkId = m_modeGroup->checkedId();

            if (userDlg.getBtnSelectState() != BtnSelectState::confirm) {
                return;
            }
        }
    }

    if(CommonFunc::isGhost()) {
        if(!PartitionFullDiskManager::instance()->checkGhostModeSpace(err)) {
            m_tipLabel->setText(err);
            return;
        }

        QString ghostDevPath = CommonFunc::getSettingsString("DI_UIMG_DEVICE");
        auto ghostSysDev = PartitionFullDiskManager::instance()->getSystemDev();
        auto ghosthomeDev = PartitionFullDiskManager::instance()->getHomeDev();
        if(!ghostDevPath.isEmpty() && (ghostDevPath.startsWith(ghostSysDev.device) ||
            (!ghosthomeDev.device.isEmpty() && ghostDevPath.startsWith(ghosthomeDev.device)))) {
            m_tipLabel->setText(tr("Cannot select the disk contains the image."));
            return;
        }
    }

    if(checkId == 4) {
        //Keep user data
        PartitionFullDiskManager::instance()->setPartitionMode(SaveData);
        PartitionFullDiskManager::instance()->saveUserData();
    } else if (checkId == 2) {
        //写加密密码到配置文件
        CommonFunc::setSettingsValue("DI_CRYPT_PASSWORD", m_fullEncryptionPwd);
        CommonFunc::setSettingsValue("DI_IS_AUTO_DECRYPT", m_autoDecrypt);
        PartitionFullDiskManager::instance()->setPartitionMode(Cryptsetup);
        PartitionFullDiskManager::instance()->addFullDiskLVM(gCryptoLuksFileSystem);
    } else if (checkId == 3) {
        //写使用lvm逻辑分区标志字段
        PartitionFullDiskManager::instance()->setPartitionMode(LvmFullDisk);
        PartitionFullDiskManager::instance()->addFullDiskLVM(gLVM2PVFileSystem);
    }

    Q_EMIT next();
}

void MultipleDiskPartitionFrame::handleSystemDiskChange(const QModelIndex &current, const QModelIndex &previous)
{
    if(!previous.isValid() && current.isValid()) {
        m_sysDevView->selectionModel()->select(current, QItemSelectionModel::SelectCurrent);
    }

    if(m_modeGroup->checkedId() == -1) {
        m_modeGroup->button(1)->setEnabled(true);
        m_modeGroup->button(2)->setEnabled(true);
        m_modeGroup->button(3)->setEnabled(true);
        m_modeGroup->button(4)->setEnabled(true);
        m_modeGroup->button(1)->setChecked(true);
    }

    //如果系统盘和数据盘不一致，则添加数据盘
    if((m_dataDevCombo->currentIndex() > 0) && (current.row() != (m_dataDevCombo->currentIndex() - 1))) {
        QString err;
        if (!PartitionFullDiskManager::instance()->addHomeDisk(m_devs[m_dataDevCombo->currentIndex() - 1], err)) {
            qWarning() << "add home disk failed: " << err;
        }
    }

    QString err;
    const SDeviceInfo &sysDev = m_devs.at(current.row());
    PartitionFullDiskManager::instance()->delSystemDev();
    PartitionFullDiskManager::instance()->addSystemDisk(sysDev, err);
    updateSystemDiskStatus();
}

void MultipleDiskPartitionFrame::updateSystemDiskStatus()
{
    auto index = m_sysDevView->currentIndex();
    if (!index.isValid()) {
        return;
    }
    qint64 totalSize = m_devs.at(index.row()).size;

    int sysShowMinSize = m_sysMinSize;

    //大于225G的磁盘取值为磁盘容量的10%
    if (totalSize > gDiskThreshold) {
        sysShowMinSize = qFloor(totalSize * 0.1);
    }

    // 如果选择的是保留用户数据模式，则获取之前roota的大小，用于在界面显示
    if (m_modeGroup->checkedId() == 4) {
        auto devInfos = PartitionOperateManager::instance()->getAvailableDeviceInfo();
        for (SDeviceInfo dev : devInfos.keys()) {
            if (dev.device != m_devs.at(index.row()).device) {
                continue;
            }
            for (SPartitionInfo in : devInfos.value(dev)) {
                if (in.label == "Roota") {
                    sysShowMinSize = in.size;
                    break;
                }
            }
        }
    }

    adjustSystemSize(sysShowMinSize, m_modeGroup->checkedId() != 4);
}

void MultipleDiskPartitionFrame::initContent()
{
    m_devInfoListModel = new QStandardItemModel;
    //
    m_labelSys = new DLabel;
    m_labelSys->setFixedWidth(CENTER_WIDTH);
    m_labelSys->setIndent(10);
    //disk listview
    m_sysDevView = new QListView;
    m_sysDevView->setFocusPolicy(Qt::TabFocus);
    m_sysDevView->setStyleSheet(""
                       "QWidget:focus {"
                       "border: 2px solid;"
                       "border-radius:5px;"
                       "border-color:rgb(1, 128, 255)"
                       "}");
    m_sysDevView->setFixedSize(CENTER_WIDTH + 10, 170);
    m_sysDevView->setSpacing(5);
    m_sysDevView->setIconSize(QSize(56, 56));
    m_sysDevView->setFrameShape(QFrame::NoFrame);
    m_sysDevView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_sysDevView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_sysDevView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_sysDevView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_sysDevView->viewport()->setAutoFillBackground(false);
    m_sysDevView->setAutoFillBackground(false);
    m_sysDevView->setBackgroundRole(m_sysDevView->viewport()->backgroundRole());
    m_sysDevView->setModel(m_devInfoListModel);
    m_sysDevView->setItemDelegate(new MultiDiskDelegate);
    //data disk
    DFrame *dataFrame = new DFrame;
    dataFrame->setFixedSize(CENTER_WIDTH, 60);
    dataFrame->setLineWidth(0);
    dataFrame->setBackgroundRole(DPalette::ItemBackground);
    QHBoxLayout *dataLayout = new QHBoxLayout;
    dataLayout->setContentsMargins(10, 0, 10, 0);
    dataFrame->setLayout(dataLayout);
    m_dataLabel = new DLabel;
    dataLayout->addWidget(m_dataLabel, 0, Qt::AlignLeft);
    m_dataDevCombo = new DComboBox;
    m_dataDevCombo->setMinimumSize(300, 36);
    dataLayout->addWidget(m_dataDevCombo, 0, Qt::AlignRight);
    //
    m_labelMode = new DLabel;
    m_labelMode->setFixedWidth(CENTER_WIDTH);
    m_labelMode->setIndent(10);
    DFrame *modeFrame = new DFrame;
    modeFrame->setFixedWidth(CENTER_WIDTH);
    modeFrame->setMinimumHeight(60);
    modeFrame->setLineWidth(0);
    modeFrame->setBackgroundRole(DPalette::ItemBackground);
    QHBoxLayout *modeLayout = new QHBoxLayout;
    modeLayout->setContentsMargins(20, 0, 20, 0);
    modeLayout->setSpacing(0);
    modeFrame->setLayout(modeLayout);
    m_btnDefault = new HelpRadioButton;
    m_btnEncrypt = new HelpRadioButton;
    m_btnExtand = new HelpRadioButton;
    m_btnKeepData = new HelpRadioButton;
    m_btnDefault->setEnabled(false);
    m_btnEncrypt->setEnabled(false);
    m_btnExtand->setEnabled(false);
    m_btnKeepData->setEnabled(false);
    m_btnDefault->setFixedWidth(120);
    m_btnEncrypt->setFixedWidth(120);
    m_btnExtand->setFixedWidth(160);
    m_btnKeepData->setFixedWidth(160);
    m_modeGroup = new QButtonGroup;
    m_modeGroup->addButton(m_btnDefault, 1);
    m_modeGroup->addButton(m_btnEncrypt, 2);
    m_modeGroup->addButton(m_btnExtand, 3);
    m_modeGroup->addButton(m_btnKeepData, 4);

    modeLayout->addWidget(m_btnDefault, 0, Qt::AlignLeft);
    modeLayout->addWidget(m_btnEncrypt);
    modeLayout->addWidget(m_btnExtand);
    modeLayout->addWidget(m_btnKeepData, 0, Qt::AlignRight);
    m_modeTip_1 = new DTipLabel;
    m_modeTip_1->setAlignment(Qt::AlignLeft);
    m_modeTip_1->setFixedWidth(CENTER_WIDTH);
    m_modeTip_1->setMinimumHeight(40);
    m_modeTip_1->setIndent(10);
    m_modeTip_1->setWordWrap(true);
    m_modeTip_1->setVisible(false);

    m_modeTip_2 = new DTipLabel;
    m_modeTip_2->setAlignment(Qt::AlignLeft);
    m_modeTip_2->setFixedWidth(CENTER_WIDTH);
    m_modeTip_2->setMinimumHeight(40);
    m_modeTip_2->setIndent(10);
    m_modeTip_2->setWordWrap(true);
    m_modeTip_2->setVisible(false);

    m_modeTip_3 = new DTipLabel;
    m_modeTip_3->setAlignment(Qt::AlignLeft);
    m_modeTip_3->setFixedWidth(CENTER_WIDTH);
    m_modeTip_3->setMinimumHeight(40);
    m_modeTip_3->setIndent(10);
    m_modeTip_3->setWordWrap(true);
    m_modeTip_3->setVisible(false);

    m_modeTip_4 = new DTipLabel;
    m_modeTip_4->setAlignment(Qt::AlignLeft);
    m_modeTip_4->setFixedWidth(CENTER_WIDTH);
    m_modeTip_4->setMinimumHeight(40);
    m_modeTip_4->setIndent(10);
    m_modeTip_4->setWordWrap(true);
    m_modeTip_4->setVisible(false);

    m_modeTip_5 = new DTipLabel;
    m_modeTip_5->setAlignment(Qt::AlignLeft);
    m_modeTip_5->setFixedWidth(CENTER_WIDTH);
    m_modeTip_5->setMinimumHeight(40);
    m_modeTip_5->setIndent(10);
    m_modeTip_5->setWordWrap(true);
    m_modeTip_5->setVisible(false);

    QVBoxLayout *subContent = new QVBoxLayout;
    subContent->setContentsMargins(0, 0, 0, 0);
    subContent->setSpacing(0);
    subContent->addWidget(m_labelSys, 0, Qt::AlignHCenter);
    subContent->addSpacing(10);
    subContent->addWidget(m_sysDevView, 0, Qt::AlignHCenter);
    subContent->addSpacing(20);
    subContent->addWidget(dataFrame, 0, Qt::AlignHCenter);
    subContent->addSpacing(20);
    subContent->addWidget(m_labelMode, 0, Qt::AlignHCenter);
    subContent->addSpacing(10);
    subContent->addWidget(modeFrame, 0, Qt::AlignHCenter);
    subContent->addSpacing(10);
    subContent->addWidget(m_modeTip_1, 0, Qt::AlignHCenter);
    subContent->addWidget(m_modeTip_2, 0, Qt::AlignHCenter);
    subContent->addWidget(m_modeTip_3, 0, Qt::AlignHCenter);
    subContent->addWidget(m_modeTip_4, 0, Qt::AlignHCenter);
    subContent->addWidget(m_modeTip_5, 0, Qt::AlignHCenter);
    subContent->addStretch();

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setMaximumWidth(CENTER_WIDTH + 20);
    m_contentWidget = new QWidget;
    m_contentWidget->setLayout(subContent);
    scrollArea->setWidget(m_contentWidget);
    m_mainlayout->addWidget(scrollArea, 0, Qt::AlignHCenter);
}

void MultipleDiskPartitionFrame::setControlEnable()
{
    SDeviceInfo system = PartitionFullDiskManager::instance()->getSystemDev();
    SDeviceInfo home = PartitionFullDiskManager::instance()->getHomeDev();


    m_nextBtn->setEnabled(false);

    if (!system.device.isEmpty()) {

        m_nextBtn->setEnabled(true);

//        if (m_keepUserData->isChecked()) {
//            m_keepUserData->setChecked(PartitionFullDiskManager::instance()->checkSaveUserData());
//        } else {
//            if (data.device.isEmpty()) {
//                m_editbtn->setEnabled(true);
//            }
        }
//    } else {
//        m_fullEncryption->setChecked(false);
//        m_useLvmLogical->setChecked(false);
//        m_keepUserData->setChecked(false);
//    }
}

bool MultipleDiskPartitionFrame::updateDiskInfo(const SDeviceInfo &info)
{
    return true;
}

void MultipleDiskPartitionFrame::updateDiskInfo(const SDeviceInfo &info, EDevType type)
{
}

void MultipleDiskPartitionFrame::updateTipInfo(const QString &text)
{
    if (!text.isEmpty()) {
        // 错误显示
        m_tipLabel->setText(text);
        m_nextBtn->setEnabled(false);
    } else {
        m_nextBtn->setEnabled(true);
        m_tipLabel->setStyleSheet("");
        m_tipLabel->setText("");
    }
}

bool MultipleDiskPartitionFrame::checkSysDiskSpace(const SDeviceInfo &info)
{
    //校验系统分区大小
    auto efiSize = CommonFunc::isUEFI() ? PartitionFullDiskManager::instance()->getConfigPartitionSize(info.size, gEFILabel) : 0;
    auto bootSize = PartitionFullDiskManager::instance()->getConfigPartitionSize(info.size, gBootLabel);
    auto rootSize = PartitionFullDiskManager::instance()->getConfigPartitionSize(info.size, gRootaLabel);
    auto swapSize = PartitionFullDiskManager::instance()->getConfigPartitionSize(info.size, gSWAPLabel);
    qint64 deviceMinSize = efiSize + bootSize + rootSize + swapSize;

    if (info.size < deviceMinSize) {
        DDialog dialog;
        dialog.setWindowFlags(Qt::WindowTitleHint);
        dialog.setTitle(::QObject::tr("Insufficient disk space"));
        dialog.setMessage(QString(::QObject::tr("To install this operating system, the system partition requires "
                                                "at least %1 of disk space").arg(QString("%1GiB").arg(CommonFunc::mToG(deviceMinSize)))));
        dialog.setWordWrapMessage(true);
        dialog.addButton(::QObject::tr("Cancel", "Button"), false);
        dialog.setCloseButtonVisible(true);
        dialog.move(qApp->primaryScreen()->availableSize().width() / 2 - dialog.width() / 2,
                    qApp->primaryScreen()->availableSize().height() / 2 - dialog.height() / 2);
        dialog.exec();

        return false;
    } else {
        return true;
    }
}

MultipleDiskItem *MultipleDiskPartitionFrame::getItemInfo(int row)
{
    DStandardItem* item = static_cast<DStandardItem*>(m_devInfoListModel->item(row));
    DViewItemActionList actionList= item->actionList(Qt::Edge::LeftEdge);
    return static_cast<MultipleDiskItem*>(actionList.last()->widget());
}

SDeviceInfo MultipleDiskPartitionFrame::getItemDevInfo(int row)
{
    DStandardItem* item = static_cast<DStandardItem*>(m_devInfoListModel->item(row));
    return item->data().value<SDeviceInfo>();
}

void MultipleDiskPartitionFrame::updateTs()
{
        m_titleWidget->setTitleLabelProperty(::QObject::tr("Full Disk"), "");
        m_titleWidget->setTipsLabelProperty(::QObject::tr("The system will be installed on the disk you selected. All data on the selected disk (including system and data partitions) will be formatted."), "font-size:16px;font-weight:Medium;");

        m_labelSys->setText(tr("Select system disk"));
        m_dataLabel->setText(tr("Select data disk"));
        m_labelMode->setText(tr("select a install mode"));

        int spacinValue = m_btnDefault->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
        m_btnDefault->setText(CommonFunc::wrapWords(tr("Default"), m_btnDefault->width() - m_btnDefault->iconSize().width() - spacinValue, m_btnDefault->fontMetrics()));
        m_btnEncrypt->setText(CommonFunc::wrapWords(tr("Encrypt disk"), m_btnEncrypt->width() - m_btnEncrypt->iconSize().width() - spacinValue, m_btnEncrypt->fontMetrics()));
        m_btnExtand->setText(CommonFunc::wrapWords(tr("Enable Volume Extension"), m_btnExtand->width() - m_btnExtand->iconSize().width() - spacinValue, m_btnExtand->fontMetrics()));
        m_btnKeepData->setText(CommonFunc::wrapWords(tr("keep user data"), m_btnKeepData->width() - m_btnKeepData->iconSize().width() - spacinValue, m_btnKeepData->fontMetrics()));

        m_modeTip_1->setText(::QObject::tr("Automatic partition to install %1, it will format the selected disk, please backup your data.").arg(CommonFunc::getCurrentOs() == OSType::Community ? "deepin" : "UOS"));
        m_modeTip_1->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        m_modeTip_1->adjustSize();
        m_modeTip_2->setText(::QObject::tr("Encrypt the whole disk to prevent data theft."));        
        m_modeTip_2->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        m_modeTip_2->adjustSize();
        m_modeTip_3->setText(::QObject::tr("By enabling it, LVM will be used to manage partitions for your system "
                                           "so that you can expand partitions with disk management tools, but may affect disk performance."));
        m_modeTip_3->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        m_modeTip_3->adjustSize();
        m_modeTip_4->setText(::QObject::tr("If you have previously installed %1 using the same boot mode "
                                           "as this installation configuration, then you can keep your data (/data/home/).").arg(CommonFunc::getCurrentOs() == OSType::Community ? "deepin" : "UOS"));
        m_modeTip_4->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        m_modeTip_4->adjustSize();
        m_modeTip_5->setText(::QObject::tr("User data will be saved during system installation, and the disk encryption password will remain the same"));
        m_modeTip_5->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        m_modeTip_5->adjustSize();

        m_tipLabel->setText("");
        m_nextBtn->setText(::QObject::tr("Next"));
        m_sizeEditDialog->updateTs();
        m_btnDefault->addHelpText(tr("Default"), m_modeTip_1->text());
        m_btnEncrypt->addHelpText(tr("Encrypt disk"), m_modeTip_2->text());
        m_btnExtand->addHelpText(tr("Enable Volume Extension"), m_modeTip_3->text());
        m_btnKeepData->addHelpText(tr("keep user data"), m_modeTip_4->text());

        for (int index = 0; index < m_dataDevCombo->count(); ++index) {
            if (index == 0) {
                m_dataDevCombo->setItemText(index, tr("Unselect"));
                continue;
            }
            auto itemData = m_dataDevCombo->itemData(index, Qt::UserRole + 1).toString();
            auto it = std::find_if(m_devs.begin(), m_devs.end(), [itemData](const SDeviceInfo &dev) {
                return dev.device == itemData;
            });

            if (it != m_devs.end()) {
                SDeviceInfo dev = *it;
                double totalSize = CommonFunc::mToG(dev.size);
                QString totalSizeString;
                if (totalSize < 1.0) {
                    totalSizeString = QString::number(totalSize, 'f', 2);
                } else {
                    totalSizeString = QString::number((int) totalSize);
                }

                QString availSize = totalSizeString;
                //add data disk
                QString itemText = dev.device;
                itemText.append(tr(" (Available/Total:%1/%2GiB)").arg(availSize).arg(totalSizeString));
                m_dataDevCombo->setItemText(index, itemText);
            }
        }

    m_contentWidget->adjustSize();
}

void MultipleDiskPartitionFrame::showEvent(QShowEvent *event)
{
    if (CommonFunc::getSettingsBool("DI_ENABLE_AUTO_SELECT_DISK")) {
        showAutoSelectDialog();
    }
    updateSystemDiskStatus();
    setFocus();
    QWidget::showEvent(event);
}

