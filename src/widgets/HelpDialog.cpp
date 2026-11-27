#include "HelpDialog.h"
#include "CommonFunc.h"
#include <DTipLabel>
#include <DFontSizeManager>

static int DIALOG_WIDTH = 600;
static int SENIOR_TIPDLG_WIDTH = 400;
static int BACKUP_TIPDLG_WIDTH = 400;
static int AUTO_DECRYPTDLG_WIDTH = 360;
static int LABEL_SAPCE = 2;

HelpDialog::HelpDialog(QWidget *parent): DBlurEffectWidget (parent), m_dlgWidth(DIALOG_WIDTH - 200)
{
    initUI();
}

HelpDialog::HelpDialog(int width, QWidget *parent): DBlurEffectWidget (parent), m_dlgWidth(width)
{
    initUI();
}

void HelpDialog::setContentText()
{
    addHelpText(QString("%1").arg(::QObject::tr("Keep user data")),
                QString(::QObject::tr("If you have previously installed %1, "
                                      "not enabled disk encryption and the lvm file system, "
                                      "and the configuration this time is the same as before, "
                                      "you can keep the old user data.").arg(CommonFunc::isNameDeepin())));
}

void HelpDialog::updateTs()
{
    QLayoutItem *child = nullptr;
    while ((child = mainLayout->takeAt(0)) != nullptr) {
        if(child->widget()) {
            child->widget()->setParent(nullptr);
            delete  child->widget();
        }

        delete child;
    }

    resize(m_dlgWidth, 40);
    setContentText();
}

void HelpDialog::addHelpText(const QString &title, const QString &description)
{
    int addHeight = 0;
    if(mainLayout->count() > 0) {
        mainLayout->addSpacing(10);
        addHeight += 10;
    }

    DLabel *titleLabel = new DLabel;
    QPalette titlePalette = titleLabel->palette();
    titlePalette.setColor(QPalette::WindowText, QColor("#CC000000"));
    titleLabel->setPalette(titlePalette);
    DLabel *despLabel = new DLabel;
    QPalette despPalette = despLabel->palette();
    despPalette.setColor(QPalette::WindowText, QColor("#99000000"));
    despLabel->setPalette(despPalette);
    DFontSizeManager::instance()->bind(titleLabel, DFontSizeManager::T6, QFont::DemiBold);
    DFontSizeManager::instance()->bind(despLabel, DFontSizeManager::T6);
    titleLabel->setWordWrap(true);
    titleLabel->setText(title);
    titleLabel->setAlignment(Qt::AlignVCenter);
    addHeight += titleLabel->fontMetrics().boundingRect(0, 0, m_dlgWidth - 40, 0, Qt::TextWordWrap, title).height();
    addHeight += LABEL_SAPCE;

    despLabel->setWordWrap(true);
    despLabel->setAlignment(Qt::AlignLeading | Qt::AlignVCenter);
    despLabel->setText(description);
    addHeight += despLabel->fontMetrics().boundingRect(0, 0, m_dlgWidth - 40, 0, Qt::TextWordWrap, description).height();

    resize(width(), height() + addHeight);
    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(LABEL_SAPCE);
    mainLayout->addWidget(despLabel);
}

void HelpDialog::showCenter(QWidget *showWidget)
{
    if(showWidget && showWidget->isVisible()) {
        QPoint pos = showWidget->mapToGlobal(showWidget->rect().center());
        pos -= QPoint(0, showWidget->height() / 2 + 10 + this->height() / 2);

        QRect qr = geometry();
        qr.moveCenter(pos);
        move(qr.topLeft());
        this->show();
    }
}

void HelpDialog::initUI()
{
    setBlendMode(BehindWindowBlend);
    setBlurRectXRadius(10);
    setBlurRectYRadius(10);
    setWindowFlag(Qt::Tool);
    setFixedWidth(m_dlgWidth);
    setMinimumHeight(40);
    resize(m_dlgWidth, 40);

    mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(0);
    setLayout(mainLayout);
}

SeniorTipDialog::SeniorTipDialog(QWidget *parent): HelpDialog (SENIOR_TIPDLG_WIDTH, parent)
{
    ;
}

void SeniorTipDialog::setContentText()
{
    QString efiSize = QString::number(CommonFunc::getSettingsInt("DI_EFI_SIZE_CONFIG")) + "MiB";
    QString bootSize = QString::number(CommonFunc::getSettingsInt("DI_BOOT_SIZE_CONFIG") * 1.0 / 1024) + "GiB";
    QString rootSize = QString::number(CommonFunc::getSettingsInt("DI_ROOTA_SIZE_CONFIG") * 1.0 / 1024) + "GiB";
    if (CommonFunc::isUEFI()) {
        addHelpText(::QObject::tr("/boot/efi Partition, Boot Partition"), QString(::QObject::tr("File System:%1\n").arg("vfat")) + QString(::QObject::tr("Size:At least %1\n").arg(efiSize)));
    } else {
        addHelpText(::QObject::tr("BIOS boot partition (GPT)"),
                    ::QObject::tr("At least 1 MiB, with no filesystem or mount point. Not needed on an existing MBR disk."));
    }
    addHelpText(::QObject::tr("/boot Partition, Boot Partition"), QString(::QObject::tr("File System:%1\n").arg("ext4")) + QString(::QObject::tr("Size:At least %1\n").arg(bootSize)));
    addHelpText(::QObject::tr("/ Partition, Root Directory"),
                ::QObject::tr("Btrfs by default; ext4 and XFS are also supported.\nRoot and /home share Btrfs space unless /home is separate.\n")
                + ::QObject::tr("Size: At least %1\n").arg(rootSize));
    addHelpText(::QObject::tr("Swap (optional)"),
                ::QObject::tr("zram provides swap by default. A linux-swap partition is optional and has no mount point."));
    addHelpText(::QObject::tr("System encryption (LUKS2)"),
                ::QObject::tr("Keep /home and other personal data inside the encrypted root; separate data and swap partitions are not supported with encryption. /boot and the EFI System Partition remain unencrypted. A passphrase is required at every start.")
                + "\n" + ::QObject::tr("Reserve an additional 32 MiB in the root partition for LUKS2 metadata."));

}

BackupTipDialog::BackupTipDialog(QWidget *parent):HelpDialog (BACKUP_TIPDLG_WIDTH, parent)
{
    ;
}

void BackupTipDialog::setContentText()
{
    addHelpText(::QObject::tr("Create Init Backup:"), ::QObject::tr("Init backup can be use to restore data.\nCreate init backup cost more time."));
}

AutoDecryptDialog::AutoDecryptDialog(QWidget *parent):HelpDialog (AUTO_DECRYPTDLG_WIDTH, parent)
{
    ;
}

void AutoDecryptDialog::setContentText()
{
    addHelpText(tr("AutoDecrypt:"), tr("Your device supports TPM/TCM enables automatic decryption. You don't need to enter password when the system setup."));
}

OtherPartitionDialog::OtherPartitionDialog(QWidget *parent) : HelpDialog(400, parent)
{

}

void OtherPartitionDialog::setContentText()
{
    QString description = ::QObject::tr("/boot: Separate ext4 boot partition.\n");
    if (CommonFunc::isUEFI())
        description += ::QObject::tr("/boot/efi: System boot partition, stores boot files.\n");
    else
        description += ::QObject::tr("BIOS boot: An unformatted 1 MiB area on GPT disks.\n");
    description += ::QObject::tr("Swap uses zram by default. An on-disk swap partition is optional.");
    addHelpText(::QObject::tr("Other partitions include:"), description);
}
