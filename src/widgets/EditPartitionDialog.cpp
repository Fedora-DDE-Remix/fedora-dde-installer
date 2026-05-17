#include "EditPartitionDialog.h"
#include "globals.h"
#include "CommonFunc.h"
#include "TitleWidget.h"
#include "PartitionSeniorManager.h"

#include <DCheckBox>
#include <QScreen>

EditPartitionDialog::EditPartitionDialog(const SPartitionInfo &info, QWidget *parent):
    DDialog (parent),
    m_editPartitionInfo(info)
{
    setOnButtonClickedClose(false);
    setTitle(::QObject::tr("Edit Partition"));
    QWidget *pInnerWidget = new QWidget(this);
    m_tipsLabel = new DLabel(this);
    m_tipsLabel->setAlignment(Qt::AlignCenter);
    m_tipsLabel->setFixedSize(420, 40);
    m_tipsLabel->setWordWrap(true);

    m_formatPartition = new DCheckBox(::QObject::tr("Format the partition"));
    m_formatPartition->setCheckState(Qt::CheckState::Checked);
    // path 为空说明是新建的分区，需要强制格式化
    m_formatPartition->setEnabled(!m_editPartitionInfo.path.isEmpty());
    if (gNeedFormatMountPoints.contains(m_editPartitionInfo.mountPoint)) {
        m_formatPartition->setEnabled(false);
    }
    //m_formatPartition->setCheckState(info.isNeedFormat ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    m_formattedTips = ::QObject::tr("This partition will be formatted. Make sure you have backed up all data.");
    updateErrTips(m_formatPartition->checkState() == Qt::CheckState::Checked ? m_formattedTips : "");
    connect(m_formatPartition, &DCheckBox::clicked, this, [=] {
        updateErrTips(m_formatPartition->checkState() == Qt::CheckState::Checked ? m_formattedTips : "");
    });
    QHBoxLayout *formatPartitionHLayout = new QHBoxLayout;
    formatPartitionHLayout->addStretch();
    formatPartitionHLayout->addWidget(m_formatPartition);
    formatPartitionHLayout->addStretch();

    m_fileSystemLabel = new DLabel(this);
    m_fileSystemLabel->setFixedSize(80, 36);
    m_fileSystemLabel->setText(::QObject::tr("File system"));
    m_fileSystemCmb = new DComboBox(this);
    m_fileSystemCmb->setFixedSize(320, 36);
    m_fileSystemCmb->addItems(getFileSystemList());
    m_fileSystemCmb->setCurrentText(CommonFunc::getFileSystem(m_editPartitionInfo.filesystem));
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
        updateErrTips(m_formatPartition->checkState() == Qt::CheckState::Checked ? m_formattedTips : "");
    });

    QHBoxLayout *pFileSystemLayout = new QHBoxLayout;
    pFileSystemLayout->setContentsMargins(0, 0, 0, 0);
    pFileSystemLayout->setSpacing(glayoutSpacing);
    pFileSystemLayout->addWidget(m_fileSystemLabel, 0, Qt::AlignLeft);
    pFileSystemLayout->addWidget(m_fileSystemCmb, 0, Qt::AlignRight);

    m_mountPointLabel = new DLabel(this);
    m_mountPointLabel->setFixedSize(80, 36);
    m_mountPointLabel->setText(::QObject::tr("Mount point"));
    m_mountPointCmb = new DComboBox(this);
    m_mountPointCmb->setFixedSize(320, 36);
    m_mountPointCmb->addItems(getMountPointList());
    m_mountPointCmb->setCurrentText(CommonFunc::getPartitionMountText(m_editPartitionInfo.mountPoint));
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
        updateErrTips(m_formatPartition->checkState() == Qt::CheckState::Checked ? m_formattedTips : "");
    });

    QHBoxLayout *pTypeLayout = new QHBoxLayout;
    pTypeLayout->setContentsMargins(0, 0, 0, 0);
    pTypeLayout->setSpacing(glayoutSpacing);
    pTypeLayout->addWidget(m_mountPointLabel, 0, Qt::AlignLeft);
    pTypeLayout->addWidget(m_mountPointCmb, 0, Qt::AlignRight);

    QVBoxLayout *pVLayout = new QVBoxLayout;
    pVLayout->setContentsMargins(0, 0, 0, 0);
    pVLayout->setSpacing(0);
    pVLayout->addSpacing(glayoutSpacing);
    pVLayout->addStretch();
    pVLayout->addLayout(pFileSystemLayout);
    pVLayout->addSpacing(glayoutSpacing);
    pVLayout->addLayout(pTypeLayout);
    pVLayout->addStretch();
    pVLayout->addSpacing(glayoutSpacing * 2);
    pVLayout->addLayout(formatPartitionHLayout);
    pVLayout->addWidget(m_tipsLabel, Qt::AlignHCenter);
    pVLayout->addStretch();

    QHBoxLayout *pMainLayout = new QHBoxLayout;
    pMainLayout->setSpacing(0);
    pMainLayout->setContentsMargins(0, 0, 0, 0);
    pMainLayout->addLayout(pVLayout);
    pInnerWidget->setLayout(pMainLayout);

    addContent(pInnerWidget);
    addButton(::QObject::tr("Cancel", "button"));
    addButton(::QObject::tr("Confirm", "button"), true, DDialog::ButtonRecommend);
    m_cancelBtn = getButton(0);
    m_okBtn = getButton(1);
    connect(m_cancelBtn, &QAbstractButton::clicked, this, [=]{updateErrTips(""); reject();});
    connect(this, &DDialog::closed, this, [=]{updateErrTips(""); reject();});
    connect(m_okBtn, &QAbstractButton::clicked, this, [=]{
        QString err = QString();
        if (!PartitionSeniorManager::instance()->checkDialogNextEnable(m_editPartitionInfo, err)) {
            updateErrTips(err);
            return;
        }

        if (m_formatPartition->isChecked()) {
            m_editPartitionInfo.label = ""; // 勾选了格式化分区，则将label置空，后面根据挂载点确定label的内容
        }

        updateErrTips(err);
        accept();
    });

    setFixedWidth(440);

    move(qApp->primaryScreen()->availableSize().width() / 2 - width() / 2,
         qApp->primaryScreen()->availableSize().height() / 2 - height() / 2);
}

SPartitionInfo EditPartitionDialog::getPartition() const
{
    return m_editPartitionInfo;
}

bool EditPartitionDialog::getFormatPartitionCheckStatus()
{
    return m_formatPartition->isChecked();
}

void EditPartitionDialog::updateErrTips(const QString &err)
{
    m_tipsLabel->setStyleSheet(QString("QLabel{color:%1;}").arg("#FF7400"));
    m_tipsLabel->setText(err);
}

QStringList EditPartitionDialog::getFileSystemList()
{
    QStringList list = CommonFunc::getSettingsStringList("DI_PARTITION_FILESYSTEM_CONFIG");
    if (CommonFunc::isUEFI() && !list.contains("vfat"))  {
        if(canUseEFI(m_editPartitionInfo)) {
            list.append("vfat");
        }
    }

    return list;
}

QStringList EditPartitionDialog::getMountPointList()
{
    QStringList list = CommonFunc::getPartitionMountPoints(m_editPartitionInfo);

    // 被选中的分区大小不满足efi分区的条件，则剔除efi的挂载点
    if(!canUseEFI(m_editPartitionInfo)) {
        list.removeAll("/boot/efi");
    }

    return list;
}


bool EditPartitionDialog::canUseEFI(const SPartitionInfo &info)
{
    if (info.isEFIPartition) {
        return true;
    }

    qint64 efiMinSize = CommonFunc::getSettingsInt("DI_EFI_SIZE_CONFIG");
    qint64 efiMaxSize = qMin(info.size, (qint64)4 * 1024);
    if((info.size < efiMinSize && qAbs(efiMinSize - info.size) > gPartitionDelta)
        || (info.size > efiMaxSize && qAbs(efiMaxSize - info.size) > gPartitionDelta)) {
        return false;
    } else {
        return true;
    }
}

