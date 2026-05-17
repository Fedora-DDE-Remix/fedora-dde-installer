#include "CustomPartitionFrame.h"
#include "TitleWidget.h"
#include "globals.h"
#include "CommonFunc.h"
#include "CustomPartationListView.h"
#include "CreateNewSysDialog.h"
#include "EditPartitionDialog.h"
#include "PartitionCustomManager.h"
#include "PartitionSeniorManager.h"
#include "FormatDiskTipsDialog.h"

#include <QHBoxLayout>
#include <QScroller>
#include <QScrollArea>
#include <QScreen>
#include <DLabel>

CustomPartitionFrame::CustomPartitionFrame(const QMap<SDeviceInfo, QList<SPartitionInfo>> &devs, const QMap<SVGInfo, QList<SLVMInfo>> &vgs, QWidget *parent):
    PartitionFrameBase(parent), m_devs(devs), m_vgs(vgs)
{
    // 标题栏
    m_titleWidget = new TitleWidget(this);
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->addWidget(m_titleWidget);

    m_tableView = new CustomPartationListView(this);
    m_tableView->setFixedWidth(CommonFunc::isBigScreen() ? 980 : 680);
    m_model = new DataModel;
    m_model->setDatas(devs, vgs);
    m_tableView->setModel(m_model);
    m_tableView->setColumnWidth(0, CommonFunc::isBigScreen() ? customPartationInstall::kColumnWidth_0 : customPartationInstall::kColumnWidthSmall_0);
    m_tableView->setColumnWidth(1, CommonFunc::isBigScreen() ? customPartationInstall::kColumnWidth_1 : customPartationInstall::kColumnWidthSmall_1);
    m_tableView->setColumnWidth(2, CommonFunc::isBigScreen() ? customPartationInstall::kColumnWidth_2 : customPartationInstall::kColumnWidthSmall_2);
    m_tableView->setColumnWidth(3, CommonFunc::isBigScreen() ? customPartationInstall::kColumnWidth_3 : customPartationInstall::kColumnWidthSmall_3);
    m_tableView->setColumnWidth(4, CommonFunc::isBigScreen() ? customPartationInstall::kColumnWidth_4 : customPartationInstall::kColumnWidthSmall_4);
    m_tableView->setColumnWidth(5, CommonFunc::isBigScreen() ? customPartationInstall::kColumnWidth_5 : customPartationInstall::kColumnWidthSmall_5);
    QHBoxLayout *viewLayout = new QHBoxLayout;
    viewLayout->setContentsMargins(0, 0, 0, 0);
    viewLayout->addWidget(m_tableView);

    connect(m_model, &DataModel::signalPartitionAdd, this, [=](const SPartitionInfo &partinfo) {
        SDeviceInfo dev = PartitionCustomManager::instance()->findDev(partinfo.device);
        if (!PartitionCustomManager::instance()->isFormatDisk(dev)) {

            if (PartitionCustomManager::instance()->isNewSystem(dev)) {
                SPartitionInfo free = PartitionCustomManager::instance()->find(dev, partinfo);
                QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionSeniorManager::instance()->getSystemPartitionInfo();
                if (devs.value(dev).size()<=1) {
                    free = PartitionSeniorManager::instance()->getEmptyDiskPartition(dev);
                }
                qint64 systemMinSpace = PartitionCustomManager::instance()->getSystemMinSpace();
                qint64 dataMinSpace = CommonFunc::getSettingsInt("DI_PERSISTENT_SIZE_CONFIG");
                // 可自定义分区的最小空间要大于等于系统空间和数据空间最小值之和
                if (free.size >= (systemMinSpace + dataMinSpace)) {
                    CreateNewSysDialog newPartitionDlg(free);
                    newPartitionDlg.setSystemSpaceRange(systemMinSpace, free.size - dataMinSpace);
                    newPartitionDlg.setDataSpaceRange(dataMinSpace, free.size - systemMinSpace);
                    if (newPartitionDlg.exec() == DDialog::Accepted) {
                        free.isNeedFormat = newPartitionDlg.getFormatPartitionCheckStatus();
                        PartitionCustomManager::instance()->newPartition(dev, free, newPartitionDlg.getSystemSpaceSize(), newPartitionDlg.getDataSpaceSize());
                        QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionCustomManager::instance()->getSystemPartitionInfo();
                        QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionCustomManager::instance()->getSystemLVMInfo();
                        this->updateInstallInfo(devs, vgs);
                        this->updateTips("");
                        setFocus();
                    }
                } else {
                    this->updateTips(::QObject::tr("The partition size is less than the minimum disk space requirement (%1) for system installation.")
                                     .arg(QString("%1GiB").arg(QString::number((systemMinSpace / giDeviceUnitSize) + (dataMinSpace / giDeviceUnitSize)))));
                }
            } else {
                this->updateTips(::QObject::tr("There can only be four primary partitions for legacy boot. Please reserve enough primary partitions."));
            }
        } else {
            this->setFormatDisk(dev);
        }
    }, Qt::QueuedConnection);

    connect(m_model, &DataModel::signalPartitionEdit, this, [=](const SPartitionInfo &partinfo) {
        SDeviceInfo dev = PartitionCustomManager::instance()->findDev(partinfo.device);
        if (!PartitionCustomManager::instance()->isFormatDisk(dev)) {
            SPartitionInfo part = PartitionCustomManager::instance()->find(dev, partinfo);
            EditPartitionDialog editDialog(part);
            if (editDialog.exec() == DDialog::Accepted) {
                part = editDialog.getPartition();
                part.isNeedFormat = editDialog.getFormatPartitionCheckStatus();
                PartitionCustomManager::instance()->editPartition(dev, part);
            }
        } else {
            this->setFormatDisk(dev);
        }

        QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionCustomManager::instance()->getSystemPartitionInfo();
        QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionCustomManager::instance()->getSystemLVMInfo();
        this->updateInstallInfo(devs, vgs);
        this->updateTips("");
    }, Qt::QueuedConnection);

    connect(m_model, &DataModel::signalPartitionDelete, this, [=](const SPartitionInfo &partinfo) {
        SDeviceInfo dev = PartitionCustomManager::instance()->findDev(partinfo.device);
        if (!PartitionCustomManager::instance()->isFormatDisk(dev)) {
            PartitionCustomManager::instance()->delPartition(dev, partinfo);
        } else {
            this->setFormatDisk(dev);
        }

        QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionCustomManager::instance()->getSystemPartitionInfo();
        QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionCustomManager::instance()->getSystemLVMInfo();
        this->updateInstallInfo(devs, vgs);
        this->updateTips("");
    });

    connect(m_model, &DataModel::signalVGDelete, this, [=](const SVGInfo &vg) {
        PartitionCustomManager::instance()->delVG(vg);
        QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionCustomManager::instance()->getSystemPartitionInfo();
        QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionCustomManager::instance()->getSystemLVMInfo();
        this->updateInstallInfo(devs, vgs);
        this->updateTips("");
    });

    QVBoxLayout *listLayout = new QVBoxLayout;
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->addLayout(viewLayout);

    m_tipLabel = new DLabel(this);
    m_tipLabel->setAlignment(Qt::AlignCenter);
    m_tipLabel->setFixedSize(600, 40);
    m_tipLabel->setWordWrap(true);
    DPalette pe = m_tipLabel->palette();
    pe.setColor(DPalette::WindowText, QColor("#FF7400"));
    m_tipLabel->setBackgroundRole(DPalette::Window);

    m_tipLabel->setPalette(pe);
    if (devs.isEmpty()) {
        // 错误显示
        m_tipLabel->setText(::QObject::tr("You need at least %1 GB disk space to install %2. To get better performance, %3 GB or more is recommended.")
                            .arg(CommonFunc::getSettingsString("DI_DEVICE_MIN_SIZE_CONFIG"),
                                 CommonFunc::isNameDeepin(),
                                 CommonFunc::getSettingsString("DI_DEVICE_MIN_SIZE_CONFIG")));
    }

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
        if (!PartitionCustomManager::instance()->checkPartition(log)) {
            this->updateTips(log);
        } else if (!PartitionCustomManager::instance()->findFirstESP(bootloader)) {
            this->updateTips(QString(::QObject::tr("The %1 boot partition must be a primary partition and the first partition on the disk")).arg(bootloader.mountPoint));
        } else {
            Q_EMIT this->next();
        } });
    QHBoxLayout *okLayout = new QHBoxLayout;
    okLayout->addWidget(m_nextBtn, 0, Qt::AlignCenter);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(titleLayout);
    mainLayout->addLayout(listLayout);
    mainLayout->addLayout(tipsLayout);
    mainLayout->addLayout(okLayout);
    this->setLayout(mainLayout);
}

void CustomPartitionFrame::updateInstallInfo(const QMap<SDeviceInfo, QList<SPartitionInfo>> &devs, const QMap<SVGInfo, QList<SLVMInfo>> &vgs)
{
    m_model->setDatas(devs, vgs);

    this->update();
}

void CustomPartitionFrame::updateTips(const QString &text)
{
    m_tipLabel->setText(text);
}

void CustomPartitionFrame::setFormatDisk(const SDeviceInfo &dev)
{
    // 格式化磁盘界面
    FormatDiskTipsDialog dialog(dev);
    if (dialog.exec() == DDialog::Accepted) {
        PartitionCustomManager::instance()->setFormatDisk(dev);
        QMap<SDeviceInfo, QList<SPartitionInfo>> devs = PartitionCustomManager::instance()->getSystemPartitionInfo();
        QMap<SVGInfo, QList<SLVMInfo>> vgs = PartitionCustomManager::instance()->getSystemLVMInfo();
        this->updateInstallInfo(devs, vgs);
        this->updateTips("");
    }
}

void CustomPartitionFrame::updateTs()
{
    setFocus();
    m_titleWidget->setTitleLabelProperty(::QObject::tr("Custom"), "");
    m_titleWidget->setTipsLabelProperty(::QObject::tr("Choose a disk partition to install the system, while the data on other partitions will be preserved."), "font-size:16px;font-weight:Medium;");

    m_nextBtn->setText(::QObject::tr("Next"));
}

void CustomPartitionFrame::initDeviceInfo(const QMap<SDeviceInfo, QList<SPartitionInfo>> &devs, const QMap<SVGInfo, QList<SLVMInfo>> &vgs)
{
    updateInstallInfo(devs, vgs);
    updateTips("");
}
