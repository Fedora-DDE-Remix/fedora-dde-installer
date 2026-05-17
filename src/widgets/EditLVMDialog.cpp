#include "EditLVMDialog.h"
#include "globals.h"
#include "CommonFunc.h"
#include "TitleWidget.h"
#include "PartitionSeniorManager.h"

#include <DCheckBox>
#include <QScreen>

EditLVMDialog::EditLVMDialog(const SLVMInfo &info, QWidget *parent):
    DDialog (parent),
    m_editPartitionInfo(info)
{
    m_titleWidget = new TitleWidget(this);
    m_titleWidget->setTitleLabelProperty(::QObject::tr("Edit Partition"), "font-size:16px;font-weight:bold;");
    m_titleWidget->setTipsLabelVisible(false);
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->addStretch();
    titleLayout->addWidget(m_titleWidget);
    titleLayout->addStretch();

    QWidget *pInnerWidget = new QWidget(this);
    m_tipsLabel = new DLabel(this);
    m_tipsLabel->setAlignment(Qt::AlignCenter);
    m_tipsLabel->setFixedSize(420, 40);
    m_tipsLabel->setWordWrap(true);

    m_formatPartition = new DCheckBox(::QObject::tr("Format the partition"));
    m_formatPartition->setCheckState(Qt::CheckState::Checked);
    m_formatPartition->setEnabled(!m_editPartitionInfo.path.isEmpty());
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
    m_fileSystemCmb->setCurrentText(m_editPartitionInfo.filesystem);
    connect(m_fileSystemCmb, &DComboBox::currentTextChanged, this, [=](const QString &text){
        if (text == "linux-swap" || text == "lvm2 pv") {
            m_mountPointCmb->setEnabled(false);
            m_mountPointCmb->setCurrentText(CommonFunc::getPartitionMountText(""));
        } else if(text == "vfat") {
            m_mountPointCmb->setCurrentText(CommonFunc::getPartitionMountText("/boot/efi"));
        } else {
            m_mountPointCmb->setEnabled(true);
        }
        m_formatPartition->setChecked(info.filesystem != text);
        m_formatPartition->setEnabled(info.filesystem == text);
        m_editPartitionInfo.filesystem = text;
        m_tipsLabel->setText(QString());
        updateErrTips(m_formatPartition->checkState() == Qt::CheckState::Checked ? formattedTips : "");
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
    m_mountPointCmb->addItems(CommonFunc::getPartitionMountPoints(m_editPartitionInfo));
    if (m_editPartitionInfo.filesystem == "linux-swap" || m_editPartitionInfo.filesystem == "lvm2 pv") {
        m_mountPointCmb->setEnabled(false);
        m_mountPointCmb->setCurrentText(CommonFunc::getPartitionMountText(""));
    } else {
        m_mountPointCmb->setEnabled(true);
        m_mountPointCmb->setCurrentText(CommonFunc::getPartitionMountText(m_editPartitionInfo.mountPoint));
    }
    connect(m_mountPointCmb, &DComboBox::currentTextChanged, this, [=](const QString &text){
        m_editPartitionInfo.mountPoint = CommonFunc::getPartitionMountPoint(text);
        m_tipsLabel->setText(QString());
        m_formatPartition->setCheckState(Qt::CheckState::Checked);
        m_formatPartition->setEnabled(!m_editPartitionInfo.path.isEmpty());
        if(text == "/boot/efi") {
            m_fileSystemCmb->setCurrentText("vfat");
        } else if (gNeedFormatMountPoints.contains(text)) {
            m_formatPartition->setEnabled(false);
        } else if (text == "/home") {
            m_formatPartition->setChecked(false);
        }
        m_editPartitionInfo.filesystem = m_fileSystemCmb->currentText();
        updateErrTips(m_formatPartition->checkState() == Qt::CheckState::Checked ? formattedTips : ""); 
    });

    QHBoxLayout *pTypeLayout = new QHBoxLayout;
    pTypeLayout->setSpacing(glayoutSpacing);
    pTypeLayout->addStretch();
    pTypeLayout->addWidget(m_mountPointLabel, 0, Qt::AlignLeft);
    pTypeLayout->addWidget(m_mountPointCmb, 0, Qt::AlignRight);
    pTypeLayout->addStretch();

    m_cancelBtn = new DPushButton(::QObject::tr("Cancel", "button"), this);
    m_cancelBtn->setFixedSize(200, 36);
    connect(m_cancelBtn, &DSuggestButton::clicked, this, &DDialog::close);

    m_okBtn = new DSuggestButton(::QObject::tr("Confirm", "button"), this);
    m_okBtn->setFixedSize(200, 36);
    connect(m_okBtn, &DSuggestButton::clicked, this, [=]{
        QString err = QString();
        m_editPartitionInfo.filesystem = m_fileSystemCmb->currentText();
        m_editPartitionInfo.mountPoint = CommonFunc::getPartitionMountPoint(m_mountPointCmb->currentText());
        if (PartitionSeniorManager::instance()->isExistSameMountpointAsThePrimaryPartition(m_editPartitionInfo.mountPoint)) {
            updateErrTips(QString(::QObject::tr("The %1 mount point has been used, please select another one")).arg(m_editPartitionInfo.mountPoint));
            return;
        }

        if (!PartitionSeniorManager::instance()->checkDialogNextEnable(m_editPartitionInfo, err)) {
            updateErrTips(err);
            return;
        }
        updateErrTips(err);
        accept();
    });

    QHBoxLayout *pHLayout = new QHBoxLayout;
    pHLayout->addStretch();
    pHLayout->addWidget(m_cancelBtn);
    pHLayout->addSpacing(15);
    pHLayout->addWidget(m_okBtn);
    pHLayout->addStretch();

    QVBoxLayout *pVLayout = new QVBoxLayout;
    pVLayout->setContentsMargins(0, 0, 0, 0);
    pVLayout->setSpacing(0);
    pVLayout->addLayout(titleLayout);
    pVLayout->addSpacing(glayoutSpacing);
    pVLayout->addStretch();
    pVLayout->addLayout(pFileSystemLayout);
    pVLayout->addSpacing(glayoutSpacing);
    pVLayout->addLayout(pTypeLayout);
    pVLayout->addStretch();
    pVLayout->addSpacing(glayoutSpacing * 2);
    pVLayout->addLayout(formatPartitionHLayout);
    pVLayout->addWidget(m_tipsLabel, Qt::AlignHCenter);
    pVLayout->addSpacing(glayoutSpacing);
    pVLayout->addLayout(pHLayout);
    pVLayout->addStretch();

    QHBoxLayout *pMainLayout = new QHBoxLayout;
    pMainLayout->setSpacing(0);
    pMainLayout->setContentsMargins(0, 0, 0, 0);
    pMainLayout->addLayout(pVLayout);
    pInnerWidget->setLayout(pMainLayout);

    addContent(pInnerWidget);
    setFixedSize(440, 320);

    this->setOnButtonClickedClose(true);

    move(qApp->primaryScreen()->availableSize().width() / 2 - width() / 2,
         qApp->primaryScreen()->availableSize().height() / 2 - height() / 2);
}

SLVMInfo EditLVMDialog::getPartition() const
{
    return m_editPartitionInfo;
}

void EditLVMDialog::updateErrTips(const QString &err)
{
    m_tipsLabel->setStyleSheet(QString("QLabel{color:%1;}").arg("#FF7400"));
    m_tipsLabel->setText(err);
}

QStringList EditLVMDialog::getFileSystemList()
{
    QStringList list = CommonFunc::getSettingsStringList("DI_LVM_FILESYSTEM_CONFIG");
    return list;
}

bool EditLVMDialog::getFormatPartitionCheckStatus()
{
    return m_formatPartition->isChecked();
}
