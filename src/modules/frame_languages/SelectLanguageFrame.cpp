#include "SelectLanguageFrame.h"
#include "CommonFunc.h"
#include "globals.h"
#include "LicenseDialog.h"
#include "TimezoneFrame.h"
#include "DeviceManager.h"

#include <DApplication>
#include <DWidget>
#include <DDialog>
#include <DListView>

#include <QShortcut>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QStandardItemModel>
#include <DConfig>
#include <DFontSizeManager>
#include <QScrollBar>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

SelectLanguageFrame::SelectLanguageFrame(QWidget *parent) :FramePluginInterface (parent)
{
//    initTranslator();
    m_translator = new QTranslator;
    m_frameName = gFrameLanguage;

    // 选择语言标题
    m_title = new DLabel(::QObject::tr("Select Language"));
    DFontSizeManager::instance()->bind(m_title, DFontSizeManager::T3, 450);
    m_title->setAlignment(Qt::AlignCenter);
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->addStretch();
    titleLayout->addWidget(m_title);
    titleLayout->addStretch();

    // 选择语言视图
    QScrollBar *vBar = new QScrollBar(Qt::Vertical);
    vBar->setFixedWidth(15);
    vBar->setContextMenuPolicy(Qt::NoContextMenu);
    m_selectLanguageWidget = new SelectLanguageWidget;
    m_selectLanguageWidget->setFocusPolicy(Qt::NoFocus);

    m_selectLanguageWidget->setVerticalBar(vBar);
    m_selectLanguageWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QHBoxLayout *selectLanguageWidgetLayout = new QHBoxLayout;
    selectLanguageWidgetLayout->setSpacing(0);
    CommonFunc::isBigScreen() ? selectLanguageWidgetLayout->setContentsMargins(180, 0, 0, 0) : \
    selectLanguageWidgetLayout->setContentsMargins(30, 0, 0, 0);
    selectLanguageWidgetLayout->addWidget(m_selectLanguageWidget);
    selectLanguageWidgetLayout->addSpacing(CommonFunc::isBigScreen() ? 165 : 15);
    selectLanguageWidgetLayout->addWidget(vBar, 0, Qt::AlignRight);

    // 用户体验计划和用户许可协议标签和勾选框
    m_acceptExperience = new DCheckBox(::QObject::tr("Agree to the"));
    m_acceptExperience->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_acceptExperience->setCheckState(CommonFunc::getSettingsBool("DI_USER_EXPERIENCE") ?
                                          Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    m_experienceLabel = new DLabel(userExperienceTitle());
    m_experienceLabel->setMaximumWidth(386);
    m_experienceLabel->setFocusPolicy(Qt::TabFocus);
    m_experienceLabel->setCursor(QCursor(Qt::PointingHandCursor));
    m_experienceLabel->installEventFilter(this);
    m_experienceLabel->setStyleSheet(""
                                     "QLabel {"
                                     "color:#0180FF"
                                     "}"
                                     "QLabel:focus {"
                                     "border: 2px solid;"
                                     "border-radius:5px;"
                                     "border-color:rgb(1, 128, 255)"
                                     "}");

    m_experienceLabel->installEventFilter(this);
    QHBoxLayout *acceptExperienceLayout = new QHBoxLayout;
    acceptExperienceLayout->setContentsMargins(0, 0, 0, 0);
    acceptExperienceLayout->setSpacing(5);
    acceptExperienceLayout->addStretch();
    acceptExperienceLayout->addWidget(m_acceptExperience);
    acceptExperienceLayout->addWidget(m_experienceLabel);
    acceptExperienceLayout->addStretch();
    m_experienceWidget = new QFrame;
    m_experienceWidget->setLayout(acceptExperienceLayout);
    m_experienceWidget->setVisible(isShowExperienceWidget());

    m_acceptLicense = new DCheckBox(::QObject::tr("I have read and agree to the"));
    connect(m_acceptLicense, &DCheckBox::clicked, this, [=]{
        if( m_acceptLicense->checkState() == Qt::CheckState::Checked) {
            m_nextBnt->setEnabled(true);
        } else {
            m_nextBnt->setEnabled(CommonFunc::getCurrentOs() == OSType::Community);
        }
    });
    m_acceptLicense->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_licenseLabel = new DLabel(::QObject::tr("End User License Agreement"));
    m_licenseLabel->setMaximumWidth(186);
    m_licenseLabel->setCursor(QCursor(Qt::PointingHandCursor));
    m_licenseLabel->setStyleSheet(""
                                     "QLabel {"
                                     "color:#0180FF"
                                     "}"
                                     "QLabel:focus {"
                                     "border: 2px solid;"
                                     "border-radius:5px;"
                                     "border-color:rgb(1, 128, 255)"
                                     "}");
    m_licenseLabel->installEventFilter(this);
    m_licenseLabel->setFocusPolicy(Qt::TabFocus);
    m_andLabel = new DLabel(::QObject::tr("and"));
    m_privacyLicenseLabel = new DLabel(::QObject::tr("Privacy Policy"));
    m_privacyLicenseLabel->setMaximumWidth(90);
    m_privacyLicenseLabel->setCursor(QCursor(Qt::PointingHandCursor));
    m_privacyLicenseLabel->setStyleSheet(""
                                  "QLabel {"
                                  "color:#0180FF"
                                  "}"
                                  "QLabel:focus {"
                                  "border: 2px solid;"
                                  "border-radius:5px;"
                                  "border-color:rgb(1, 128, 255)"
                                  "}");
    m_privacyLicenseLabel->installEventFilter(this);
    m_privacyLicenseLabel->setFocusPolicy(Qt::TabFocus);
    QHBoxLayout *acceptLicenseLayout = new QHBoxLayout;
    acceptLicenseLayout->setContentsMargins(0, 0, 0, 0);
    acceptLicenseLayout->setSpacing(5);
    acceptLicenseLayout->addStretch();
    acceptLicenseLayout->addWidget(m_acceptLicense);
    acceptLicenseLayout->addWidget(m_licenseLabel);
    acceptLicenseLayout->addWidget(m_andLabel);
    acceptLicenseLayout->addWidget(m_privacyLicenseLabel);
    acceptLicenseLayout->addStretch();

    m_licenseWidget = new QFrame;
    m_licenseWidget->setLayout(acceptLicenseLayout);
    m_licenseWidget->hide();

    QVBoxLayout *userLayout = new QVBoxLayout;
    userLayout->addWidget(m_experienceWidget, 0, Qt::AlignLeft);
    userLayout->addSpacing(5);
    userLayout->addWidget(m_licenseWidget, 0, Qt::AlignLeft);

    QHBoxLayout *userHBoxLayout = new QHBoxLayout;
    userHBoxLayout->addStretch();
    userHBoxLayout->addLayout(userLayout);
    userHBoxLayout->addStretch();

    // 下一步,返回按钮
    m_nextBnt = new DSuggestButton;
    m_nextBnt->setFixedSize(gnextbuttonwidth, gnextbuttonheight);
    m_nextBnt->setText(::QObject::tr("Next"));
    m_nextBnt->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_nextBnt->setEnabled(CommonFunc::getCurrentOs() == OSType::Community);
    m_nextBnt->setDefault(true);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(m_nextBnt, 0, Qt::AlignHCenter);

    // 显示安装器版本号的label
    QWidget *rightHolder = new QWidget;
    QHBoxLayout *hVerlayout = new QHBoxLayout;
    hVerlayout->setContentsMargins(0, 0, 0, 0);
    rightHolder->setLayout(hVerlayout);
    m_versionLabel = new DLabel;
    m_versionLabel->setText("Fedora DDE Remix Installer " + CommonFunc::getVersion());
    m_versionLabel->adjustSize();
    m_versionLabel->setStyleSheet("font-size:12px; color: black; font-weight: lighter;");
    m_versionLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
    m_versionLabel->setVisible(false);
    hVerlayout->addWidget(m_versionLabel);
    btnLayout->setContentsMargins(30, 0, 30, 0);
    btnLayout->insertWidget(0, new QWidget, 0, Qt::AlignLeft);
    btnLayout->addWidget(rightHolder, 0, Qt::AlignRight);

    // 主布局
    m_contentLayout->addLayout(titleLayout);
    m_contentLayout->addSpacing(15);
    m_contentLayout->addLayout(selectLanguageWidgetLayout);
    m_contentLayout->addSpacing(15);
    m_contentLayout->addLayout(userHBoxLayout);
    m_contentLayout->addSpacing(20);
    m_contentLayout->addLayout(btnLayout);
    m_contentLayout->addSpacing(CommonFunc::getInnerMargin());

    qInfo() << "mainLayout" << m_contentLayout->count();
    // 设置快捷键Ctrl+V，用来显示安装器版本
    QShortcut *shortcut = new QShortcut(QKeySequence("Ctrl+V"), this);
    connect(shortcut, &QShortcut::activated, [=] {
        if (m_versionLabel->isHidden()) {
            m_versionLabel->show();
        } else {
            m_versionLabel->hide();
        }
    });

    connect(m_nextBnt, &QPushButton::clicked, this, [=] {
        bool canNext = true;
        if (qApp->applicationName() != "fedora-dde-installer-live-config") {
            canNext = checkDiskSpace();
        }
        if (canNext) {
            this->writeConf();
            emit this->next();
        }
    });
    connect(m_selectLanguageWidget, &SelectLanguageWidget::selectLanguageViewClicked, [=](const SLanguageItem item) {
        this->writeConf();
        Q_EMIT onChangeLanguage(item.locale);
        updateTs();
    });
}

SelectLanguageFrame::~SelectLanguageFrame()
{
}

bool SelectLanguageFrame::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (obj == m_licenseLabel) {
            requestShowUserLicense();
        } else if (obj == m_experienceLabel) {
            requestShowUserExperience();
        } else if (obj == m_privacyLicenseLabel) {
            requestShowPrivacyLicense();
        }
    } else if (event->type() == QEvent::KeyPress) {
        auto keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Space || keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (obj == m_licenseLabel) {
                requestShowUserLicense();
            } else if (obj == m_experienceLabel) {
                requestShowUserExperience();
            } else if (obj == m_privacyLicenseLabel) {
                requestShowPrivacyLicense();
            }
        }
    }

    return QObject::eventFilter(obj, event);
}


void SelectLanguageFrame::writeConf()
{
    SLanguageItem item = m_selectLanguageWidget->getCurrentItem();
    qInfo() << item.name << item.locale
            << item.localName << item.timezone;
    CommonFunc::setSettingsValue("DI_LOCALE", item.locale);

    if (m_acceptExperience->checkState() == Qt::CheckState::Checked) {
        CommonFunc::setSettingsValue("DI_USER_EXPERIENCE", true);
    } else {
        CommonFunc::setSettingsValue("DI_USER_EXPERIENCE", false);
    }

    if (m_acceptLicense->checkState() == Qt::CheckState::Checked) {
        CommonFunc::setSettingsValue("DI_PRIVACY_POLICY", true);
    } else {
        CommonFunc::setSettingsValue("DI_PRIVACY_POLICY", false);
    }
}


void SelectLanguageFrame::updateTs()
{
    m_title->setText(::QObject::tr("Select Language"));
    m_acceptExperience->setText(::QObject::tr("Agree to the"));
    setLabelText(m_experienceLabel, userExperienceTitle(), 386);
    m_acceptLicense->setText(::QObject::tr("I have read and agree to the"));
    setLabelText(m_licenseLabel, ::QObject::tr("End User License Agreement"), 186);
    m_andLabel->setText(::QObject::tr("and"));
    setLabelText(m_privacyLicenseLabel, ::QObject::tr("Privacy Policy"), 90);
    m_nextBnt->setText(::QObject::tr("Next"));
}


void SelectLanguageFrame::requestShowUserLicense()
{
    QString text = getLicenseText(LicenseType::endUserLicenseAgreement);
    LicenseDialog dialog(text);
    dialog.move(qApp->primaryScreen()->availableSize().width() / 2 - dialog.width() / 2,
                qApp->primaryScreen()->availableSize().height() / 2 - dialog.height() / 2);
    dialog.exec();
}

void SelectLanguageFrame::requestShowUserExperience()
{
    QString text = getLicenseText(LicenseType::userExperienceAgreement);
    LicenseDialog dialog(text);
    dialog.move(qApp->primaryScreen()->availableSize().width() / 2 - dialog.width() / 2,
                qApp->primaryScreen()->availableSize().height() / 2 - dialog.height() / 2);
    dialog.exec();
}

void SelectLanguageFrame::requestShowPrivacyLicense()
{
    QString text = getLicenseText(LicenseType::privacyPolicy);
    LicenseDialog dialog(text);
    dialog.move(qApp->primaryScreen()->availableSize().width() / 2 - dialog.width() / 2,
                qApp->primaryScreen()->availableSize().height() / 2 - dialog.height() / 2);
    dialog.exec();
}

void SelectLanguageFrame::onBackButton()
{
}

bool SelectLanguageFrame::isShowExperienceWidget()
{
    return false;
}

QString SelectLanguageFrame::userExperienceTitle()
{
    return QString();
}

QString SelectLanguageFrame::getLicenseText(LicenseType licenseType)
{
    QString locale = m_selectLanguageWidget->getCurrentItem().locale;
    QString licenseName;
    QString licenseDir = "/usr/share/protocol/";
    QString editionName = CommonFunc::getSettingsString("DI_EDITION_NAME");
    QString osType = editionName.isEmpty() ? CommonFunc::getCurrentOsType() : editionName;
    QString osTypeString = "";
    auto config = DConfig::create("org.fedoradde.installer", "org.fedoradde.installer");
    switch (licenseType) {
    case LicenseType::privacyPolicy:
        licenseDir += "privacy-policy/";
        osTypeString = CommonFunc::getCurrentOs() == OSType::Community ? ("-" + osType) : "";
        licenseName = QString("Privacy-Policy%1-CN-%2.txt").arg(osTypeString).arg(locale);
        if (!QFile::exists(licenseDir + licenseName)) {
            if (locale == "zh_HK" || locale == "zh_TW" || locale == "bo_CN" || locale == "ug_CN") {
                locale = "zh_CN";
            } else {
                locale = "en_US";
            }
            licenseName = QString("Privacy-Policy%1-CN-%2.txt").arg(osTypeString).arg(locale);
        }
        break;
    case LicenseType::endUserLicenseAgreement:
        licenseDir += "enduser-agreement/";
        if (config != nullptr && config->isValid()) {
            qInfo() << "read config file org.fedoradde.installer success";
            if (config->keyList().contains("Use_mil")) {
                int military = config->value("Use_mil").toInt();
                //1:军用版  0：非军用版
                if (military == 1) {
                    licenseName = config->value("Mil_enduserAgreement_txt").toString();
                    //dconfig获取的路径已经是绝对路径
                    licenseDir.clear();
                    if (!QFile::exists(licenseName.arg(locale))) {
                        qInfo() << QString("%1 not exist").arg(licenseName.arg(locale));
                        if (locale == "zh_HK" || locale == "zh_TW" || locale == "bo_CN" || locale == "ug_CN") {
                            locale = "zh_CN";
                        } else {
                            locale = "en_US";
                        }
                    }
                    licenseName = licenseName.arg(locale);
                    break;
                }
                
            }
        } 

        if (osType == "E") {
            osType = "Education";
        }
        osTypeString = CommonFunc::getCurrentOs() == OSType::Community ? "" : ("-" + osType);
        licenseName = QString("End-User-License-Agreement%1-CN-%2.txt").arg(osTypeString).arg(locale);
        if (!QFile::exists(licenseDir + licenseName)) {
            if (locale == "zh_HK" || locale == "zh_TW" || locale == "bo_CN" || locale == "ug_CN") {
                locale = "zh_CN";
            } else {
                locale = "en_US";
            }
            licenseName = QString("End-User-License-Agreement%1-CN-%2.txt").arg(osTypeString).arg(locale);
        }
        break;
    case LicenseType::userExperienceAgreement:
        licenseDir += "userexperience-agreement/";
        osTypeString = CommonFunc::getCurrentOs() == OSType::Community ? ("-" + osType) : "";
        licenseName = QString("User-Experience-Program-License-Agreement%1-CN-%2.txt").arg(osTypeString).arg(locale);
        if (!QFile::exists(licenseDir + licenseName)) {
            if (locale == "zh_HK" || locale == "zh_TW" || locale == "bo_CN" || locale == "ug_CN") {
                locale = "zh_CN";
            } else {
                locale = "en_US";
            }
            licenseName = QString("User-Experience-Program-License-Agreement%1-CN-%2.txt").arg(osTypeString).arg(locale);
        }
        break;
    }
    QString text = CommonFunc::readFile(licenseDir + licenseName);
    return text;
}

bool SelectLanguageFrame::checkDiskSpace()
{
    bool isValidOk = true;
    QMap<SDeviceInfo, QList<SPartitionInfo>> deviceInfos = DeviceManager::instance()->getDeviceInfos();
    qint64 validDeviceMinSize = CommonFunc::getSettingsInt("DI_VALID_DEVICE_MIN_SIZE_CONFIG");
    qint64 deviceMinSize = CommonFunc::getSettingsInt("DI_DEVICE_MIN_SIZE_CONFIG");

    bool isValidFullDisk = false;
    bool isValidDisk = false;
    for (SDeviceInfo dev : deviceInfos.keys()) {
        if (dev.isStartDevice || dev.isVentoyDevice)
            continue;
        if (dev.size >= CommonFunc::gToM(deviceMinSize)) {
            isValidFullDisk = true;
            isValidDisk = true;
            break;
        } else if (dev.size >= CommonFunc::gToM(validDeviceMinSize)) {
            isValidDisk = true;
        }
    }

    if (!isValidFullDisk) {
        if (isValidDisk) {
            DDialog dialog;
            dialog.setWindowFlags(Qt::WindowTitleHint);
            dialog.setTitle(::QObject::tr("Limited disk space"));
            dialog.setMessage(::QObject::tr("Limited disk space may affect your subsequent experience, "
                                            "it is recommended to select a larger disk for installation. "
                                            "You can click Cancel to re-select and click OK to continue the installation"));
            dialog.setWordWrapMessage(true);
            dialog.addButton(::QObject::tr("Cancel", "Button"), false);
            dialog.addButton(::QObject::tr("Installation", "Button"), false);
            dialog.setCloseButtonVisible(true);
            connect(&dialog, &DDialog::buttonClicked, this, [&](int index, const QString &text) {
                if (index == 0) {
                    isValidOk = false;
                }
            });
            connect(&dialog, &DDialog::closed, this, [&]() {
                isValidOk = false;
            });
            dialog.move(qApp->primaryScreen()->availableSize().width() / 2 - dialog.width() / 2,
                        qApp->primaryScreen()->availableSize().height() / 2 - dialog.height() / 2);
            dialog.exec();
        } else {
            DDialog dialog;
            dialog.setWindowFlags(Qt::WindowTitleHint);
            dialog.setAttribute(Qt::WA_TranslucentBackground);
            dialog.setTitle(::QObject::tr("Insufficient disk space"));
            dialog.setMessage(::QObject::tr("At least %1 of disk space is required to install this operating system").arg(QString("%1GiB").arg(validDeviceMinSize)));
            dialog.setWordWrapMessage(true);
            dialog.addButton(::QObject::tr("Cancel", "Button"), false);
            dialog.setCloseButtonVisible(true);
            connect(&dialog, &DDialog::buttonClicked, this, [&](int index, const QString &text) {
                isValidOk = false;
            });
            connect(&dialog, &DDialog::closed, this, [&]() {
                isValidOk = false;
            });
            dialog.move(qApp->primaryScreen()->availableSize().width() / 2 - dialog.width() / 2,
                        qApp->primaryScreen()->availableSize().height() / 2 - dialog.height() / 2);
            dialog.exec();
        }
    }

    return isValidOk;
}

void SelectLanguageFrame::setLabelText(DLabel *itemLabel, const QString &content, const int textWidth)
{
    if (itemLabel == nullptr) {
        return;
    }

    QFontMetrics labelFont(itemLabel->font());
    QString elideNote = labelFont.elidedText(content, Qt::ElideMiddle, textWidth, Qt::AlignLeft | Qt::AlignBottom | Qt::TextSingleLine);
    itemLabel->setText(elideNote);
    (elideNote.indexOf("…") != -1) ? itemLabel->setToolTip(content) : itemLabel->setToolTip("");
}
