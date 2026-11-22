#include "SeniorPartitionFrame.h"
#include "TitleWidget.h"
#include "DiskinfoWidget.h"
#include "globals.h"
#include "CommonFunc.h"
#include "SeniorPartationListView.h"
#include "CreateNewPartationDialog.h"
#include "CreateNewLVMDialog.h"
#include "PartitionSeniorManager.h"
#include "PartitionOperateManager.h"
#include "EditPartitionDialog.h"
#include "FormatDiskTipsDialog.h"
#include "EditLVMDialog.h"
#include "ResetListDialog.h"
#include "CreateSwapDialog.h"
#include "HelpLabel.h"
#include "HelpDialog.h"
#include "PasswdDialog.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QScreen>

SeniorPartitionFrame::SeniorPartitionFrame(QWidget *parent):PartitionFrameBase(parent)
{
    // 标题栏
    m_titleWidget = new TitleWidget(this);
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->addWidget(m_titleWidget);
    //
    m_centerWidget = new QStackedWidget;
    //列表
    m_listReset = new QPushButton(this);
    m_listReset->setMinimumSize(130, 36);
    m_listReset->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_listReset->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    connect(m_listReset, &QPushButton::clicked, this, [=] {
        ResetListDialog dialog;
        if (dialog.exec() == DDialog::Accepted) {
            PartitionSeniorManager::instance()->resetDisk();
            PartitionSeniorManager::instance()->clearCryptList();
            updateInstallInfo(PartitionSeniorManager::instance()->getSystemPartitionInfo(),
                              PartitionSeniorManager::instance()->getSystemLVMInfo());
            this->updateTips("");
        }
        setFocus();
    }, Qt::QueuedConnection);

    m_lvmCreate = new QPushButton(this);
    m_lvmCreate->hide();
    m_lvmCreate->setEnabled(false);
    m_lvmCreate->setFixedSize(130, 36);
    m_lvmCreate->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    m_lvmFrame = new SeniorLVMFrame;
    connect(m_lvmFrame, &SeniorLVMFrame::back, this, [=]{
        m_centerWidget->setCurrentIndex(0);
    });
    connect(m_lvmFrame, &SeniorLVMFrame::createLVM, this, [=]{
        QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionSeniorManager::instance()->getSystemPartitionInfo();
        QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionSeniorManager::instance()->getSystemLVMInfo();
        this->updateInstallInfo(devs, vgs);
        m_centerWidget->setCurrentIndex(0);
    });
    connect(m_lvmCreate, &QPushButton::clicked, this, [=] {
        QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionSeniorManager::instance()->getSystemPartitionInfo();
        QList<SPartitionInfo> vgInfoList = PartitionSeniorManager::instance()->getPartitionListByFileSystem(gLVM2PVFileSystem);
        m_lvmFrame->setDevData(devs);
        if (!vgInfoList.size()) {
            this->updateTips(::QObject::tr("Please create partitions with the lvm file system first, and then create a logical volume group."));
            return;
        }

        // 根据引导模式，查找是否存在/boot、/等分区
        bool isExistBoot  = false;
        bool isExistRoot = false;
        for (auto devParts : devs) {
            isExistBoot = isExistBoot ? true : std::find_if(devParts.begin(), devParts.end(), [=](SPartitionInfo tmpPart) {
                return !tmpPart.mountPoint.compare("/boot");
            }) != devParts.end();

            isExistRoot = isExistRoot ? true : std::find_if(devParts.begin(), devParts.end(), [=](SPartitionInfo tmpPart) {
                return !tmpPart.mountPoint.compare("/");
            }) != devParts.end();
        }
        if (!CommonFunc::isUEFI()) {
            if ((!isExistBoot) && (!isExistRoot)) {
                this->updateTips(QString(::QObject::tr("To create lvm, %1 partition is required")).arg("/boot、/"));
                return;
            }
        }

        m_centerWidget->setCurrentIndex(1);
    });
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(0);
    btnLayout->addStretch();
    btnLayout->addWidget(m_listReset);
    btnLayout->addSpacing(gitemSpacing);
    btnLayout->addWidget(m_lvmCreate);
    btnLayout->addSpacing(gitemSpacing);

    m_tableView = new SeniorPartationListView(this);
    m_tableView->setFixedWidth(CommonFunc::isBigScreen() ? 980 : 680);
    m_model = new ItemModel;
    m_tableView->setModel(m_model);
    QHeaderView *pHeaderView = m_tableView->header();
    pHeaderView->setSectionResizeMode(QHeaderView::Stretch);
    pHeaderView->setSectionResizeMode(0, QHeaderView::Fixed);
    pHeaderView->setSectionResizeMode(1, QHeaderView::Fixed);
    pHeaderView->setSectionResizeMode(2, QHeaderView::Fixed);
    pHeaderView->setSectionResizeMode(3, QHeaderView::Fixed);
    pHeaderView->setSectionResizeMode(4, QHeaderView::Fixed);
    pHeaderView->setDefaultSectionSize(36);

    m_tableView->setColumnWidth(0, CommonFunc::isBigScreen() ? seniorPartationInstall::kColumnWidth_0 : seniorPartationInstall::kColumnWidthSmall_0);
    m_tableView->setColumnWidth(1, CommonFunc::isBigScreen() ? seniorPartationInstall::kColumnWidth_1 : seniorPartationInstall::kColumnWidthSmall_1);
    m_tableView->setColumnWidth(2, CommonFunc::isBigScreen() ? seniorPartationInstall::kColumnWidth_2 : seniorPartationInstall::kColumnWidthSmall_2);
    m_tableView->setColumnWidth(3, CommonFunc::isBigScreen() ? seniorPartationInstall::kColumnWidth_3 : seniorPartationInstall::kColumnWidthSmall_3);
    m_tableView->setColumnWidth(4, CommonFunc::isBigScreen() ? seniorPartationInstall::kColumnWidth_4 : seniorPartationInstall::kColumnWidthSmall_4);
    QHBoxLayout *viewLayout = new QHBoxLayout;
    viewLayout->setContentsMargins(0, 0, 0, 0);
    viewLayout->addStretch();
    viewLayout->addWidget(m_tableView);
    viewLayout->addStretch();

    QVBoxLayout *listLayout = new QVBoxLayout;
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(0);
    listLayout->addSpacing(glayoutSpacing);
    listLayout->addLayout(btnLayout);
    listLayout->addSpacing(glayoutSpacing);
    listLayout->addLayout(viewLayout);
    listLayout->addSpacing(glayoutSpacing);

    connect(m_model, &ItemModel::signalPartitionAdd, this, [=](const SPartitionInfo &partinfo) {
        SDeviceInfo dev = PartitionSeniorManager::instance()->findDev(partinfo.device);
        if(this->filterGhostDisk(dev.device)) {
            return;
        }

        if (!PartitionSeniorManager::instance()->isFormatDisk(dev)) {
            SPartitionInfo free = PartitionSeniorManager::instance()->find(dev, partinfo);
            
            QMap<QString, EPartitionType> partTypes;
            if (PartitionSeniorManager::instance()->isNewPrimaryPartition(dev, partinfo)) {
                partTypes.insert(CommonFunc::partitionTypeToText().key(EPartitionType::PartitionTypePrimary), EPartitionType::PartitionTypePrimary);
            }

            if (PartitionSeniorManager::instance()->isNewLogicalPartition(dev, free)) {
                partTypes.insert(CommonFunc::partitionTypeToText().key(EPartitionType::PartitionTypeLogical), EPartitionType::PartitionTypeLogical);
            }

            if (!partTypes.isEmpty()) {
                QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionSeniorManager::instance()->getSystemPartitionInfo();
                if (devs.value(dev).size()<=1) {
                    free = PartitionSeniorManager::instance()->getEmptyDiskPartition(dev);
                }

                CreateNewPartitionDialog newPartitionDlg(dev, free);
                newPartitionDlg.setPartitionTypes(partTypes);
                if (newPartitionDlg.exec() == DDialog::Accepted) {
                    SPartitionInfo newInfo = newPartitionDlg.getPartition();
                    newInfo.isNeedFormat = newPartitionDlg.getFormatPartitionCheckStatus();
                    PartitionSeniorManager::instance()->newPartition(dev, partinfo, newInfo);
                    PartitionSeniorManager::instance()->setHideCryptParts(dev);
                    devs = PartitionSeniorManager::instance()->getSystemPartitionInfo();
                    QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionSeniorManager::instance()->getSystemLVMInfo();
                    this->updateInstallInfo(devs, vgs);
                    this->updateTips("");
                }
                setFocus();
            } else {
                this->updateTips(::QObject::tr("There can only be four primary partitions for legacy boot. Please reserve enough primary partitions."));
            }
        } else {
            // 格式化磁盘界面
            this->setFormatDisk(dev);
        }
    }, Qt::QueuedConnection);

    connect(m_model, &ItemModel::signalPartitionDelete, this, [=](const SPartitionInfo &partinfo) {
        SDeviceInfo dev = PartitionSeniorManager::instance()->findDev(partinfo.device);
        if(this->filterGhostDisk(dev.device)) {
            return;
        }

        if (!PartitionSeniorManager::instance()->isFormatDisk(dev)) {
            PartitionSeniorManager::instance()->delPartition(dev, partinfo);
        } else {
            this->setFormatDisk(dev);
        }

        PartitionSeniorManager::instance()->setHideCryptParts(dev);

        QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionSeniorManager::instance()->getSystemPartitionInfo();
        QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionSeniorManager::instance()->getSystemLVMInfo();
        this->updateInstallInfo(devs, vgs);
        this->updateTips("");
    });

    connect(m_model, &ItemModel::signalPartitionEdit, this, [=](const SPartitionInfo &partinfo) {
        SDeviceInfo dev = PartitionSeniorManager::instance()->findDev(partinfo.device);
        if(this->filterGhostDisk(dev.device)) {
            return;
        }

        if (!PartitionSeniorManager::instance()->isFormatDisk(dev)) {
            SPartitionInfo part = PartitionSeniorManager::instance()->find(dev, partinfo);
            EditPartitionDialog editDialog(part);
            if (editDialog.exec() == DDialog::Accepted) {
                part = editDialog.getPartition();
                part.isNeedFormat = editDialog.getFormatPartitionCheckStatus();
                PartitionSeniorManager::instance()->editPartition(dev, part);
            }
            setFocus();
        } else {
            this->setFormatDisk(dev);
        }

        QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionSeniorManager::instance()->getSystemPartitionInfo();
        QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionSeniorManager::instance()->getSystemLVMInfo();
        this->updateInstallInfo(devs, vgs);
        this->updateTips("");
    }, Qt::QueuedConnection);

    connect(m_model, &ItemModel::signalVGDelete, this, [=](const SVGInfo &vg) {
        PartitionSeniorManager::instance()->delVG(vg);
        QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionSeniorManager::instance()->getSystemPartitionInfo();
        QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionSeniorManager::instance()->getSystemLVMInfo();
        this->updateInstallInfo(devs, vgs);
        this->updateTips("");
    });

    connect(m_model, &ItemModel::signalLVMAdd, this, [=](const SLVMInfo &freeLvName) {
        SLVMInfo free = PartitionSeniorManager::instance()->findLV(freeLvName.id);
        free.path = "";
        CreateNewLVMDialog newlvmDlg(free);
        if (newlvmDlg.exec() == DDialog::Accepted) {
            SLVMInfo lv = newlvmDlg.getLVInfo();
            lv.isNeedFormat = newlvmDlg.getFormatPartitionCheckStatus();
            PartitionSeniorManager::instance()->newLV(free, lv);
            QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionSeniorManager::instance()->getSystemPartitionInfo();
            QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionSeniorManager::instance()->getSystemLVMInfo();
            this->updateInstallInfo(devs, vgs);
            this->updateTips("");
        }
        setFocus();
    });

    connect(m_model, &ItemModel::signalLVMEdit, this, [=](const SLVMInfo &lv) {
        SLVMInfo lvInfo = PartitionSeniorManager::instance()->findLV(lv.id);
        EditLVMDialog editlvmDlg(lvInfo);
        if (editlvmDlg.exec() == DDialog::Accepted) {
            SLVMInfo tmpLv = editlvmDlg.getPartition();
            tmpLv.isNeedFormat = editlvmDlg.getFormatPartitionCheckStatus();
            PartitionSeniorManager::instance()->editLV(tmpLv);
            QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionSeniorManager::instance()->getSystemPartitionInfo();
            QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionSeniorManager::instance()->getSystemLVMInfo();
            this->updateInstallInfo(devs, vgs);
            this->updateTips("");
        }
        setFocus();
    });

    connect(m_model, &ItemModel::signalLVMDelete, this, [=](const SLVMInfo &lv) {
        PartitionSeniorManager::instance()->delLV(lv);
        QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionSeniorManager::instance()->getSystemPartitionInfo();
        QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionSeniorManager::instance()->getSystemLVMInfo();
        this->updateInstallInfo(devs, vgs);
        this->updateTips("");
    });

    m_tipLabel = new DLabel(this);
    m_tipLabel->setAlignment(Qt::AlignCenter);
    m_tipLabel->setFixedWidth(600);
    m_tipLabel->setWordWrap(true);
    DPalette pe = m_tipLabel->palette();
    pe.setColor(DPalette::WindowText, QColor("#FF7400"));
    m_tipLabel->setBackgroundRole(DPalette::Window);
    m_tipLabel->setPalette(pe);

    QHBoxLayout *tipsLayout = new QHBoxLayout;
    tipsLayout->setContentsMargins(0, 0, 0, 0);
    tipsLayout->addStretch();
    tipsLayout->addWidget(m_tipLabel);
    tipsLayout->addStretch();

    // 确认按钮
    m_nextBtn = new DSuggestButton(this);
    m_nextBtn->setFixedSize(gnextbuttonwidth, gnextbuttonheight);
    m_nextBtn->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_nextBtn->setDefault(true);
    connect(m_nextBtn, &DSuggestButton::clicked, this, [=] {
        QString log;
        SPartitionInfo bootloader;
        if (!PartitionSeniorManager::instance()->checkPartition(log)) {
            this->updateTips(log);
        } else if (!PartitionSeniorManager::instance()->findFirstESP(bootloader)) {
            this->updateTips(QString(::QObject::tr("The %1 boot partition must be a primary partition and the first partition on the disk")).arg(bootloader.mountPoint));
        } else {
            if(CommonFunc::isGhost()) {
                if(!PartitionSeniorManager::instance()->checkGhostPartition(log)) {
                    this->updateTips(log);
                    return;
                }
            }
            //不存在swap分区,弹框提示用户创建swap分区
            QString swapPartId = "";
            if (!PartitionSeniorManager::instance()->isHaveCreatedSwap(swapPartId)) {
                CreateSwapDialog dialog;
                dialog.move(qApp->primaryScreen()->availableSize().width() / 2 - dialog.width() / 2,
                            qApp->primaryScreen()->availableSize().height() / 2 - dialog.height() / 2);
                if (dialog.exec() == DDialog::Accepted) {
                    setFocus();
                    return;
                }
                setFocus();
            }

            Q_EMIT this->next();
        }
    });

    m_helpShowDialog = new SeniorTipDialog;
    m_helpShowDialog->setVisible(false);
    m_helpLabel = new HelpLabel;
    QHBoxLayout *okLayout = new QHBoxLayout;
    okLayout->setContentsMargins(20, 0, 20, 0);
    okLayout->addWidget(new QWidget);
    okLayout->addWidget(m_nextBtn, 0, Qt::AlignHCenter);
    okLayout->addWidget(m_helpLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
    connect(m_helpLabel, &HelpLabel::hoverEnter, [=]{
        QPoint pos = m_helpLabel->mapToGlobal(m_helpLabel->rect().topRight());
        pos += QPoint(10, -15);
        QRect geRect = m_helpShowDialog->geometry();
        geRect.moveBottomRight(pos);
        m_helpShowDialog->move(geRect.topLeft());
        m_helpShowDialog->show();
    });
    connect(m_helpLabel, &HelpLabel::hoverLeave, [=]{
        m_helpShowDialog->hide();
    });

    // 主布局
    QWidget *partitionWidget = new QWidget;
    QVBoxLayout *partitionLayout = new QVBoxLayout;
    partitionLayout->setContentsMargins(0, 0, 0, 0);
    partitionLayout->setSpacing(0);
    partitionLayout->addLayout(titleLayout);
    partitionLayout->addLayout(listLayout);
    partitionLayout->addSpacing(glayoutSpacing);
    partitionLayout->addLayout(tipsLayout);
    partitionLayout->addSpacing(glayoutSpacing);
    partitionLayout->addLayout(okLayout);
    partitionWidget->setLayout(partitionLayout);

    QVBoxLayout *mainlayout = new QVBoxLayout;
    mainlayout->setContentsMargins(0, 0, 0, 0);
    mainlayout->setSpacing(0);
    mainlayout->addWidget(m_centerWidget);
    setLayout(mainlayout);
    m_centerWidget->addWidget(partitionWidget);
    m_centerWidget->addWidget(m_lvmFrame);
    //
    initConnection();
}

void SeniorPartitionFrame::updateInstallInfo(const QMap<SDeviceInfo, QList<SPartitionInfo>> &devs, const QMap<SVGInfo, QList<SLVMInfo>> &vgs)
{
    Q_UNUSED(vgs);
    // This release installs to ordinary disk partitions. Existing volume groups
    // are not offered as editable installation targets.
    m_model->setDatas(devs, {});
    this->update();
}

void SeniorPartitionFrame::updateTips(const QString &text)
{
    m_tipLabel->setText(text);
}

void SeniorPartitionFrame::setFormatDisk(const SDeviceInfo &dev)
{
    // 格式化磁盘界面
    FormatDiskTipsDialog dialog(dev);
    if (dialog.exec() == DDialog::Accepted) {
        PartitionSeniorManager::instance()->setFormatDisk(dev);
        QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionSeniorManager::instance()->getSystemPartitionInfo();
        QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionSeniorManager::instance()->getSystemLVMInfo();
        this->updateInstallInfo(devs, vgs);
        this->updateTips("");
    }
    setFocus();
}

bool SeniorPartitionFrame::filterGhostDisk(const QString &devPath)
{
    if(CommonFunc::isGhost()) {
        QString ghostDevPath = CommonFunc::getSettingsString("DI_UIMG_DEVICE");
        if(ghostDevPath.startsWith(devPath)) {
            DDialog dialog(::QObject::tr("Can not select the disk contains the image"), "");
            dialog.setIcon(QIcon::fromTheme("dialog-warning"));
            dialog.addButton(::QObject::tr("Confirm"), false, DDialog::ButtonWarning);
            dialog.move(qApp->primaryScreen()->availableSize().width() / 2 - dialog.width() / 2,
                        qApp->primaryScreen()->availableSize().height() / 2 - dialog.height() / 2);
            dialog.exec();
            return true;
        }
    }

    return false;
}

void SeniorPartitionFrame::encryptSinglePartition(const QString &partId)
{
    PasswdDialog encryptDlg(this);
    encryptDlg.setAutoDecryptVisible(!PartitionSeniorManager::instance()->find(partId).mountPoint.isEmpty() && CommonFunc::isTPM());
    if (QDialog::Accepted == encryptDlg.exec()) {
        QString strpwd = encryptDlg.getPasswd();
        bool bAuto = encryptDlg.autoDecryptIsChecked();
        qInfo() << "user encrypt partition:" << partId;
        PartitionSeniorManager::instance()->updateEncryptStatus(partId, strpwd, bAuto, true);

        for (SDeviceInfo devItem : PartitionOperateManager::instance()->getCurrentDevice()) {
            QString partToDev = PartitionSeniorManager::instance()->find(partId).device;
            if (!partToDev.isEmpty()) {
                SDeviceInfo devItemSet = PartitionSeniorManager::instance()->findDev(partToDev);
                PartitionSeniorManager::instance()->setHideCryptParts(devItemSet);
                break;
            }
        }

        QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionSeniorManager::instance()->getSystemPartitionInfo();
        QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionSeniorManager::instance()->getSystemLVMInfo();
        this->updateInstallInfo(devs, vgs);
    }
}

void SeniorPartitionFrame::decryptSinglePartition(const QString &partId, const QString &mountPoint)
{
    DDialog decryptDlg;
    decryptDlg.setTitle(tr("Are you sure to cancel encrypt %1?").arg(mountPoint));
    decryptDlg.addSpacing(30);
    decryptDlg.addButton(::QObject::tr("Cancel"));
    decryptDlg.addButton(::QObject::tr("Confirm"), true, DDialog::ButtonWarning);
    connect(&decryptDlg, &DDialog::visibleChanged, [&](bool visible){
        if(visible) {
            decryptDlg.moveToCenter();
        }
    });
    decryptDlg.move(qApp->primaryScreen()->availableSize().width() / 2 - decryptDlg.width() / 2,
                    qApp->primaryScreen()->availableSize().height() / 2 - decryptDlg.height() / 2);
    if(QDialog::Accepted == decryptDlg.exec()) {
        qInfo() << "user cancel encrypt partition";
        PartitionSeniorManager::instance()->updateEncryptStatus(partId, "", false, false);

        for (SDeviceInfo devItem : PartitionOperateManager::instance()->getCurrentDevice()) {
            QString partToDev = PartitionSeniorManager::instance()->find(partId).device;
            if (!partToDev.isEmpty()) {
                SDeviceInfo devItemSet = PartitionSeniorManager::instance()->findDev(partToDev);
                PartitionSeniorManager::instance()->setHideCryptParts(devItemSet);
                break;
            }
        }

        QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionSeniorManager::instance()->getSystemPartitionInfo();
        QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionSeniorManager::instance()->getSystemLVMInfo();
        this->updateInstallInfo(devs, vgs);
    }
}

void SeniorPartitionFrame::updateTs()
{
    m_titleWidget->setTitleLabelProperty(::QObject::tr("Advanced"), "");
    m_titleWidget->setTipsLabelProperty(tr("Create a root filesystem and a separate ext4 /boot partition. Select formatting for both. UEFI also requires a FAT32 /boot/efi partition."), "font-size:16px;font-weight:Medium;");

    m_listReset->setText(::QObject::tr("Reset List"));

    m_lvmCreate->setText(::QObject::tr("Create LVM"));

    m_nextBtn->setText(::QObject::tr("Next"));
    m_helpShowDialog->updateTs();
    m_lvmFrame->updateTs();
}

bool SeniorPartitionFrame::switchBack()
{
    if(m_centerWidget->currentIndex() == 1) {
        m_centerWidget->setCurrentIndex(0);
        return true;
    }

    return false;
}

void SeniorPartitionFrame::initDeviceInfo(const QMap<SDeviceInfo, QList<SPartitionInfo> > &devs, const QMap<SVGInfo, QList<SLVMInfo> > &vgs)
{
    Q_UNUSED(vgs);
    m_model->setDatas(devs, {});
    PartitionSeniorManager::instance()->clearCryptList();
    updateTips("");
    if (devs.isEmpty()) {
        // 错误显示
        m_tipLabel->setText(::QObject::tr("You need at least %1 GB disk space to install %2. To get better performance, %3 GB or more is recommended.")
                            .arg(CommonFunc::getSettingsString("DI_DEVICE_MIN_SIZE_CONFIG"),
                                 CommonFunc::isNameDeepin(),
                                 CommonFunc::getSettingsString("DI_DEVICE_MIN_SIZE_CONFIG")));
    }
}

void SeniorPartitionFrame::initConnection()
{
    connect(m_model, &ItemModel::signalPartitionEncrypt, this, &SeniorPartitionFrame::encryptSinglePartition);

    connect(m_model, &ItemModel::signalPartitionDecrypt, this, &SeniorPartitionFrame::decryptSinglePartition);
}
