#include "ProgressUpdateFrame.h"
#include "AnimationWidget.h"
#include "CommonFunc.h"
#include "TitleWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>
#include <DIconButton>
#include <QLabel>


DWIDGET_USE_NAMESPACE

ProgressUpdateFrame::ProgressUpdateFrame(QWidget *parent)
    : QWidget (parent)
{
    // 标题栏
    m_titleWidget = new TitleWidget(this);
    m_titleWidget->setMinimumHeight(90);
    QHBoxLayout *titleMainLayout = new QHBoxLayout;
    titleMainLayout->addWidget(m_titleWidget);

    //　显示动画
    AnimationWidget *animationWidget = new AnimationWidget(this);
    animationWidget->setAnimations(CommonFunc::getInstallAnimations());
    connect(this, &ProgressUpdateFrame::startAnime, animationWidget, &AnimationWidget::startAnimation);
    connect(animationWidget, &AnimationWidget::animationChange, this, [=](const QString &pictureName) {
        for (auto &key: m_mapProTrans.keys()) {
            if(pictureName.contains(key)) {
                setAnimationsTitle(m_mapProTrans[key].first, m_mapProTrans[key].second);
                break;
            }
        }
    });

    // 上一个动画按钮
    DIconButton *backAnimationButton = new DIconButton(this);
    backAnimationButton->setIcon(QIcon::fromTheme("go-previous"));
    backAnimationButton->setIconSize(QSize(20, 20));
    backAnimationButton->setStyleSheet("background-color:rgba(255, 255, 255, 0);");
    backAnimationButton->setFocusPolicy(Qt::NoFocus);
    QVBoxLayout *backAnimationLayout = new QVBoxLayout;
    backAnimationLayout->addStretch();
    backAnimationLayout->addWidget(backAnimationButton);
    backAnimationLayout->addStretch();
    connect(backAnimationButton, &DIconButton::clicked, animationWidget, &AnimationWidget::backAnimation);

    // 下一个动画按钮
    DIconButton *nextAnimationButton = new DIconButton(this);
    nextAnimationButton->setIcon(QIcon::fromTheme("go-next"));
    nextAnimationButton->setIconSize(QSize(20, 20));
    nextAnimationButton->setStyleSheet("background-color:rgba(255, 255, 255, 0);");
    nextAnimationButton->setFocusPolicy(Qt::NoFocus);
    QVBoxLayout *nextAnimationLayout = new QVBoxLayout;
    nextAnimationLayout->addStretch();
    nextAnimationLayout->addWidget(nextAnimationButton);
    nextAnimationLayout->addStretch();
    connect(nextAnimationButton, &DIconButton::clicked, animationWidget, &AnimationWidget::nextAnimation);

    // 动画主布局
    QHBoxLayout *animationLayout = new QHBoxLayout;
    animationLayout->setContentsMargins(0, 0, 0, 0);
    animationLayout->setSpacing(0);
    animationLayout->addLayout(backAnimationLayout, 1);
    animationLayout->addSpacing(15);
    animationLayout->addWidget(animationWidget, 15);
    animationLayout->addSpacing(15);
    animationLayout->addLayout(nextAnimationLayout, 1);

    // 提示信息
    m_titleLabel = new QLabel;
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPixelSize(22);
    m_titleLabel->setFont(titleFont);
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->addStretch();
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();
    m_titleLabel->setVisible(CommonFunc::isBigScreen());

    m_promptLabel = new QLabel;
    m_promptLabel->setAlignment(Qt::AlignCenter);
    QHBoxLayout *promptLayout = new QHBoxLayout;
    promptLayout->addStretch();
    promptLayout->addWidget(m_promptLabel);
    promptLayout->addStretch();

    // 进度显示
    m_progressWidget = new QProgressBar(this);
    m_progressWidget->setValue(0);
    m_progressWidget->setAlignment(Qt::AlignCenter);  // 对齐方式

    QHBoxLayout *progressLayout = new QHBoxLayout;
    progressLayout->setContentsMargins(100, 0, 100, 0);
    progressLayout->addWidget(m_progressWidget);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(titleMainLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(animationLayout);
    mainLayout->addSpacing(CommonFunc::isBigScreen() ? 10 : 0);
    mainLayout->addLayout(titleLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(progressLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(promptLayout);
    mainLayout->addSpacing(CommonFunc::isBigScreen() ? 60 : 8);

    this->setLayout(mainLayout);
}

void ProgressUpdateFrame::setProgressStatus(const QString &status)
{
    auto locale = CommonFunc::getSettingsString("DI_LOCALE");
    if (locale == "ug_CN") { //维语特殊处理
        m_progressWidget->setFormat(QString("%1%v").arg(QString("%1%").arg(status)));
    } else {
        m_progressWidget->setFormat(QString("%1 %v%").arg(status));
    }
}

void ProgressUpdateFrame::setAnimationsTitle(const QString &title, const QString &tips)
{
    m_titleWidget->setTitleLabelProperty(title, "");
    m_titleWidget->setTipsLabelProperty(tips, "font-size:16px;font-weight:Medium;");
}

void ProgressUpdateFrame::updateTS()
{
    //m_titleLabel->setText(::QObject::tr("Configuring your computer"));
    m_promptLabel->setText(QString(::QObject::tr("It takes about a few minutes, and you can use %1 soon...")).arg(CommonFunc::isNameDeepin()));
    if(CommonFunc::getCurrentOsVersion() == "V23") {
        QString currentOs = CommonFunc::getCurrentOsType();
        if ("Desktop" == currentOs) {
            initV23DesktopString();
        } else if ("Professional" == currentOs) {
            initV23ProfessionalString();
        }
    } else if (CommonFunc::getCurrentOsVersion() == "V25") {
        initV25DefaultTitleString();
    } else {
        QString currentOs = CommonFunc::getCurrentOsType();
        if ("Desktop" == currentOs) {
            initV20DesktopString();
        } else if ("Professional" == currentOs || "E" == currentOs) {
            initV20ProfessionalString();
        }
    }
}


void ProgressUpdateFrame::initV25DefaultTitleString()
{
    m_mapProTrans = {
            {"01", {::QObject::tr("Multi-Architecture Support"),
                           ::QObject::tr("Supports AMD64、ARM64、RISC-V and LoongArch64 multi-platform architectures, providing users with more choices.")}},
            {"02", {::QObject::tr("Global Language Support"),
                           ::QObject::tr("Offers system translations in 55 languages, including Chinese, Tibetan, Uyghur, English, French, Portuguese, and more, providing global users with more localized and thoughtful services.")}},
            {"03", {::QObject::tr("Immutable System, Ever-Stable"),
                           ::QObject::tr("The core system remains as stable as ever, supporting efficient installation and incremental upgrades, along with quick rollback capabilities.")}},
            {"04", {::QObject::tr("New Desktop Experience"),
                           ::QObject::tr("The redesigned taskbar, launcher, and control center provide users with an enhanced operating experience and a more unified, streamlined management interface.")}},
            {"05", {::QObject::tr("Treeland Window Compositor"),
                           ::QObject::tr("Our self-developed Treeland window compositor offers users quicker response times and a smoother visual interaction experience.")}},
            {"06", {::QObject::tr("UOS AI 2.0"),
                           ::QObject::tr("Supports AI system management, intelligent search, translation, interpretation, document creation, PPT generation, poster design, and more, empowering multiple scenarios and significantly boosting office efficiency.")}},
            {"07", {::QObject::tr("Application Ecosystem"),
                           ::QObject::tr("A wide range of applications for various fields are available in the app store, meeting users' diverse needs in entertainment, work, study, and more.")}},
            {"08", {::QObject::tr("Subsystem"),
                           ::QObject::tr("Seamlessly run Debian, Ubuntu, Arch, and other multi-system environments, breaking the boundaries of system ecosystems.")}}

    };
}

void ProgressUpdateFrame::initV20DesktopString()
{
    m_mapProTrans = {
        {"01", {::QObject::tr("deepin V20"), ""}},
        {"02", {::QObject::tr("Linglong"), ::QObject::tr("Born for better compatibility and security.")}},
        {"03", {::QObject::tr("New Kernel"), ::QObject::tr("Support for 12th Gen CPUs and more hardware devices.")}},
        {"04", {::QObject::tr("Atomic Update"), ::QObject::tr("Atomic system upgrade ensures rollback safety.")}},
        {"05", {::QObject::tr("Desktop Environment"), ::QObject::tr("The brand-new UI design and custom system theme make your desktop beautiful.")}},
        {"06", {::QObject::tr("Flow Design"), ::QObject::tr("New design, new experiences.")}},
        {"07", {::QObject::tr("Grand Search"), ::QObject::tr("Search through the whole system for required files and settings.")}},
        {"08", {::QObject::tr("Image to Text | Scrollshots"),
                ::QObject::tr("Screen Capture, Image Viewer, and Album - extract text from pictures with one click, and take a quick scrolling screenshot.")}},
        {"09", {::QObject::tr("App Store"), ::QObject::tr("Plenty of apps are available, including many daily-used Android apps.")}},
        {"10", {::QObject::tr("Backup and Restore"), ::QObject::tr("Multiple backup and restore schemes are available.")}},
        {"11", {::QObject::tr("Multi-Screen Collaboration"), ::QObject::tr("Seamless collaboration improves working efficiency across computers and smartphones.")}},
    };
}

void ProgressUpdateFrame::initV20ProfessionalString()
{
    m_mapProTrans = {
        {"01", {QObject::tr("Compatible with multiple platforms"), QObject::tr("Compatible with Loongson, Sunway, Hygon, Zhaoxin, Phytium, Huawei Kunpeng 920 and other platforms\n"
                                                                               "Provide users with professional technical support")}},
        {"02", {QObject::tr("Support for multiple languages"), QObject::tr("It includes more than 30 languages including Chinese (simplified, traditional Chinese from Hong Kong, traditional Chinese from Taiwan), English, Tibetan, and Uyghur\n"
                                                                           "Let users around the world experience the infinite charm of UOS")}},
        {"03", {QObject::tr("Desktop environment"), QObject::tr("Good looking, better to use\n"
                                                                "One-click organization of the desktop, easy wireless projection, so that you can enjoy a more convenient operation experience")}},
        {"04", {QObject::tr("App Store"), QObject::tr("Deliver the vast number of applications you need")}},
        {"05", {QObject::tr("File Manager"), QObject::tr("New  design of interface, convenient tree view for efficient office, partition encryption to escort file storage security")}},
        {"06", {QObject::tr("Safe and powerful"), QObject::tr("Passwordless login, disk partition encryption, application signature control, application permission control, firewall and VPN tools, etc. \n"
                                                              "The full-stack security framework USEC comprehensively protects data and privacy security")}},
        {"07", {QObject::tr("Service and support"), QObject::tr("It is recommended that you use the Help Manual, Service & Support App for assistance \n"
                                                                "If you need more help, you can join the official group, call the 400 hotline or contact online customer service")}}
    };
}

void ProgressUpdateFrame::initV23DesktopString()
{
    m_mapProTrans = {
        {"01", {::QObject::tr("deepin V23"), ""}},
        {"02", {::QObject::tr("Linglong"), ::QObject::tr("Born for better compatibility and security.")}},
        {"03", {::QObject::tr("New Kernel"), ::QObject::tr("Support for 12th Gen CPUs and more hardware devices.")}},
        {"04", {::QObject::tr("Atomic Update"), ::QObject::tr("Atomic system upgrade ensures rollback safety.")}},
        {"05", {::QObject::tr("Desktop Environment"), ::QObject::tr("The brand-new UI design and custom system theme make your desktop beautiful.")}},
        {"06", {::QObject::tr("Flow Design"), ::QObject::tr("New design, new experiences.")}},
        {"07", {::QObject::tr("Grand Search"), ::QObject::tr("Search through the whole system for required files and settings.")}},
        {"08", {::QObject::tr("Image to Text | Scrollshots"),
                ::QObject::tr("Screen Capture, Image Viewer, and Album - extract text from pictures with one click, and take a quick scrolling screenshot.")}},
        {"09", {::QObject::tr("App Store"), ::QObject::tr("Plenty of apps are available, including many daily-used Android apps.")}},
        {"10", {::QObject::tr("Backup and Restore"), ::QObject::tr("Multiple backup and restore schemes are available.")}},
        {"11", {::QObject::tr("Multi-Screen Collaboration"), ::QObject::tr("Seamless collaboration improves working efficiency across computers and smartphones.")}}
    };
}

void ProgressUpdateFrame::initV23ProfessionalString()
{
    m_mapProTrans = {
        {"01", {::QObject::tr("UnionTech OS V23"), ""}},
        {"02", {::QObject::tr("Linglong"), ::QObject::tr("Born for better compatibility and security.")}},
        {"03", {::QObject::tr("New Kernel"), ::QObject::tr("Support for 12th Gen CPUs and more hardware devices.")}},
        {"04", {::QObject::tr("Atomic Update"), ::QObject::tr("Atomic system upgrade ensures rollback safety.")}},
        {"05", {::QObject::tr("Desktop Environment"), ::QObject::tr("The brand-new UI design and custom system theme make your desktop beautiful.")}},
        {"06", {::QObject::tr("Flow Design"), ::QObject::tr("New design, new experiences.")}},
        {"07", {::QObject::tr("Grand Search"), ::QObject::tr("Search through the whole system for required files and settings.")}},
        {"08", {::QObject::tr("Image to Text | Scrollshots"),
                ::QObject::tr("Screen Capture, Image Viewer, and Album - extract text from pictures with one click, and take a quick scrolling screenshot.")}},
        {"09", {::QObject::tr("App Store"), ::QObject::tr("Plenty of apps are available, including many daily-used Android apps.")}}
    };
}

void ProgressUpdateFrame::setTitle(const QString &content, const QString &stylesheet)
{
    m_titleLabel->setText(content);
    if (stylesheet.compare("")) {
        m_titleLabel->setStyleSheet(stylesheet);
    }
}

void ProgressUpdateFrame::setProgressTip(const QString &content, const QString &stylesheet)
{
    m_promptLabel->setText(content);
    if (stylesheet.compare("")) {
        m_promptLabel->setStyleSheet(stylesheet);
    }
}
