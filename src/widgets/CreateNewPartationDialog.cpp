#include "CreateNewPartationDialog.h"
#include "globals.h"
#include "PartitionSizeSlider.h"
#include "CommonFunc.h"
#include "PartitionSeniorManager.h"
#include "TitleWidget.h"

#include <DFrame>
#include <QHBoxLayout>
#include <DCheckBox>

#include <QUuid>
#include <QMap>
#include <cmath>
#include <QScreen>

static const int kContentLength = 310;

CreateNewPartitionDialog::CreateNewPartitionDialog(const SDeviceInfo &dev, const SPartitionInfo &free, QWidget *parent):
    DDialog (parent),
    m_freePartitionInfo(free)
{
    setOnButtonClickedClose(false);
    setTitle(::QObject::tr("Create partition"));

    QWidget *pInnerWidget = new QWidget(this);
    m_tipsLabel = new DLabel(this);
    m_tipsLabel->setAlignment(Qt::AlignCenter);
    m_tipsLabel->setFixedSize(420, 40);
    m_tipsLabel->setWordWrap(true);

    m_formatPartition = new DCheckBox(::QObject::tr("Format the partition"));
    m_formatPartition->setCheckState(Qt::CheckState::Checked);
    m_formatPartition->setEnabled(false);
//    m_formatPartition->setCheckState(info.isNeedFormat ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    QString formattedTips = ::QObject::tr("This partition will be formatted. Make sure you have backed up all data.");
    updateErrTips(m_formatPartition->checkState() == Qt::CheckState::Checked ? formattedTips : "");
    connect(m_formatPartition, &DCheckBox::clicked, this, [=] {
        updateErrTips(m_formatPartition->checkState() == Qt::CheckState::Checked ? formattedTips : "");
    });
    QHBoxLayout *formatPartitionHLayout = new QHBoxLayout;
    formatPartitionHLayout->addStretch();
    formatPartitionHLayout->addWidget(m_formatPartition);
    formatPartitionHLayout->addStretch();

    m_typeLabel = new DLabel(this);
    m_typeLabel->setFixedSize(100, 36);
    m_typeLabel->setText(::QObject::tr("Type"));
    m_typeCmb = new DComboBox(this);
    m_typeCmb->setFixedSize(kContentLength, 36);
    connect(m_typeCmb, &DComboBox::currentTextChanged, this, [=](const QString &text){
        m_newPartitionInfo.partitionType = CommonFunc::partitionTypeToText().value(text);

        QList<std::pair<QString, bool>> partLocatins = {CommonFunc::getPartitionStartPointText(), CommonFunc::getPartitionEndPointText()};
        EPartitionType partType =  CommonFunc::partitionTypeToText().value(m_typeCmb->currentText());
        if (partType == EPartitionType::PartitionTypeLogical) {
            if (PartitionSeniorManager::instance()->isNewStartLogicalPartition(dev, free)) {
                partLocatins = {CommonFunc::getPartitionEndPointText()};
            } else if (PartitionSeniorManager::instance()->isNewEndLogicalPartition(dev, free)) {
                partLocatins = {CommonFunc::getPartitionStartPointText()};
            }
        }

        this->setPartitionLocation(partLocatins);
    });

    QHBoxLayout *pTypeLayout = new QHBoxLayout;
    pTypeLayout->setSpacing(glayoutSpacing);
    pTypeLayout->addWidget(m_typeLabel, 0, Qt::AlignLeft);
    pTypeLayout->addWidget(m_typeCmb, 0, Qt::AlignRight);

    m_locationLabel = new DLabel(this);
    m_locationLabel->setFixedSize(100, 36);
    m_locationLabel->setText(::QObject::tr("Location"));
    m_locationCmb = new DComboBox(this);
    m_locationCmb->setFixedSize(kContentLength, 36);
    m_locationCmb->addItems({CommonFunc::getPartitionStartPointText().first, CommonFunc::getPartitionEndPointText().first});
    connect(m_locationCmb, &DComboBox::currentTextChanged, this, [=](const QString &text){
        m_newPartitionInfo.isStartPoint = std::map<QString, bool>{CommonFunc::getPartitionStartPointText(), CommonFunc::getPartitionEndPointText()}[text];
    });

    QHBoxLayout *pLocationLayout = new QHBoxLayout;
    pLocationLayout->setSpacing(glayoutSpacing);
    pLocationLayout->addWidget(m_locationLabel, 0, Qt::AlignLeft);
    pLocationLayout->addWidget(m_locationCmb, 0, Qt::AlignRight);

    m_fileSystemLabel = new DLabel(this);
    m_fileSystemLabel->setFixedSize(100, 36);
    m_fileSystemLabel->setText(::QObject::tr("File system"));
    m_fileSystemCmb = new DComboBox(this);
    m_fileSystemCmb->setFixedSize(kContentLength, 36);
    m_fileSystemCmb->addItems(getFileSystemList());
    connect(m_fileSystemCmb, &DComboBox::currentTextChanged, this, [=](const QString &text){
        const bool biosBoot = text == ::QObject::tr("BIOS boot (no filesystem)");
        m_newPartitionInfo.isBiosBoot = biosBoot;
        m_newPartitionInfo.filesystem = biosBoot ? QString() : text;
        m_newPartitionInfo.isNeedFormat = !biosBoot;
        m_formatPartition->setChecked(!biosBoot);

        if (biosBoot || text == "linux-swap" || text == "lvm2 pv") {
            m_mountPointCmb->setEnabled(false);
            m_mountPointCmb->setCurrentText(CommonFunc::getPartitionMountText(""));
            this->limitEditorSizeByMountPoint();
        } else if (text == "vfat") {
            m_mountPointCmb->setCurrentText(CommonFunc::getPartitionMountText("/boot/efi"));
        } else {
            m_mountPointCmb->setEnabled(true);
        }
        updateErrTips(biosBoot ? ::QObject::tr("Reserved for GRUB on a BIOS/GPT disk; this partition is not formatted or mounted.") : formattedTips);
    });

    QHBoxLayout *pFileSystemLayout = new QHBoxLayout;
    pFileSystemLayout->setSpacing(glayoutSpacing);
    pFileSystemLayout->addWidget(m_fileSystemLabel, 0, Qt::AlignLeft);
    pFileSystemLayout->addWidget(m_fileSystemCmb, 0, Qt::AlignRight);

    m_mountPointLabel = new DLabel(this);
    m_mountPointLabel->setFixedSize(100, 36);
    m_mountPointLabel->setText(::QObject::tr("Mount point"));

    m_mountPointCmb = new DComboBox(this);
    m_mountPointCmb->setFixedSize(kContentLength, 36);
    m_mountPointCmb->addItems(CommonFunc::getPartitionMountPoints(m_freePartitionInfo));
    m_mountPointCmb->setCurrentText(CommonFunc::getPartitionMountText(m_freePartitionInfo.mountPoint));
    connect(m_mountPointCmb, &DComboBox::currentTextChanged, this, [=](const QString &text){
        m_newPartitionInfo.mountPoint = CommonFunc::getPartitionMountPoint(text);
        if(m_newPartitionInfo.mountPoint == "/boot/efi") {
            m_fileSystemCmb->setCurrentText("vfat");
        }

        m_tipsLabel->setText(QString());
        if(limitEditorSizeByMountPoint()) {
            updateErrTips(m_formatPartition->checkState() == Qt::CheckState::Checked ? formattedTips : "");
        }
    });

    QHBoxLayout *pMountPointLayout = new QHBoxLayout;
    pMountPointLayout->setSpacing(glayoutSpacing);
    pMountPointLayout->addWidget(m_mountPointLabel, 0, Qt::AlignLeft);
    pMountPointLayout->addWidget(m_mountPointCmb, 0, Qt::AlignRight);

    m_sizeLabel = new DLabel(this);
    m_sizeLabel->setFixedSize(100, 36);
    m_sizeLabel->setText(::QObject::tr("Size"));
    m_editSlider = new PartitionSizeSlider(this);
    m_editSlider->setFixedWidth(kContentLength);
    connect(m_editSlider, &PartitionSizeSlider::valueChanged, this, [=](qint64 size){
        m_newPartitionInfo.size = size;
        m_newPartitionInfo.byteSize = CommonFunc::mToSectors(size, m_newPartitionInfo.sectorSize) * m_newPartitionInfo.sectorSize;
    });

    QHBoxLayout *pSizeLayout = new QHBoxLayout;
    pSizeLayout->setSpacing(glayoutSpacing);
    pSizeLayout->addWidget(m_sizeLabel, 0, Qt::AlignLeft);
    pSizeLayout->addWidget(m_editSlider, 0, Qt::AlignRight);

    QVBoxLayout *pVLayout = new QVBoxLayout;
    pVLayout->setContentsMargins(0, 0, 0, 0);
    pVLayout->setSpacing(glayoutSpacing);
    pVLayout->addSpacing(glayoutSpacing);
    pVLayout->addLayout(pTypeLayout);
    pVLayout->addLayout(pLocationLayout);
    pVLayout->addLayout(pFileSystemLayout);
    pVLayout->addLayout(pMountPointLayout);
    pVLayout->addLayout(pSizeLayout);
    pVLayout->addLayout(formatPartitionHLayout);
    pVLayout->addWidget(m_tipsLabel, Qt::AlignHCenter);

    QHBoxLayout *pMainLayout = new QHBoxLayout;
    pMainLayout->setContentsMargins(0, 0, 0, 0);
    pMainLayout->addStretch();
    pMainLayout->addLayout(pVLayout);
    pMainLayout->addStretch();
    pInnerWidget->setLayout(pMainLayout);

    addContent(pInnerWidget);
    addButton(::QObject::tr("Cancel", "button"));
    addButton(::QObject::tr("Confirm", "button"), true, DDialog::ButtonRecommend);
    m_cancelBtn = getButton(0);
    m_okBtn = getButton(1);
    connect(m_cancelBtn, &QAbstractButton::clicked, [=]{
        updateErrTips("");
        reject();
    });
    connect(this, &DDialog::closed, this, [=]() {
        updateErrTips("");
        reject();
    });
    connect(m_okBtn, &QAbstractButton::clicked, this, [=]{
        QString err = QString();
        m_newPartitionInfo.isBiosBoot = m_fileSystemCmb->currentText() == ::QObject::tr("BIOS boot (no filesystem)");
        m_newPartitionInfo.filesystem = m_newPartitionInfo.isBiosBoot ? QString() : m_fileSystemCmb->currentText();
        m_newPartitionInfo.isNeedFormat = m_formatPartition->isChecked();
        m_newPartitionInfo.mountPoint = CommonFunc::getPartitionMountPoint(m_mountPointCmb->currentText());
        if (!PartitionSeniorManager::instance()->checkDialogNextEnable(m_newPartitionInfo, err)) {
            updateErrTips(err);
            return;
        }
        updateErrTips(err);
        accept();
    });

    setFixedWidth(440);

    this->initPartition();
    limitEditorSizeByMountPoint();

    move(qApp->primaryScreen()->availableSize().width() / 2 - width() / 2,
         qApp->primaryScreen()->availableSize().height() / 2 - height() / 2);
}

void CreateNewPartitionDialog::setPartitionTypes(const QMap<QString, EPartitionType> &types)
{
    m_typeCmb->clear();
    QString primary = types.key(EPartitionType::PartitionTypePrimary);
    if (!primary.isEmpty()) {
        m_typeCmb->addItem(primary);
    }

    QString logical = types.key(EPartitionType::PartitionTypeLogical);
    if (!logical.isEmpty()) {
        m_typeCmb->addItem(logical);
    }
}

void CreateNewPartitionDialog::setPartitionLocation(const QList<std::pair<QString, bool> > &locations)
{
    m_locationCmb->clear();
    for (std::pair<QString, bool> in : locations) {
        m_locationCmb->addItem(in.first);
    }
}

SPartitionInfo CreateNewPartitionDialog::getPartition()
{
    return m_newPartitionInfo;
}

void CreateNewPartitionDialog::initPartition()
{
    m_newPartitionInfo.id = QUuid::createUuid().toString();
    m_newPartitionInfo.device = m_freePartitionInfo.device;
    m_newPartitionInfo.deviceSize = m_freePartitionInfo.deviceSize;
    m_newPartitionInfo.partitionType = CommonFunc::partitionTypeToText().value(m_typeCmb->currentText());
    m_newPartitionInfo.isStartPoint = std::map<QString, bool>{CommonFunc::getPartitionStartPointText(), CommonFunc::getPartitionEndPointText()}[m_locationCmb->currentText()];
    m_newPartitionInfo.filesystem = m_fileSystemCmb->currentText();
    m_newPartitionInfo.mountPoint = CommonFunc::getPartitionMountPoint(m_mountPointCmb->currentText());
    m_newPartitionInfo.sectorSize = m_freePartitionInfo.sectorSize;
    m_newPartitionInfo.phySectorSize = m_freePartitionInfo.phySectorSize;
    m_newPartitionInfo.size = m_editSlider->value();
    m_newPartitionInfo.byteSize = CommonFunc::mToSectors(m_newPartitionInfo.size, m_newPartitionInfo.sectorSize) * m_newPartitionInfo.sectorSize;
}

bool CreateNewPartitionDialog::limitEditorSizeByMountPoint()
{
    qint64 ghostMinSize = 0;
    if(CommonFunc::isGhost()) {
        ghostMinSize = PartitionSeniorManager::instance()->getMinGhostPartitionSize(m_newPartitionInfo.mountPoint);
    }

    qint64 maxSize = m_freePartitionInfo.size;
    qint64 minSize = 0;
    bool useMinSize = false;
    QString partitionLabel = CommonFunc::getPartitionLabel(m_newPartitionInfo);
    if (m_newPartitionInfo.isBiosBoot) {
        minSize = 1;
        useMinSize = true;
    } else if (gBootLabel == partitionLabel) {
        minSize = qMax((qint64)CommonFunc::getSettingsInt("DI_BOOT_SIZE_CONFIG"), ghostMinSize);
        useMinSize = true;
    } else if (gRootLabel == partitionLabel || gRootaLabel == partitionLabel) {
        minSize = CommonFunc::getSettingsInt("DI_ROOTA_SIZE_CONFIG");
        if (m_newPartitionInfo.deviceSize > gDiskThreshold) {
            minSize = static_cast<int>(floor(m_newPartitionInfo.deviceSize * 0.1));
        }
        useMinSize = true;
    } else if (gEFILabel == partitionLabel) {
        minSize = qMax((qint64)CommonFunc::getSettingsInt("DI_EFI_SIZE_CONFIG"), ghostMinSize);
        maxSize = qMin(m_freePartitionInfo.size, (qint64)4 * 1024);
        useMinSize = true;
    } else if (gSWAPLabel == partitionLabel) {
        minSize = CommonFunc::getSwapSize();
        useMinSize = true;
    }  else if (gDataLabel == partitionLabel) {
        minSize = CommonFunc::getSettingsInt("DI_PERSISTENT_SIZE_CONFIG");
    }  else {
        if (m_newPartitionInfo.mountPoint.isEmpty()) {
            if (m_newPartitionInfo.filesystem.contains(gLVM2PVFileSystem)) {
                minSize = CommonFunc::getSettingsInt("DI_LVM2_PV_SIZE_CONFIG");
            } else {
                minSize = 1;
            }
        } else {
            qint64 otherSize = CommonFunc::getSettingsInt("DI_OTHER_SIZE_CONFIG");
            minSize = qMax(otherSize, ghostMinSize);
        }
    }

    return checkSliderRange(minSize, maxSize, useMinSize);
}

bool CreateNewPartitionDialog::limitEditorSizeByFileSystem()
{
    qint64 otherSize = CommonFunc::getSettingsInt("DI_OTHER_SIZE_CONFIG");
    qint64 maxSize = m_freePartitionInfo.size;
    qint64 minSize = 0;
    bool useMinSize = false;
    if (gSwapFileSystems.contains(m_newPartitionInfo.filesystem)) {
        qint64 swapSize = CommonFunc::getSwapSize();
        minSize = swapSize;
        useMinSize = true;
    } else if (gLVM2PVFileSystem == m_newPartitionInfo.filesystem) {
        minSize = otherSize;
    } else if ("vfat" == m_newPartitionInfo.filesystem) {
        minSize = CommonFunc::getSettingsInt("DI_EFI_SIZE_CONFIG");
        useMinSize = true;
    }  else {
        minSize = m_newPartitionInfo.mountPoint.isEmpty() ? 1 : otherSize;
    }

    return checkSliderRange(minSize, maxSize, useMinSize);
}

void CreateNewPartitionDialog::updateErrTips(const QString &err)
{
    m_tipsLabel->setStyleSheet(QString("QLabel{color:%1;}").arg("#FF7400"));
    m_tipsLabel->setText(err);
}

QStringList CreateNewPartitionDialog::getFileSystemList()
{
    QStringList list = {"ext4", "xfs", "btrfs", "linux-swap"};
    if (CommonFunc::isUEFI()) {
        list.removeAll("vfat");
        list.append("vfat");
    } else if (PartitionSeniorManager::instance()->findDev(m_freePartitionInfo.device).type == EDisklabelType::GPT) {
        list.append(::QObject::tr("BIOS boot (no filesystem)"));
    }

    return list;
}

bool CreateNewPartitionDialog::checkSliderRange(qint64 minSize, qint64 maxSize, bool bUseMin)
{
    qInfo() << "check partition size range:" << minSize << maxSize;
    if(maxSize < minSize) {
        if(qAbs(maxSize - minSize) > gPartitionDelta) {
            updateErrTips(::QObject::tr("Less than the minimum system space requirement"));
            m_okBtn->setEnabled(false);
            return false;
        } else {
            minSize = maxSize;
        }
    }

    m_editSlider->setMinimum(minSize);
    m_editSlider->setMaximum(maxSize);
    if(bUseMin) {
        m_editSlider->setValue(minSize);
    } else {
        m_editSlider->setValue(maxSize);
    }

    m_okBtn->setEnabled(true);
    return true;
}

bool CreateNewPartitionDialog::getFormatPartitionCheckStatus()
{
    return m_formatPartition->isChecked();
}
