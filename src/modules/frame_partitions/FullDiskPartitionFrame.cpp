#include "FullDiskPartitionFrame.h"
#include "TitleWidget.h"
#include "DiskinfoWidget.h"
#include "globals.h"
#include "DiskinfoWidget.h"
#include "DeviceManager.h"
#include "PartitionOperateManager.h"
#include "PartitionFullDiskManager.h"
#include "PartitionSeniorManager.h"
#include "PasswdDialog.h"
#include "CommonFunc.h"
#include "DiskDeviceButton.h"
#include "PasswdDialog.h"
#include "SizeEditDialog.h"
#include "HelpDialog.h"
#include "FormatDiskTipsDialog.h"
#include "UserDataDialog.h"
#include "UseLvmLogicDialog.h"
#include "HelpLabel.h"
#include "PartitionDecryptDialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QMouseEvent>
#include <QApplication>
#include <QDebug>
#include <DSysInfo>
#include <DFrame>
#include <DBlurEffectWidget>
#include <DFontSizeManager>
#include <QtMath>
#include <QScrollArea>

DCORE_USE_NAMESPACE
static const int CENTER_WIDTH = 600;

FullDiskPartitionFrame::FullDiskPartitionFrame(QWidget *parent):PartitionFrameBase (parent)
{
    // 标题栏
    m_titleWidget = new TitleWidget(this);
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->addWidget(m_titleWidget);

    // 安装提示信息
    auto tipLayout = new QHBoxLayout;
    tipLayout->setAlignment(Qt::AlignCenter);
    m_tipLabel = new DLabel(this);
    tipLayout->addWidget(m_tipLabel);
    m_tipLabel->setIndent(10);
    m_tipLabel->setAlignment(Qt::AlignCenter);
    m_tipLabel->setForegroundRole(DPalette::TextWarning);

    m_sizeEditDialog = new SizeEditDialog;
    // 确认按钮
    m_nextBtn = new DSuggestButton;
    m_nextBtn->setFixedSize(gnextbuttonwidth, gnextbuttonheight);
    m_nextBtn->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_nextBtn->setDefault(true);
    QHBoxLayout *nextLayout = new QHBoxLayout;
    nextLayout->setSpacing(0);
    nextLayout->addStretch();
    nextLayout->addWidget(m_nextBtn);
    nextLayout->addStretch();
    connect(m_nextBtn, &DSuggestButton::clicked, this, &FullDiskPartitionFrame::onNextBtnClicked);

    // 主布局
    m_mainlayout = new QVBoxLayout;
    m_mainlayout->setContentsMargins(0, 0, 0, 0);
    m_mainlayout->setSpacing(0);
    m_mainlayout->addLayout(titleLayout);
    m_mainlayout->addSpacing(CommonFunc::isBigScreen() ? 60 : 10);
    initContent();
    m_mainlayout->addSpacing(10);
    m_mainlayout->addLayout(tipLayout);
    m_mainlayout->addSpacing(10);
    m_mainlayout->addLayout(nextLayout);
    setLayout(m_mainlayout);
    //
    initConnection();
}

void FullDiskPartitionFrame::updateTs()
{
    m_titleWidget->setTitleLabelProperty(::QObject::tr("Full Disk"), "");
    m_titleWidget->setTipsLabelProperty(::QObject::tr("The system will be installed on the disk you selected. All data on the selected disk (including system and data partitions) will be formatted."), "font-size:16px;font-weight:Medium;");
    m_sysInfo->setText(tr("System"));
    m_dataInfo->setText(tr("Data"));
    m_otherInfo->setText(tr("Other partitions"));
    m_sysInfo->setToolTip(m_sysInfo->text());
    m_dataInfo->setToolTip(m_dataInfo->text());
    m_otherInfo->setToolTip(m_otherInfo->text());

    int spacinValue = m_btnDefault->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
    m_btnDefault->setText(CommonFunc::wrapWords(tr("Default"), m_btnDefault->width() - m_btnDefault->iconSize().width() - spacinValue, m_btnDefault->fontMetrics()));
    m_btnEncrypt->setText(CommonFunc::wrapWords(tr("Encrypt disk"), m_btnEncrypt->width() - m_btnEncrypt->iconSize().width() - spacinValue, m_btnEncrypt->fontMetrics()));
    m_btnExtand->setText(CommonFunc::wrapWords(tr("Enable Volume Extension"), m_btnExtand->width() - m_btnExtand->iconSize().width() - spacinValue, m_btnExtand->fontMetrics()));
    m_btnKeepData->setText(CommonFunc::wrapWords(tr("keep user data"), m_btnKeepData->width() - m_btnKeepData->iconSize().width() - spacinValue, m_btnKeepData->fontMetrics()));

    m_labelTip->setText(tr("Total size"));
    m_installLabel->setText(tr("Select a install mode"));

    m_nextBtn->setText(::QObject::tr("Next"));
    m_sizeEditDialog->updateTs();
    m_tipLabel->setText(m_tipLabel->text());
    m_otherPartitionDialog->updateTs();
    m_modeTip_1->setText(tr("Creates boot partitions and a compressed Btrfs filesystem. Root and /home share the remaining space. Swap uses zram. All data on the selected disk will be erased."));
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
                                       "as this installation configuration, then you can keep your data (/data/home/).").arg(CommonFunc::isNameDeepin()));
    m_modeTip_4->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_modeTip_4->adjustSize();
    m_modeTip_5->setText(::QObject::tr("User data will be saved during system installation, and the disk encryption password will remain the same"));
    m_modeTip_5->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_modeTip_5->adjustSize();

    m_btnDefault->addHelpText(tr("Default"), m_modeTip_1->text());
    m_btnEncrypt->addHelpText(tr("Encrypt disk"), m_modeTip_2->text());
    m_btnExtand->addHelpText(tr("Enable Volume Extension"), m_modeTip_3->text());
    m_btnKeepData->addHelpText(tr("keep user data"), m_modeTip_4->text());

    m_contentWidget->adjustSize();
}

void FullDiskPartitionFrame::showEvent(QShowEvent *event)
{
    if(m_modeGroup->checkedId() == -1) {
        m_modeGroup->button(1)->setChecked(true);
    }
    setFocus();
    QWidget::showEvent(event);
}

void FullDiskPartitionFrame::onNextBtnClicked()
{
    if (!checkDiskSpace())
        return;
    QString error;
    CommonFunc::setSettingsValue("DI_CRYPT_PASSWORD", "");
    CommonFunc::setSettingsValue("DI_OLD_USERNAME_LIST", "");
    PartitionFullDiskManager::instance()->setPartitionMode(FullDiskMode);
    PartitionFullDiskManager::instance()->resetDisk();
    SDeviceInfo device = m_dev;
    device.isCrypt = false;
    device.isCreateCryptHeader = false;
    if (!PartitionFullDiskManager::instance()->addSystemDisk(device, error)
            || !PartitionSeniorManager::instance()->checkPartition(error)) {
        m_tipLabel->setText(error);
        return;
    }
    Q_EMIT next();
}

void FullDiskPartitionFrame::initContent()
{
    //disk info
    DFrame *diskFrame = new DFrame;
    diskFrame->setLineWidth(0);
    diskFrame->setBackgroundRole(DPalette::ItemBackground);
    diskFrame->setFixedSize(CENTER_WIDTH, 116);
    QHBoxLayout *infoLayout = new QHBoxLayout;
    infoLayout->setContentsMargins(10, 10, 10, 10);
    infoLayout->setSpacing(0);
    diskFrame->setLayout(infoLayout);
    QLabel *iconLabel = new QLabel;
    iconLabel->setPixmap(CommonFunc::renderSVG(":/images/drive-harddisk-128px.svg", QSize(96, 96)));
    infoLayout->addWidget(iconLabel, 0, Qt::AlignLeft);
    QVBoxLayout *textLayout = new QVBoxLayout;
    textLayout->setContentsMargins(8, 20, 10, 20);
    textLayout->setSpacing(10);

    QHBoxLayout *nameLayout = new QHBoxLayout;
    nameLayout->setContentsMargins(0, 0, 0, 0);
    nameLayout->setSpacing(0);
    m_labelDiskName = new DLabel;
    m_labelDiskName->setElideMode(Qt::ElideRight);
    m_labelDiskName->setMaximumWidth(300);
    m_labelDiskSize = new DLabel;
    nameLayout->addWidget(m_labelDiskName, 0, Qt::AlignLeft);
    nameLayout->addStretch();
    nameLayout->addWidget(m_labelDiskSize, 0, Qt::AlignRight);
    DFontSizeManager::instance()->bind(m_labelDiskName, DFontSizeManager::T5, QFont::DemiBold);
    DFontSizeManager::instance()->bind(m_labelDiskSize, DFontSizeManager::T5, QFont::DemiBold);

    QHBoxLayout *tipLayout = new QHBoxLayout;
    tipLayout->setContentsMargins(0, 0, 0, 0);
    tipLayout->setSpacing(0);
    m_labelDiskPath = new DLabel;
    m_labelTip = new DLabel;
    tipLayout->addWidget(m_labelDiskPath, 0, Qt::AlignLeft);
    tipLayout->addStretch();
    tipLayout->addWidget(m_labelTip, 0, Qt::AlignRight);
    DFontSizeManager::instance()->bind(m_labelDiskPath, DFontSizeManager::T7, QFont::Thin);
    DFontSizeManager::instance()->bind(m_labelTip, DFontSizeManager::T7, QFont::Thin);
    textLayout->addLayout(nameLayout);
    textLayout->addLayout(tipLayout);
    infoLayout->addLayout(textLayout);
    //size slider
    m_diskSizeSlider = new SizeSliderWidget;
    m_diskSizeSlider->setFixedSize(CENTER_WIDTH, 30);
    //size info
    QWidget *sizeInfo = new QWidget;
    sizeInfo->setFixedWidth(CENTER_WIDTH);
    sizeInfo->hide();
    m_diskSizeSlider->hide();
    QHBoxLayout *sizeInfoLayout = new QHBoxLayout;
    sizeInfoLayout->setContentsMargins(0, 0, 0, 0);
    sizeInfoLayout->setSpacing(10);
    sizeInfo->setLayout(sizeInfoLayout);

    RoundWidget *sysHeader = new RoundWidget;
    sysHeader->setStyleSheet("background-color:#0081FF");
    RoundWidget *dataHeader = new RoundWidget;
    dataHeader->setStyleSheet("background-color:#44D7B6");
    RoundWidget *otherHeader = new RoundWidget;
    otherHeader->setStyleSheet("background-color:#D8D8D8");
    m_sysInfo = new DLabel;
    m_dataInfo = new DLabel;
    m_otherInfo = new DLabel;
    m_helpLabel = new HelpLabel;
    m_otherPartitionDialog = new OtherPartitionDialog(this);
    m_sysInfo->setElideMode(Qt::ElideRight);
    m_dataInfo->setElideMode(Qt::ElideRight);
    m_otherInfo->setElideMode(Qt::ElideRight);
    connect(m_helpLabel, &HelpLabel::hoverEnter, [=]{
        QPoint pos = m_helpLabel->mapToGlobal(m_helpLabel->rect().topRight());
        pos += QPoint(10, -15);
        QRect geRect = m_otherPartitionDialog->geometry();
        geRect.moveBottomRight(pos);
        m_otherPartitionDialog->move(geRect.topLeft());
        m_otherPartitionDialog->show();
    });

    connect(m_helpLabel, &HelpLabel::hoverLeave, [=]{
        m_otherPartitionDialog->hide();
    });

    DTipLabel *sysSizeTip = new DTipLabel("GiB");
    m_dataSizeTip = new DTipLabel;
    m_otherSizeTip = new DTipLabel;
    m_sysSpinBox = new DSpinBox;
    m_sysSpinBox->setFixedSize(80, 36);
    m_sysSpinBox->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
    m_sysSpinBox->setEnabledEmbedStyle(true);
    m_sysSpinBox->setFocusPolicy(Qt::StrongFocus);
    QWidget *sysWidget = new QWidget;
    QHBoxLayout *sysLayout = new QHBoxLayout;
    sysLayout->setSpacing(5);
    sysLayout->setContentsMargins(0, 0, 0, 0);
    sysWidget->setLayout(sysLayout);
    sysLayout->addWidget(sysHeader);
    sysLayout->addWidget(m_sysInfo);
    sysLayout->addWidget(m_sysSpinBox);
    sysLayout->addWidget(sysSizeTip);
    sysLayout->addStretch();

    QWidget *dataWidget = new QWidget;
    QHBoxLayout *dataLayout = new QHBoxLayout;
    dataLayout->setSpacing(5);
    dataLayout->setContentsMargins(0, 0, 0, 0);
    dataWidget->setLayout(dataLayout);

    dataLayout->addWidget(dataHeader);
    dataLayout->addWidget(m_dataInfo);
    dataLayout->addWidget(m_dataSizeTip);
    dataLayout->addStretch();

    QWidget *otherWidget = new QWidget;
    QHBoxLayout *otherLayout = new QHBoxLayout;
    otherLayout->setSpacing(5);
    otherLayout->setContentsMargins(0, 0, 0, 0);
    otherWidget->setLayout(otherLayout);

    otherLayout->addWidget(otherHeader);
    otherLayout->addWidget(m_otherInfo);
    otherLayout->addWidget(m_otherSizeTip);
    otherLayout->addWidget(m_helpLabel);
    otherLayout->addStretch();

    sizeInfoLayout->addWidget(sysWidget);
    sizeInfoLayout->addStretch();
    sizeInfoLayout->addWidget(dataWidget);
    sizeInfoLayout->addStretch();
    sizeInfoLayout->addWidget(otherWidget);
    //
    m_installLabel = new DLabel;
    m_installLabel->setIndent(10);
    m_installLabel->setFixedWidth(CENTER_WIDTH);
    DFrame *modeFrame = new DFrame;
    modeFrame->setLineWidth(0);
    modeFrame->hide();
    m_installLabel->hide();
    modeFrame->setFixedWidth(CENTER_WIDTH);
    modeFrame->setMinimumHeight(60);
    modeFrame->setBackgroundRole(DPalette::ItemBackground);
    QHBoxLayout *modeLayout = new QHBoxLayout;
    modeLayout->setContentsMargins(20, 0, 20, 0);
    modeFrame->setLayout(modeLayout);
    m_btnDefault = new HelpRadioButton;
    m_btnEncrypt = new HelpRadioButton;
    m_btnEncrypt->hide();
    m_btnEncrypt->setEnabled(false);
    m_btnExtand = new HelpRadioButton;
    m_btnExtand->hide();
    m_btnExtand->setEnabled(false);
    m_btnKeepData = new HelpRadioButton;
    m_btnKeepData->hide();
    m_btnKeepData->setEnabled(false);
    m_modeGroup = new QButtonGroup;
    m_btnDefault->setFixedWidth(120);
    m_btnEncrypt->setFixedWidth(120);
    m_btnExtand->setFixedWidth(160);
    m_btnKeepData->setFixedWidth(160);
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
    m_modeTip_1->setVisible(true);

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

    subContent->addWidget(diskFrame, 0, Qt::AlignHCenter);
    subContent->addSpacing(40);
    subContent->addWidget(m_diskSizeSlider, 0, Qt::AlignHCenter);
    subContent->addSpacing(15);
    subContent->addWidget(sizeInfo, 0, Qt::AlignHCenter);
    subContent->addSpacing(40);
    subContent->addWidget(m_installLabel, 0, Qt::AlignHCenter);
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
    scrollArea->setFixedWidth(CENTER_WIDTH + 20);
    m_contentWidget = new QWidget;
    m_contentWidget->setLayout(subContent);
    scrollArea->setWidget(m_contentWidget);
    m_mainlayout->addWidget(scrollArea, 0, Qt::AlignHCenter);
}

void FullDiskPartitionFrame::initConnection()
{
    // The Fedora layout uses all remaining space for a writable root filesystem.
}

void FullDiskPartitionFrame::resetToDefaultSize()
{
    int sysShowMinSize = m_sysMinSize;
    if (m_dev.size > gDiskThreshold) {
        //大于225G的磁盘取值为磁盘容量的10%
        sysShowMinSize = (int)CommonFunc::mToG(qFloor(m_dev.size * 0.1));
    }
    m_sysSpinBox->setValue(sysShowMinSize);
}

void FullDiskPartitionFrame::initDiskData(const QList<SDeviceInfo> &devs)
{
    if(devs.size() != 1) {
        qCritical() << "FullDiskPartitionFrame has more then one disk input";
    }
    m_dev = devs.first();
    int totalSize = (int)CommonFunc::mToG(m_dev.size);
    m_labelDiskName->setText(m_dev.name);
    m_labelDiskName->setToolTip(m_dev.name);
    m_labelDiskPath->setText(m_dev.device);
    m_labelDiskSize->setText(QString::number(totalSize) + "GiB");
    m_btnKeepData->hide();
    if(m_modeGroup->checkedId() != -1) {
        int id = m_modeGroup->checkedId();
        m_modeGroup->setExclusive(false);
        m_modeGroup->button(id)->setChecked(false);
        m_modeGroup->setExclusive(true);
    }

    m_sysMinSize = (int)CommonFunc::mToG(CommonFunc::getSettingsInt("DI_ROOTA_SIZE_CONFIG"));
    int otherSize = (int)CommonFunc::mToG(PartitionFullDiskManager::instance()->getOtherPartitionSize(m_dev.size));
    int dataMinSize = 0;
    int sysMaxSize = totalSize - otherSize - dataMinSize;
    m_sysSpinBox->setRange(m_sysMinSize, sysMaxSize);

    //init disk data
    int sysShowMinSize = m_sysMinSize;
    if (m_dev.size > gDiskThreshold) {
        //大于225G的磁盘取值为磁盘容量的10%
        sysShowMinSize = (int)CommonFunc::mToG(qFloor(m_dev.size * 0.1));
    }
    m_diskSizeSlider->initPartitionSize(totalSize, sysShowMinSize, otherSize);

    QString err;
    PartitionFullDiskManager::instance()->delSystemDev();
    if(!PartitionFullDiskManager::instance()->addSystemDisk(m_dev, err)) {
        qWarning() << ::QObject::tr("You are using an inappropriate partition configuration file, and cannot install the system with it. Please abort the installation and check the file.");
        qWarning() << "update install info err: " + err;
    }
}

bool FullDiskPartitionFrame::checkDiskSpace()
{
    //校验系统分区大小
    qint64 deviceMinSize = CommonFunc::getSettingsInt("DI_DEVICE_MIN_SIZE_CONFIG") * giDeviceUnitSize;

    if (m_dev.size < deviceMinSize) {
        DDialog dialog;
        dialog.setWindowFlags(Qt::WindowTitleHint);
        dialog.setTitle(::QObject::tr("Insufficient disk space"));
        dialog.setMessage(QString(::QObject::tr("To install this operating system, the system partition "
                                                "requires at least %1 of disk space").arg(QString("%1GiB").arg(CommonFunc::mToG(deviceMinSize)))));
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

bool FullDiskPartitionFrame::checkDiskNeedDecrypt(bool &decryptStatus)
{
    QList<SPartitionInfo> decryptList;
    PartitionFullDiskManager::instance()->getDeviceCryptPartitions(m_dev, decryptList);
    if(!decryptList.isEmpty()) {
        decryptStatus = handleDecryptDisk(decryptList);
    }

    return !decryptList.isEmpty();
}

bool FullDiskPartitionFrame::handleDecryptDisk(const QList<SPartitionInfo> &devList)
{
    PartitionDecryptDialog decryptDlg(devList);
    if(QDialog::Accepted != decryptDlg.exec()) {
        setFocus();
        return false;
    } else {
        setFocus();
        QString err;
        Q_EMIT this->decryptSuccess();
        m_dev = PartitionOperateManager::instance()->getCurrentDevice().first();
        qInfo() << m_dev.toString();
        PartitionFullDiskManager::instance()->delSystemDev();
        PartitionFullDiskManager::instance()->addSystemDisk(m_dev, err);
        return true;
    }
}
