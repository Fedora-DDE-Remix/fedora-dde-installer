#include "CreateNewLVMDialog.h"
#include "globals.h"
#include "PartitionSizeSlider.h"
#include "TitleWidget.h"
#include "CommonFunc.h"
#include "PartitionSeniorManager.h"

#include <DFrame>
#include <DCheckBox>

#include <QUuid>
#include <QScreen>

CreateNewLVMDialog::CreateNewLVMDialog(const SLVMInfo &freeLv, QWidget *parent):
    DDialog (parent),
    m_freeLVMInfo(freeLv)
{
    setOnButtonClickedClose(false);
    setTitle(::QObject::tr("Create LVM"));
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

    m_fileSystemLabel = new DLabel(this);
    m_fileSystemLabel->setFixedSize(100, 36);
    m_fileSystemLabel->setText(::QObject::tr("File system"));
    m_fileSystemCmb = new DComboBox(this);
    m_fileSystemCmb->setFixedSize(300, 36);
    m_fileSystemCmb->addItems(getFileSystemList());
    connect(m_fileSystemCmb, &DComboBox::currentTextChanged, this, [=]{
        m_newLVMInfo.filesystem = m_fileSystemCmb->currentText();
        m_tipsLabel->setText(QString());
        if(limitEditorSizeByFileSystem()) {
            updateErrTips(m_formatPartition->checkState() == Qt::CheckState::Checked ? formattedTips : "");
        }

        if (m_newLVMInfo.filesystem == "linux-swap" || m_newLVMInfo.filesystem == "lvm2 pv") {
            m_mountPointCmb->setEnabled(false);
            m_mountPointCmb->setCurrentText(CommonFunc::getPartitionMountText(""));
        } else {
            m_mountPointCmb->setEnabled(true);
        }
    });

    QHBoxLayout *pFileSystemLayout = new QHBoxLayout;
    pFileSystemLayout->setSpacing(glayoutSpacing);
    pFileSystemLayout->addStretch();
    pFileSystemLayout->addWidget(m_fileSystemLabel, 0, Qt::AlignLeft);
    pFileSystemLayout->addWidget(m_fileSystemCmb, 0, Qt::AlignRight);
    pFileSystemLayout->addStretch();

    m_mountPointLabel = new DLabel(this);
    m_mountPointLabel->setFixedSize(100, 36);
    m_mountPointLabel->setText(::QObject::tr("Mount point"));
    m_mountPointCmb = new DComboBox(this);
    m_mountPointCmb->setFixedSize(300, 36);
    m_mountPointCmb->addItems(CommonFunc::getPartitionMountPoints(m_freeLVMInfo));
    m_mountPointCmb->setCurrentText(CommonFunc::getPartitionMountText(m_freeLVMInfo.mountPoint));
    connect(m_mountPointCmb, &DComboBox::currentTextChanged, this, [=]{
        m_newLVMInfo.mountPoint = CommonFunc::getPartitionMountPoint(m_mountPointCmb->currentText());
        m_tipsLabel->setText(QString());
        if(limitEditorSizeByMountPoint()) {
            updateErrTips(m_formatPartition->checkState() == Qt::CheckState::Checked ? formattedTips : "");
        }
    });

    QHBoxLayout *pMountPointLayout = new QHBoxLayout;
    pMountPointLayout->setSpacing(glayoutSpacing);
    pMountPointLayout->addStretch();
    pMountPointLayout->addWidget(m_mountPointLabel, 0, Qt::AlignLeft);
    pMountPointLayout->addWidget(m_mountPointCmb, 0, Qt::AlignRight);
    pMountPointLayout->addStretch();

    m_sizeLabel = new DLabel(this);
    m_sizeLabel->setFixedSize(100, 36);
    m_sizeLabel->setText(::QObject::tr("Size"));
    m_editSlider = new PartitionSizeSlider(this);
    m_editSlider->setValue(0);
    m_editSlider->setFixedWidth(300);
    connect(m_editSlider, &PartitionSizeSlider::valueChanged, this, [=](qint64 size){
        m_newLVMInfo.size = size;
    });

    QHBoxLayout *pSizeLayout = new QHBoxLayout;
    pSizeLayout->setSpacing(glayoutSpacing);
    pSizeLayout->addStretch();
    pSizeLayout->addWidget(m_sizeLabel, 0, Qt::AlignLeft);
    pSizeLayout->addWidget(m_editSlider, 0, Qt::AlignRight);
    pSizeLayout->addStretch();

    QHBoxLayout *pHLayout = new QHBoxLayout;
    pHLayout->addStretch();
    pHLayout->addWidget(m_cancelBtn);
    pHLayout->addSpacing(15);
    pHLayout->addWidget(m_okBtn);
    pHLayout->addStretch();

    QVBoxLayout *pVLayout = new QVBoxLayout;
    pVLayout->setContentsMargins(0, 0, 0, 0);
    pVLayout->addLayout(pFileSystemLayout);
    pVLayout->addLayout(pMountPointLayout);
    pVLayout->addLayout(pSizeLayout);
    pVLayout->addLayout(formatPartitionHLayout);
    pVLayout->addWidget(m_tipsLabel, Qt::AlignHCenter);
    pVLayout->addLayout(pHLayout);

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
    m_okBtn->setEnabled(false);
    connect(m_cancelBtn, &QAbstractButton::clicked, this, &DDialog::close);
    connect(this, &DDialog::closed, this, [=]() {
        updateErrTips("");
        reject();
    });
    connect(m_okBtn, &DSuggestButton::clicked, this, [=]{
        QString err = QString();
        m_newLVMInfo.filesystem = m_fileSystemCmb->currentText();
        m_newLVMInfo.mountPoint = CommonFunc::getPartitionMountPoint(m_mountPointCmb->currentText());
        if (PartitionSeniorManager::instance()->isExistSameMountpointAsThePrimaryPartition(m_newLVMInfo.mountPoint)) {
            updateErrTips(QString(::QObject::tr("The %1 mount point has been used, please select another one")).arg(m_newLVMInfo.mountPoint));
            return;
        }

        if (!PartitionSeniorManager::instance()->checkDialogNextEnable(m_newLVMInfo, err)) {
            updateErrTips(err);
            return;
        }
        updateErrTips(err);
        accept();
    });
    setFixedSize(440, 370);

    this->initParition();
    limitEditorSizeByFileSystem();
    limitEditorSizeByMountPoint();

    move(qApp->primaryScreen()->availableSize().width() / 2 - width() / 2,
         qApp->primaryScreen()->availableSize().height() / 2 - height() / 2);
}

SLVMInfo CreateNewLVMDialog::getLVInfo() const
{
    return m_newLVMInfo;
}

void CreateNewLVMDialog::initParition()
{
    m_newLVMInfo.id = QUuid::createUuid().toString();
    m_newLVMInfo.filesystem = m_fileSystemCmb->currentText();
    m_newLVMInfo.mountPoint = CommonFunc::getPartitionMountPoint(m_mountPointCmb->currentText());
    m_newLVMInfo.size = m_editSlider->value();
}

bool CreateNewLVMDialog::limitEditorSizeByMountPoint()
{
    qint64 ghostMinSize = 0;
    if(CommonFunc::isGhost()) {
        ghostMinSize = PartitionSeniorManager::instance()->getMinGhostPartitionSize(m_newLVMInfo.mountPoint);
    }

    qint64 maxSize = m_freeLVMInfo.size;
    qint64 minSize = 0;
    bool useMinSize = false;
    if (gBootLabel == CommonFunc::getPartitionLabel(m_newLVMInfo)) {
        minSize = qMax((qint64)CommonFunc::getSettingsInt("DI_BOOT_SIZE_CONFIG"), ghostMinSize);
        useMinSize = true;
    } else if (gRootaLabel == CommonFunc::getPartitionLabel(m_newLVMInfo)) {
        minSize = CommonFunc::getSettingsInt("DI_ROOTA_SIZE_CONFIG");
        useMinSize = true;
    } else if (gEFILabel == CommonFunc::getPartitionLabel(m_newLVMInfo)) {
        minSize = qMax((qint64)CommonFunc::getSettingsInt("DI_EFI_SIZE_CONFIG"), ghostMinSize);
        useMinSize = true;
    } else if (gSWAPLabel == CommonFunc::getPartitionLabel(m_newLVMInfo)) {
        minSize = CommonFunc::getSwapSize();
        useMinSize = true;
    } else {
        if (m_newLVMInfo.mountPoint.isEmpty()) {
            minSize = 1;
        } else {
            qint64 otherSize = CommonFunc::getSettingsInt("DI_OTHER_SIZE_CONFIG");
            minSize = qMax(otherSize, ghostMinSize);
        }
    }

    return checkSliderRange(minSize, maxSize, useMinSize);
}

bool CreateNewLVMDialog::limitEditorSizeByFileSystem()
{
    qint64 maxSize = m_freeLVMInfo.size;
    qint64 minSize = 0;
    bool useMinSize = false;
    if (gSwapFileSystems.contains(m_newLVMInfo.filesystem)) {
        qint64 swapSize = CommonFunc::getSwapSize();
        minSize = swapSize;
        useMinSize = true;
    } else if ("vfat" == m_newLVMInfo.filesystem) {
        minSize = CommonFunc::getSettingsInt("DI_EFI_SIZE_CONFIG");
        useMinSize = true;
    }  else {
        minSize = m_newLVMInfo.mountPoint.isEmpty() ? 1 : CommonFunc::getSettingsInt("DI_OTHER_SIZE_CONFIG");
    }

    return checkSliderRange(minSize, maxSize, useMinSize);
}

void CreateNewLVMDialog::updateErrTips(const QString &err)
{
    m_tipsLabel->setStyleSheet(QString("QLabel{color:%1;}").arg("#FF7400"));
    m_tipsLabel->setText(err);
}

QStringList CreateNewLVMDialog::getFileSystemList()
{
    QStringList list = CommonFunc::getSettingsStringList("DI_LVM_FILESYSTEM_CONFIG");
    return list;
}

bool CreateNewLVMDialog::checkSliderRange(qint64 minSize, qint64 maxSize, bool bUseMin)
{
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

bool CreateNewLVMDialog::getFormatPartitionCheckStatus()
{
    return m_formatPartition->isChecked();
}
