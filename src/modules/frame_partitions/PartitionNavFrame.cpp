#include "PartitionNavFrame.h"
#include "OptionButton.h"
#include "TitleWidget.h"
#include "globals.h"
#include "CommonFunc.h"
#include "FullDiskPartitionFrame.h"
#include "SeniorPartitionFrame.h"
#include "CustomPartitionFrame.h"
#include "SeniorLVMFrame.h"
#include "InstallConfirmDialog.h"
#include "DeviceManager.h"
#include "MultipleDiskPartitionFrame.h"
#include "PartitionOperateManager.h"
#include "SelectFileWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QProcess>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QScrollArea>
#include <QStringList>
#include <DSpinner>
#include <DMessageManager>

static const QString SENIOR_FRAME = "SENIORFRAME";
static const QSize GHOST_TIPSIZE = QSize(20, 20);
static const QStringList NO_IMMUTABLE_PROFILE_KEYS = {
    "DI_IMMUTABLE_SYSTEM",
    "DI_PART_POLICY_FILE",
    "DI_SAVE_USER_DATA_LOAD",
    "DI_IS_SAVE_AUTHORIZED_DATA_CONFIG",
    "DI_IS_INIT_RECOVERY",
    "DI_VALID_DEVICE_MIN_SIZE_CONFIG",
    "DI_DEVICE_MIN_SIZE_CONFIG",
    "DI_BOOT_SIZE_CONFIG",
    "DI_ROOTA_SIZE_CONFIG",
    "DI_SWAP_MAX_SIZE_CONFIG",
    "DI_PERSISTENT_SIZE_CONFIG",
    "DI_DATA_MIN_SIZE_CONFIG"
};

PartitionNavFrame::PartitionNavFrame(QWidget *parent):
    FramePluginInterface (parent)
{
    m_uimgCheckResult.result = false;
    m_frameName = gFramePartition;
    cacheInstallProfileDefaults();
    //分区界面回退按钮特殊处理
    m_backBnt->disconnect(SIGNAL(clicked()));
    m_centerWidget = new QStackedWidget(this);
    // 标题栏
    m_titleWidget = new TitleWidget(this);
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->addWidget(m_titleWidget);

    QSize btnIconSize(partitionNavFrame::kIconWidth, partitionNavFrame::kIconHeight);
    // 选择分区方式
    //全盘安装
    m_fullDiskPartitionBnt = new OptionButton(0, this);
    m_fullDiskPartitionBnt->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_fullDiskPartitionBnt->setObjectName(FULLDISK_PARTTION_BTN);
    m_fullDiskPartitionBnt->setFixedWidth(gpartitionBntWidth);
    m_fullDiskPartitionBnt->setMinimumHeight(setItemHeight());
    m_fullDiskPartitionBnt->setIcon(CommonFunc::renderSVG(":/images/icon_overall.svg", btnIconSize));
    initBtnConnect(*m_fullDiskPartitionBnt);

    m_customPartitionBnt = new OptionButton(0, this);
    m_customPartitionBnt->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_customPartitionBnt->setObjectName(CUSTOM_PARTTION_BTN);
    m_customPartitionBnt->setFixedWidth(gpartitionBntWidth);
    m_customPartitionBnt->setMinimumHeight(setItemHeight());
    m_customPartitionBnt->setIcon(CommonFunc::renderSVG(":/images/icon_custom.svg", btnIconSize));
    //m_customPartitionBnt->setVisible(false);
    initBtnConnect(*m_customPartitionBnt);

    //手动安装
    m_seniorPartitionBnt = new OptionButton(0, this);
    m_seniorPartitionBnt->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_seniorPartitionBnt->setObjectName(SENIOR_PARTTION_BTN);
    m_seniorPartitionBnt->setFixedWidth(gpartitionBntWidth);
    m_seniorPartitionBnt->setMinimumHeight(setItemHeight());
    m_seniorPartitionBnt->setIcon(CommonFunc::renderSVG(":/images/icon_advanced.svg", btnIconSize));
    initBtnConnect(*m_seniorPartitionBnt);

    m_partLabel = new DLabel;
    m_partLabel->setFixedWidth(gpartitionBntWidth);
    DFontSizeManager::instance()->bind(m_partLabel, DFontSizeManager::T6, QFont::DemiBold);
    m_noImmutableCheckBox = new DCheckBox(this);
    m_noImmutableCheckBox->setFixedWidth(gpartitionBntWidth);
    m_noImmutableCheckBox->setVisible(isNoImmutableOptionAvailable());
    QVBoxLayout *optionButtonLayout = new QVBoxLayout;
    optionButtonLayout->setSpacing(CommonFunc::isBigScreen() ? gitemSpacing : glayoutSpacing);
    optionButtonLayout->setContentsMargins(0, 0, 0, 0);
    optionButtonLayout->addWidget(m_partLabel, 0, Qt::AlignHCenter);
    optionButtonLayout->addWidget(m_noImmutableCheckBox, 0, Qt::AlignHCenter);
    optionButtonLayout->addWidget(m_fullDiskPartitionBnt, 0, Qt::AlignHCenter);
    optionButtonLayout->addWidget(m_customPartitionBnt, 0, Qt::AlignHCenter);
    optionButtonLayout->addWidget(m_seniorPartitionBnt, 0, Qt::AlignHCenter);
    QWidget *optionButtonWidget = new QWidget;
    optionButtonWidget->setLayout(optionButtonLayout);

    //
    QList<DButtonBoxButton*> boxBtnList;
    m_modeBox = new DButtonBox;
    DButtonBoxButton *isoMode = new DButtonBoxButton("");
    DButtonBoxButton *ghostMode = new DButtonBoxButton("");
    boxBtnList << isoMode << ghostMode;
    m_modeBox->setButtonList(boxBtnList, true);
    for (int i = 0; i < boxBtnList.size(); ++i) {
        m_modeBox->setId(boxBtnList[i], i);
    }
    //
    m_imgSelect = new QWidget;
    m_imgSelect->setFixedWidth(gpartitionBntWidth);
    QVBoxLayout *imgLayout = new QVBoxLayout;
    imgLayout->setContentsMargins(0, 0, 0, 0);
    imgLayout->setSpacing(5);
    m_imgLabel = new DLabel;
    DFontSizeManager::instance()->bind(m_imgLabel, DFontSizeManager::T6, QFont::DemiBold);
    m_selectImgWidget = new SelectFileWidget(this);
    m_selectImgWidget->setFixedHeight(36);
    m_selectImgWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_selectImgWidget->setBackgroundRole(DPalette::ObviousBackground);

    m_ghostDescLabel = new DTipLabel;
    m_ghostDescLabel->setWordWrap(true);
    m_ghostDescLabel->setAlignment(Qt::AlignLeft);
    m_ghostDescLabel->setForegroundRole(DPalette::PlaceholderText);

    QHBoxLayout *checkLayout = new QHBoxLayout;
    checkLayout->setSpacing(5);
    checkLayout->setContentsMargins(0, 0, 0, 0);
    m_spinner = new DSpinner;
    m_spinner->setFixedSize(GHOST_TIPSIZE);
    m_spinner->setVisible(false);
    checkLayout->addWidget(m_spinner);
    m_checkStatusImage = new DLabel;
    checkLayout->addWidget(m_checkStatusImage);
    m_ghostTipLabel = new DTipLabel;
    m_ghostTipLabel->setWordWrap(true);
    m_ghostTipLabel->setAlignment(Qt::AlignLeft);
    checkLayout->addWidget(m_ghostTipLabel);
    checkLayout->addStretch();

    imgLayout->addWidget(m_imgLabel);
    imgLayout->addWidget(m_ghostDescLabel);
    imgLayout->addWidget(m_selectImgWidget);
    imgLayout->addLayout(checkLayout);
    m_imgSelect->setLayout(imgLayout);
    m_mainPage = new QWidget;
    QVBoxLayout *mainpageLayout = new QVBoxLayout;
    mainpageLayout->setContentsMargins(0, 0, 0, 0);
    mainpageLayout->setSpacing(0);
    mainpageLayout->addLayout(titleLayout);
    mainpageLayout->addSpacing(CommonFunc::isBigScreen() ? 30 : 10);
    mainpageLayout->addWidget(m_modeBox, 0, Qt::AlignHCenter);
    mainpageLayout->addSpacing(CommonFunc::isBigScreen() ? 40 : 10);

    QVBoxLayout *scrollAreaContentLayout = new QVBoxLayout;
    scrollAreaContentLayout->setContentsMargins(0, 0, 0, 0);
    scrollAreaContentLayout->setSpacing(0);
    scrollAreaContentLayout->addWidget(m_imgSelect, 0, Qt::AlignHCenter);
    scrollAreaContentLayout->addSpacing(CommonFunc::isBigScreen() ? 20 : 10);
    scrollAreaContentLayout->addWidget(optionButtonWidget, 0, Qt::AlignHCenter);
    scrollAreaContentLayout->addStretch();

    m_scrollAreaContentWidget = new QWidget;
    m_scrollAreaContentWidget->setLayout(scrollAreaContentLayout);
    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setMaximumWidth(gpartitionBntWidth + 20);
    scrollArea->setWidget(m_scrollAreaContentWidget);

    mainpageLayout->addWidget(scrollArea, 0, Qt::AlignHCenter);
    m_mainPage->setLayout(mainpageLayout);

    // 主布局
    m_centerWidget->addWidget(m_mainPage);
    m_contentLayout->addWidget(m_centerWidget);
    m_contentLayout->addSpacing(CommonFunc::getInnerMargin());

    initPartitionFrame();
    initConnection();
    //生成秘钥
    GenerateRecoveryKey();
}
//磁盘解密后，重新刷新设备信息
void PartitionNavFrame::refreshDeviceInfo()
{
    DeviceManager::instance()->scanDevices(true);
    qInfo() << "refresh device info start....";
    PartitionOperateManager::instance()->setDeviceInfo(DeviceManager::instance()->getDeviceInfos());
    PartitionOperateManager::instance()->setLVMInfo(DeviceManager::instance()->getLVMInfos());
    qInfo() << "refresh device info end!";
}

void PartitionNavFrame::showEvent(QShowEvent *event)
{
    if(m_modeBox->checkedId() == -1) {
        m_modeBox->button(0)->click();
    }

    return FramePluginInterface::showEvent(event);
}

void PartitionNavFrame::initConnection()
{
    connect(m_backBnt, &QAbstractButton::clicked, [=]{
        if(m_centerWidget->currentWidget() == m_mainPage) {
            Q_EMIT back();
        } else if (m_centerWidget->currentWidget()->objectName() == SENIOR_FRAME) {
            SeniorPartitionFrame *seniorDisk = qobject_cast<SeniorPartitionFrame*>(m_centerWidget->currentWidget());
            if(seniorDisk && !seniorDisk->switchBack()) {
                m_centerWidget->setCurrentIndex(0);
            }
        } else {
            m_centerWidget->setCurrentIndex(0);
        }
    });

    for (auto *frame: m_mapPartition.values()) {
        connect(frame, &PartitionFrameBase::next, this, &FramePluginInterface::next);
    }

    connect(m_centerWidget, &QStackedWidget::currentChanged, this, [=](int index){
        if(index == 0) {
            PartitionOperateManager::instance()->resetDisk();
        }
    });

    connect(m_selectImgWidget, &SelectFileWidget::textChanged, this, [=]{
        m_ghostTipLabel->clear();
        m_checkStatusImage->setVisible(false);
        clearGhostSetting();
        QString uimgFilePath = m_selectImgWidget->getSelectFilePath();
        if(uimgFilePath.isEmpty()) {
            m_checkStatusImage->setPixmap(QIcon::fromTheme("icon_fail").pixmap(GHOST_TIPSIZE));
            m_ghostTipLabel->setText(tr("Img is not exits"));
            return;
        }
        m_spinner->setVisible(true);
        m_spinner->start();
        m_ghostTipLabel->setForegroundRole(DPalette::TextTips);
        m_ghostTipLabel->setText(tr("Verifying uimg file..."));
        m_selectImgWidget->setEnabled(false);

        auto *watcher = new QFutureWatcher<CheckResult>(this);
        connect(watcher, &QFutureWatcher<bool>::finished, [this, watcher] {
            m_spinner->stop();
            m_selectImgWidget->setEnabled(true);
            m_spinner->setVisible(false);
            m_checkStatusImage->setVisible(true);
            m_uimgCheckResult = watcher->result();
            if (watcher->result().result) {
                m_ghostTipLabel->setForegroundRole(DPalette::TextTips);
                if (!watcher->result().warning.isEmpty()) {
                    m_checkStatusImage->setPixmap(CommonFunc::renderSVG(":/images/interaction_warning.svg", GHOST_TIPSIZE - QSize(4, 4)));
                    m_ghostTipLabel->setText(watcher->result().warning);
                } else {
                    m_checkStatusImage->setPixmap(QIcon::fromTheme("icon_success").pixmap(GHOST_TIPSIZE));
                    m_ghostTipLabel->setText(tr("Successfully verified uimg file, can be installed"));
                }
            } else {
                m_checkStatusImage->setPixmap(QIcon::fromTheme("icon_fail").pixmap(GHOST_TIPSIZE));
                m_ghostTipLabel->setForegroundRole(DPalette::TextWarning);
                m_ghostTipLabel->setText(watcher->result().error);
            }
            watcher->deleteLater();
        });

        QFuture<CheckResult> future = QtConcurrent::run([this, uimgFilePath]() {
            return this->checkUImg(uimgFilePath);
        });
        watcher->setFuture(future);
    });

    connect(m_modeBox, &DButtonBox::buttonToggled, this, [=](QAbstractButton *btn, bool checked){
        if(checked) {
            int id = m_modeBox->id(btn);
            if(id == 0) {
                m_imgSelect->setVisible(false);
                m_partLabel->setVisible(false);
                clearGhostSetting();
                m_noImmutableCheckBox->setVisible(isNoImmutableOptionAvailable());
            } else {
                m_imgSelect->setVisible(true);
                m_partLabel->setVisible(true);
                m_noImmutableCheckBox->setChecked(false);
                m_noImmutableCheckBox->setVisible(false);
            }
            m_scrollAreaContentWidget->adjustSize();
        }
    });

    connect(m_noImmutableCheckBox, &DCheckBox::toggled, this, [=]{
        applyNoImmutableChoice();
        PartitionOperateManager::instance()->resetDisk();
    });
}

void PartitionNavFrame::initPartitionFrame()
{
    qInfo() << "set device info start....";
    PartitionOperateManager::instance()->setDeviceInfo(DeviceManager::instance()->getDeviceInfos());
    PartitionOperateManager::instance()->setLVMInfo(DeviceManager::instance()->getLVMInfos());
    qInfo() << "set device info end!";

    QList<SDeviceInfo> devs = PartitionOperateManager::instance()->getCurrentDevice();
    QMap<SDeviceInfo, QList<SPartitionInfo>> devInfos = PartitionOperateManager::instance()->getCurrentDeviceInfo();
    QMap<SVGInfo, QList<SLVMInfo>> lvms = PartitionOperateManager::instance()->getCurrentLVMInfo();

    FullDiskPartitionFrame *fullDisk = new FullDiskPartitionFrame(this);
    connect(fullDisk, &FullDiskPartitionFrame::decryptSuccess, this, &PartitionNavFrame::refreshDeviceInfo);
    m_mapPartition.insert(PartitionType::FULLDISK_PART, fullDisk);
    m_centerWidget->addWidget(fullDisk);

    MultipleDiskPartitionFrame *multiDisk = new MultipleDiskPartitionFrame(this);
    connect(multiDisk, &MultipleDiskPartitionFrame::decryptSuccess, this, &PartitionNavFrame::refreshDeviceInfo);
    m_mapPartition.insert(PartitionType::MULTIDISK_PART, multiDisk);
    m_centerWidget->addWidget(multiDisk);

    CustomPartitionFrame *customDisk = new CustomPartitionFrame(devInfos, lvms, this);
    m_mapPartition.insert(PartitionType::CUSTOM_PART, customDisk);
    m_centerWidget->addWidget(customDisk);

    SeniorPartitionFrame *seniorDisk = new SeniorPartitionFrame(this);
    seniorDisk->setObjectName(SENIOR_FRAME);
    m_mapPartition.insert(PartitionType::SENIOR_PART, seniorDisk);
    m_centerWidget->addWidget(seniorDisk);
}

int PartitionNavFrame::setItemHeight()
{
    return CommonFunc::isBigScreen() ? gpartitionBntHeight : gpartitionBntHeight - 15;
}

void PartitionNavFrame::updateTs()
{
    m_titleWidget->setTitleLabelProperty(::QObject::tr("Installation Methods"), "");
    m_titleWidget->setTipsLabelProperty(::QObject::tr("Choose a way to install the system."), "font-size:16px;font-weight:Medium;");
    m_titleWidget->setTipsLabelVisible(CommonFunc::isBigScreen());

    m_modeBox->button(0)->setText(tr("Install  ISO"));
    m_modeBox->button(1)->setText(tr("Install uimg"));

    m_noImmutableCheckBox->setText(tr("Install without immutable system"));

    m_fullDiskPartitionBnt->setInstallTypeText(::QObject::tr("Full Disk (Recommended)"));
    m_fullDiskPartitionBnt->setIntroductionText(::QObject::tr("The system will be installed on the disk you selected. All data on the selected disk (including system and data partitions) will be formatted."));

    m_customPartitionBnt->setInstallTypeText(::QObject::tr("Custom"));
    m_customPartitionBnt->setIntroductionText(::QObject::tr("Choose a disk partition to install the system, while the data on other partitions will be preserved."));

    m_seniorPartitionBnt->setInstallTypeText(::QObject::tr("Advanced"));
    m_seniorPartitionBnt->setIntroductionText(::QObject::tr("You can freely allocate disk space, configure file systems and mount points, etc."));

    m_ghostDescLabel->setText(::QObject::tr("Please create a folder name 'clone' on the USB driver, "
                                            "and move the uimg file into the folder, will automatically scan uimg files in the directory."));

    m_imgLabel->setText(::QObject::tr("please select uimg file"));

    m_partLabel->setText(::QObject::tr("Select partition method"));

    m_selectImgWidget->updateTs();
    for (auto *partFrame: m_mapPartition.values()) {
        partFrame->updateTs();
    }

    QTimer::singleShot(100, m_scrollAreaContentWidget, &QWidget::adjustSize);
}

bool PartitionNavFrame::isNoImmutableOptionAvailable() const
{
    return CommonFunc::getCurrentOsVersion() == "V25" && CommonFunc::getSettingsBool("DI_NO_IMMUTABLE_LOAD");
}

void PartitionNavFrame::cacheInstallProfileDefaults()
{
    for (const QString &key : NO_IMMUTABLE_PROFILE_KEYS) {
        m_installProfileDefaults.insert(key, CommonFunc::getSettingsString(key));
    }
}

void PartitionNavFrame::applyNoImmutableChoice()
{
    const bool enabled = m_noImmutableCheckBox && m_noImmutableCheckBox->isChecked();
    CommonFunc::setSettingsValue("DI_NO_IMMUTABLE_INSTALL", enabled);

    if (!enabled) {
        for (auto it = m_installProfileDefaults.constBegin(); it != m_installProfileDefaults.constEnd(); ++it) {
            CommonFunc::setSettingsValue(it.key(), it.value());
        }
        return;
    }

    const QString policyFile = QString("%1/settings/full_disk_policy_no_immutable.json")
            .arg(CommonFunc::getSystemEnv(DI_CONFIG_DIR_ENV));
    CommonFunc::setSettingsValue("DI_IMMUTABLE_SYSTEM", false);
    CommonFunc::setSettingsValue("DI_PART_POLICY_FILE", policyFile);
    CommonFunc::setSettingsValue("DI_SAVE_USER_DATA_LOAD", false);
    CommonFunc::setSettingsValue("DI_IS_SAVE_AUTHORIZED_DATA_CONFIG", false);
    CommonFunc::setSettingsValue("DI_IS_INIT_RECOVERY", false);
    CommonFunc::setSettingsValue("DI_VALID_DEVICE_MIN_SIZE_CONFIG", 32);
    CommonFunc::setSettingsValue("DI_DEVICE_MIN_SIZE_CONFIG", 40);
    CommonFunc::setSettingsValue("DI_BOOT_SIZE_CONFIG", 2048);
    CommonFunc::setSettingsValue("DI_ROOTA_SIZE_CONFIG", 32768);
    CommonFunc::setSettingsValue("DI_SWAP_MAX_SIZE_CONFIG", 8192);
    CommonFunc::setSettingsValue("DI_PERSISTENT_SIZE_CONFIG", 0);
    CommonFunc::setSettingsValue("DI_DATA_MIN_SIZE_CONFIG", "0%");
}

void PartitionNavFrame::clearGhostSetting()
{
    CommonFunc::setSettingsValue("DI_UIMG_FILE", "");
    CommonFunc::setSettingsValue("DI_IS_GHOST_MODE", false);
    CommonFunc::setSettingsValue("DI_UIMG_DEVICE", "");
}

void PartitionNavFrame::GenerateRecoveryKey()
{
    CommonFunc::setRecoveryKey();
}

CheckResult PartitionNavFrame::checkUImg(const QString &uimgFilePath)
{
    CheckResult checkResult;
    checkResult.result = true;
    checkResult.uimgFilePath = uimgFilePath;

    //挂载uimg
    QString mountPoint = "/uimg";
    QString err;
    CommonFunc::umount(mountPoint, err);
    if (!CommonFunc::mount(QString("\'%1\'").arg(uimgFilePath), mountPoint, "loop", err)) {
        checkResult.error = tr("Failed to read uimg file");
        checkResult.result = false;
        return checkResult;
    }

    QFile md5File(uimgFilePath + ".md5");
    if (md5File.exists()) {
        try {
            md5File.open(QIODevice::ReadOnly | QIODevice::Text);
            auto md5Context = md5File.readAll().split(' ');
            QString md5 = md5Context.isEmpty() ? "" : md5Context.first();


            QString currMd5;
            CommonFunc::command("md5sum", QStringList() << uimgFilePath, currMd5);
            auto currMd5Context = currMd5.split(" ");
            currMd5 = currMd5Context.isEmpty() ? "" : currMd5Context.first();
            if (md5 != currMd5) {
                checkResult.error = tr("Damaged Mirror！");
                checkResult.result = false;
                return checkResult;
            }
            md5File.close();
        } catch (QException &e) {
            qWarning() << e.what();
            md5File.close();
            checkResult.error = tr("Damaged Mirror！");
            checkResult.result = false;
            return checkResult;
        }
    }

    //读取uimg中的info文件
    QJsonDocument doc;
    //to do 测试代码需要删除
    if (!CommonFunc::readJsonFile(mountPoint + "/info.json", doc, err)) {
        checkResult.error = tr("Damaged Mirror！");
        checkResult.result = false;
        return checkResult;
    }


    QJsonObject infoObject = doc.object();
    QString platform = infoObject.value("platform").toString();
    QString displayDriver = infoObject.value("displayDriver").toString();
    bool ghostEfi = infoObject.value("isEFI").toBool();
    QString hardwareType = infoObject.value("hardwareType").toString();

    QMap<QString, quint64> dirSize;
    QJsonObject dirSizeObject = infoObject.value("dirSize").toObject();
    QStringList list = {"/", "/persistent", "/boot", "/home", "/tmp", "/var", "/srv", "/opt", "/usr/local", "/root", "/boot/efi"};
    foreach(auto &i , list) {
        if (dirSizeObject.contains(i)) {
            dirSize.insert(i, dirSizeObject.value(i).toString().toULongLong());
        } else {
            dirSize.insert(i, 0);
        }
    }
    PartitionOperateManager::instance()->setUimgDirSize(dirSize);

    //检查架构是否一致
    QString currPlatform;
    if (!CommonFunc::command("uname", {"-m"}, currPlatform)) {
        qWarning() << "exec uname failed :" << currPlatform;
    }

    currPlatform = currPlatform.trimmed();
    if (currPlatform.isEmpty() || platform.isEmpty() || currPlatform != platform) {
        checkResult.error = tr("Unmatched architectures");
        checkResult.result = false;
        return checkResult;
    }
  
    //华为设备特殊处理，因为klv/pgw/pgv_klu镜像不通用
    QString output;
    CommonFunc::command("/bin/bash", {"-c", "dmidecode  -t 11 | grep -i \"String 4\" | cut -d \":\" -f 2"}, output);
    QString currHuaweiPlatform = parseHuaweiType(output.trimmed());
    QString huaweiPlatform = parseHuaweiType(hardwareType);

    if (currHuaweiPlatform != huaweiPlatform) {
        checkResult.error = tr("Unmatched architectures");
        checkResult.result = false;
        return checkResult;
    }

    //检查引导方式
    if(CommonFunc::isUEFI() != ghostEfi) {
        checkResult.error  = ::QObject::tr("Unmatched boot loaders");
        checkResult.result = false;
        return checkResult;
    }

    //检查显卡是否一致
    QString graphicsCard;
    if (!CommonFunc::command("/bin/bash", {"-c", "lshw -c video |grep product |awk -F : '{print $2}'"}, graphicsCard)) {
        qWarning() << "exec lshw failed :" << graphicsCard;
    }
    qInfo() << "current graphics card:" << graphicsCard;
    qInfo() << "ghost graphice card:" << displayDriver;
    graphicsCard = graphicsCard.trimmed();
    if (graphicsCard != displayDriver) {
        checkResult.warning = tr("Inconsistent graphics card");
        checkResult.result = true;
    }
   

    return checkResult;
}

QString PartitionNavFrame::parseHuaweiType(const QString &type) {
    QString result;

    QStringList klv={"KVKA0"};
    QStringList pgw={"PWC30"};
    QStringList pgv_klu={"PGK90", "PGKA0", "KVK90", "KVK90A"};
    if (klv.contains(type, Qt::CaseInsensitive)) {
        result = "klv";
    } else if (pgw.contains(type, Qt::CaseInsensitive)) {
        result = "pgw";
    } else if (pgv_klu.contains(type, Qt::CaseInsensitive)) {
        result = "pgv_klu";
    }
    return result;
}

void PartitionNavFrame::onStartRepairTool()
{
    DDialog *checkDlg = new DDialog;
    checkDlg->setOnButtonClickedClose(true);
    checkDlg->setMessage(tr("Recognizing installed OS..."));
    DSpinner *spinner = new DSpinner;
    spinner->setFixedSize(32, 32);
    checkDlg->addSpacing(15);
    checkDlg->addContent(spinner, Qt::AlignCenter);
    connect(checkDlg, &DDialog::visibleChanged, [=](bool visible){
        visible ? spinner->start() : spinner->stop();
        checkDlg->moveToCenter();
    });

    QFuture<bool> result = QtConcurrent::run([=]{
        QString output;
        CommonFunc::command("/usr/bin/os-prober", {}, output);
        if(!output.isEmpty()) {
            QStringList splits = CommonFunc::qstringSplitToQStringList(output, "\n");
            QStringList oslist;
            for (auto line: splits) {
                oslist = CommonFunc::qstringSplitToQStringList(line, ":");
                if(oslist.size() > 2) {
                    line = oslist.at(oslist.size() - 2).toLower();
                    if(line == "uos" || line == "deepin") {
                        return true;
                    }
                }
            }
        }

        return false;
    });

    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [=]{
        checkDlg->accept();
        DDialog *dialog = new DDialog;
        dialog->setIcon(QIcon::fromTheme("dialog-warning"));
        connect(dialog, &DDialog::visibleChanged, dialog, [=](bool visible){
            if(visible) {
                dialog->moveToCenter();
            }
        });
        if(watcher->result()) {
            dialog->setMessage(tr("The repair mode helps you fix boot files."
                                 " Are you sure you want to exit the installation and enter the repair mode?"));
            dialog->addButton(tr("Cancel"));
            dialog->addButton(tr("Confirm"), true, DDialog::ButtonRecommend);
            connect(dialog, &DDialog::buttonClicked, [=](int index, const QString &text) {
                Q_UNUSED(text);
                if (index == 1) {
                    if (QFile::exists("/usr/sbin/deepin-repair-tools")) {
        #ifndef QT_DEBUG
                        QProcess::startDetached("/bin/bash", QStringList() <<
                            "/usr/share/deepin-installer/tools/deepin-repairtool-launch");
        #endif
                    }
                }
            });
        } else {
            dialog->setOnButtonClickedClose(true);
            dialog->setMessage(tr("The repair mode works for devices with UOS only."));
            dialog->addButton(tr("Confirm"), true, DDialog::ButtonRecommend);
        }

        dialog->exec();
        watcher->deleteLater();
    });
    watcher->setFuture(result);
    checkDlg->exec();
}

void PartitionNavFrame::initBtnConnect(const OptionButton &pOptionButton)
{
    connect(&pOptionButton, &OptionButton::select, this, [=](const OptionButton &pOptionButton) {
        //防止切回来，全盘加密密码没清除
        PartitionFrameBase *selectFrame = nullptr;
        QList<SDeviceInfo> devs = PartitionOperateManager::instance()->getCurrentDevice();
        QMap<SDeviceInfo, QList<SPartitionInfo>> devInfos = PartitionOperateManager::instance()->getCurrentDeviceInfo();
        QMap<SVGInfo, QList<SLVMInfo>> lvms = PartitionOperateManager::instance()->getCurrentLVMInfo();

        if (m_spinner->isPlaying()) {
            DMessageManager::instance()->setContentMargens(this, QMargins(0, 0, 0, 10));
            DMessageManager::instance()->sendMessage(this, QIcon::fromTheme("icon_warning"), tr("Verifying uimg file..."));
            return;
        }

        applyNoImmutableChoice();

        if(m_modeBox->checkedId() == 1) {
            if (m_uimgCheckResult.uimgFilePath.isEmpty() || !QFileInfo::exists(m_uimgCheckResult.uimgFilePath)) {
                DMessageManager::instance()->setContentMargens(this, QMargins(0, 0, 0, 10));
                DMessageManager::instance()->sendMessage(this, QIcon::fromTheme("icon_warning"), tr("No uimg file selected, unable to proceed!"));
                return;
            } else if (!m_uimgCheckResult.result) {
                DMessageManager::instance()->setContentMargens(this, QMargins(0, 0, 0, 10));
                DMessageManager::instance()->sendMessage(this, QIcon::fromTheme("icon_warning"), tr("Failed to verify uimg file, unable to proceed!"));
                return;
            }

            if (m_uimgCheckResult.result) {
                CommonFunc::setSettingsValue("DI_UIMG_FILE", m_uimgCheckResult.uimgFilePath);
                CommonFunc::setSettingsValue("DI_IS_GHOST_MODE", true);
                CommonFunc::setSettingsValue("DI_UIMG_DEVICE", m_selectImgWidget->getSelectDevice());
            } else {
                clearGhostSetting();
            }
        }

        CommonFunc::setSettingsValue("DI_CRYPT_PASSWORD", "");
        if (!FULLDISK_PARTTION_BTN.compare(pOptionButton.objectName())) {
            if(CommonFunc::isGhost()) {
                SDeviceInfo imgDev;
                QString devName = CommonFunc::getSettingsString("DI_UIMG_DEVICE");
                imgDev.device  = devName;
                if(!devs.contains(imgDev)) {
                    for (auto key: devs) {
                        if(devName.startsWith(key.device)) {
                            imgDev.device = key.device;
                            break;
                        }
                    }
                }

                qInfo() << "filter device:" << imgDev.device << " in ghost mode";
                devs.removeAll(imgDev);
            }

            PartitionOperateManager::instance()->setPartitionMode(FullDiskMode);
            if (devs.size() > 1) {
                selectFrame = m_mapPartition.value(PartitionType::MULTIDISK_PART);
                MultipleDiskPartitionFrame *multiFrame = qobject_cast<MultipleDiskPartitionFrame*>(selectFrame);
                multiFrame->initDeviceInfo(devs);
            } else {
                selectFrame = m_mapPartition.value(PartitionType::FULLDISK_PART);
                FullDiskPartitionFrame *fullFrame = qobject_cast<FullDiskPartitionFrame*>(selectFrame);
                fullFrame->initDiskData(devs);
            }
        } else if (!CUSTOM_PARTTION_BTN.compare(pOptionButton.objectName())) {
            PartitionOperateManager::instance()->setPartitionMode(CustomMode);
            selectFrame = m_mapPartition.value(PartitionType::CUSTOM_PART);
            CustomPartitionFrame *customFrame = qobject_cast<CustomPartitionFrame*>(selectFrame);
            customFrame->initDeviceInfo(devInfos, lvms);
        } else if (!SENIOR_PARTTION_BTN.compare(pOptionButton.objectName())) {
            PartitionOperateManager::instance()->setPartitionMode(AdvancedMode);
            selectFrame = m_mapPartition.value(PartitionType::SENIOR_PART);
            SeniorPartitionFrame *seniorFrame = qobject_cast<SeniorPartitionFrame*>(selectFrame);
            seniorFrame->initDeviceInfo(devInfos, lvms);
        }
        if(selectFrame) {
            m_centerWidget->setCurrentWidget(selectFrame);
        }
    });
}
