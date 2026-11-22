#include "UserInfoFrame.h"
#include "AvatarButton.h"
#include "CommonFunc.h"
#include "InstallConfirmDialog.h"
#include "PartitionOperateManager.h"
#include "TimezoneFrame.h"
#include "TitleWidget.h"


#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <DIconButton>
#include <DDialog>
#include <QListView>
#include <QApplication>
#include <libintl.h>
#include <QDBusInterface>
#include <DPlatformWindowHandle>
#include <QTimer>
#include <QRandomGenerator>

DWIDGET_USE_NAMESPACE


// 用户对于键盘的修改规则：语言修改，那么键盘改变，覆盖之前用户的键盘选择。
UserInfoFrame::UserInfoFrame(QWidget *parent)
    : FramePluginInterface (parent)
{
    m_frameName = gFrameUserInfo;
    hideBackButton();
    hideCloseButton();
    // 初始化数据部分
    m_langItemList = CommonFunc::getLanguageList();

    KeyboardLayoutManager::instance()->xkbLayoutMap(m_layoutMap);

    // 初始化视图部分
    initUI();
    installEventFilter(this);
}

bool UserInfoFrame::load()
{
    return true;
}

void UserInfoFrame::showEvent(QShowEvent *event)
{
    FramePluginInterface::showEvent(event);
}

bool UserInfoFrame::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_networkWidget && event->type() == QEvent::MouseButtonPress) {
        if (m_networkWidget != nullptr) {
            m_networkWidget->hide();
        }
    }
    return FramePluginInterface::eventFilter(watched, event);
}

void UserInfoFrame::initUI()
{
    // 标题栏
    m_titleWidget = new TitleWidget(this);
    QHBoxLayout *titleMainLayout = new QHBoxLayout;
    titleMainLayout->addWidget(m_titleWidget);

    // 用户图标按钮
    QString avatar = CommonFunc::getSettingsString("DI_AVATAR");
    if (!QFile::exists(avatar)) {
        QStringList avatarList = CommonFunc::getAvatars();
        if (avatarList.size() > 0) {
            int index = QRandomGenerator::global()->bounded(avatarList.size());
            avatar = avatarList.at(index);
        }
    }

    m_currentAvatarButton = new AvatarButton;
    m_currentAvatarButton->setCheckable(false);
    m_currentAvatarButton->updateIcon(avatar);
    m_currentAvatarButton->installEventFilter(this);

    m_avatarListWidget = new AvatarListWidget(this);
    m_avatarListWidget->setAvatars(CommonFunc::getAvatars());
    m_avatarListWidget->setCurrentAvatar(avatar);

    connect(m_avatarListWidget, &AvatarListWidget::selectAvatar, this, [=] (const QString& avatarPath){
        m_currentAvatarButton->updateIcon(avatarPath);
    });

    m_avatarListWidget->hide();
    connect(m_currentAvatarButton, &QPushButton::clicked, this, [=] {
        m_avatarListWidget->setCurrentAvatar(m_currentAvatarButton->avatar());
        m_avatarListWidget->exec();
    });

    // 新建用户信息编辑
    m_userInfoWidget = new UserInfoWidget;
    m_userInfoWidget->installEventFilter(this);
    for (auto child : m_userInfoWidget->findChildren<QWidget*>()) {
        child->installEventFilter(this);
    }

    connect(m_userInfoWidget, &UserInfoWidget::nextButtonEnable, this, [=] (const bool enable) {
        m_nextBnt->setEnabled(enable);
    });

    ScrollAreaWidget *area = new ScrollAreaWidget(m_userInfoWidget);
    area->setFocusPolicy(Qt::NoFocus);
    // 下一步按钮
    m_nextBnt = new DSuggestButton;
    m_nextBnt->setFixedSize(gnextbuttonwidth, gnextbuttonheight);
    m_nextBnt->setDefault(true);
    connect(m_nextBnt, &QPushButton::clicked, this, [=] {
        if (m_userInfoWidget->checkAllEditText()) {
            onNextButtonClicked();
        }
    });

    QSize iconSize(24, 24);

    m_cbKeyboard = new IconComboBox(this);
    m_cbKeyboard->button()->setIconSize(iconSize);
    m_cbKeyboard->setIcon(QIcon(":/images/keyboardLayout.svg"));
    //m_cbKeyboard->button()->setFocusPolicy(Qt::TabFocus);
    m_cbKeyboard->setMaxVisibleCount(6);
    m_cbKeyboard->setVisible(false);
    connect(m_cbKeyboard->comboBox(), SIGNAL(currentIndexChanged(int)), this, SLOT(onKeyboardMenuSelected(int)));
    for (auto child : m_cbKeyboard->findChildren<QWidget*>()) {
        child->installEventFilter(this);
    }

    // 时区选择
    m_timezoneBtn = new DIconButton;
    //m_timezoneBtn->setFocusPolicy(Qt::TabFocus);
    m_timezoneBtn->setIconSize(iconSize);
    m_timezoneBtn->setIcon(QIcon(":/images/timezoneSet.svg"));
    m_timezoneBtn->setVisible(false);
    m_timezoneBtn->installEventFilter(this);


    m_timezone = new TimezoneFrame(this);
    DPlatformWindowHandle *timezoneHandle = new DPlatformWindowHandle(m_timezone);
    timezoneHandle->setWindowRadius(18);
    connect(m_timezoneBtn, &DIconButton::clicked, this, [=] {
        m_timezone->exec();
    });

    m_cbLanguage = new IconComboBox;
    m_cbLanguage->button()->setIconSize(iconSize);
    m_cbLanguage->setIcon(QIcon(":/images/language.svg"));
    //m_cbLanguage->button()->setFocusPolicy(Qt::TabFocus);
    m_cbLanguage->setVisible(false);
    //connect(m_cbLanguage->comboBox(), SIGNAL(currentIndexChanged(int)), this, SLOT(onLanguageMenuSelected(int)));
    for (auto child : m_cbLanguage->findChildren<QWidget*>()) {
        child->installEventFilter(this);
    }

    m_networkBtn = new DIconButton;
    m_networkBtn->setFocusPolicy(Qt::TabFocus);
    m_networkBtn->setIconSize(iconSize);
    m_networkBtn->setIcon(QIcon(":/images/network.svg"));

    // 键盘选择、时区、语言的布局
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setSpacing(0);
    hLayout->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *leftLayout = new QHBoxLayout;
    //
    int btnSpacing = CommonFunc::isBigScreen() ? 20 : 10;
    leftLayout->addSpacing(btnSpacing);
    //leftLayout->addWidget(m_cbKeyboard, 0, Qt::AlignLeft | Qt::AlignBottom);
    //leftLayout->addSpacing(btnSpacing);
    //leftLayout->addWidget(m_timezoneBtn, 0, Qt::AlignLeft | Qt::AlignBottom);
    //leftLayout->addSpacing(btnSpacing);
    //leftLayout->addWidget(m_cbLanguage, 0, Qt::AlignLeft | Qt::AlignBottom);
    //leftLayout->addSpacing(btnSpacing);
    leftLayout->addWidget(m_networkBtn, 0, Qt::AlignLeft | Qt::AlignBottom);
    leftLayout->addStretch();

    hLayout->addLayout(leftLayout);
    hLayout->addWidget(m_nextBnt, 0, Qt::AlignHCenter | Qt::AlignBottom);
    hLayout->addStretch();

    m_quickLogin = new DCheckBox;
    m_quickLogin->hide();
    m_quickLogin->setChecked(false);

    // 主布局
    m_contentLayout->addLayout(titleMainLayout);
    m_contentLayout->addSpacing(CommonFunc::isBigScreen() ? 30 : 20);
    m_contentLayout->addWidget(m_currentAvatarButton, 0, Qt::AlignHCenter);
    m_contentLayout->addSpacing(CommonFunc::isBigScreen() ? 30: 10);
    m_contentLayout->addWidget(area, 0, Qt::AlignHCenter);
    m_contentLayout->addStretch();
    m_contentLayout->addWidget(m_quickLogin, 0, Qt::AlignHCenter);
    m_contentLayout->addSpacing(10);
    m_contentLayout->addLayout(hLayout);
    m_contentLayout->addSpacing(CommonFunc::getInnerMargin());

}

void UserInfoFrame::updateTs()
{
    m_titleWidget->setTitleLabelProperty(::QObject::tr("Create Accounts"), "");

    m_titleWidget->setTipsLabelProperty(tr("The system is installed. Create your administrator account and computer name before restarting."), "font-size:16px;font-weight:Medium;");

    m_userInfoWidget->updateTs();
    m_quickLogin->setText(::QObject::tr("Quick Login"));
    m_nextBnt->setText(::QObject::tr("Next"));
    m_cbKeyboard->setToolTip(::QObject::tr("Keyboard"));
    m_timezoneBtn->setToolTip(::QObject::tr("Timezone"));
    m_cbLanguage->setToolTip(::QObject::tr("Languages"));
    m_networkBtn->setToolTip(::QObject::tr("Network"));
}

bool UserInfoFrame::writeConf()
{
    if (!m_userInfoWidget->writeConf())
        return false;
    CommonFunc::setSettingsValue("DI_QUICK_LOGIN",  m_quickLogin->isVisible() && m_quickLogin->isChecked());
    CommonFunc::setSettingsValue("DI_AVATAR", m_currentAvatarButton->avatar());
    return true;
}

void UserInfoFrame::initKeyboard()
{
    // 设置键盘列表数据
    QString locale(CommonFunc::getSettingsString("DI_LOCALE"));

    updateVariantListByLocale(locale);
    localizeVariantList(locale);

    // 设置默认选中
    QString layout(CommonFunc::getSettingsString("DI_LAYOUT"));
    QString variant(CommonFunc::getSettingsString("DI_LAYOUT_VARIANT"));
    if (variant.isEmpty()) {
        variant = layout;
    }

    QStringList descList;
    int index = -1;

    for (int i = 0; i < m_variantList.length(); ++i) {
        descList << m_variantList.at(i).description;
        if (m_variantList.at(i).name == variant
                && m_variantList.at(i).layoutName == layout) {
            index = i;
        }
    }

    m_cbKeyboard->comboBox()->clear();
    m_cbKeyboard->comboBox()->addItems(descList);


    if (index > 0) {
        m_cbKeyboard->comboBox()->setCurrentIndex(index);
    } else {
        //每次显示将选中项默认给第一项
        m_cbKeyboard->comboBox()->setCurrentIndex(0);
    }
}

void UserInfoFrame::onKeyboardMenuSelected(int index)
{
    if (m_networkWidget) {
        m_networkWidget->hide();
    }

    if (0 <= index && index < m_variantList.length()) {
        QString layout = m_variantList.at(index).layoutName;
        QString variant = m_variantList.at(index).name;
        if (layout == variant) {
            variant.clear();
        }
        CommonFunc::setSettingsValue("DI_LAYOUT", layout);
        CommonFunc::setSettingsValue("DI_LAYOUT_VARIANT", variant);
    }
}

void UserInfoFrame::initLanguage()
{
    // 设置语言列表数据
    QStringList langList;
    int cur = -1;
    QString locale(CommonFunc::getSettingsString("DI_LOCALE"));

    for (int idx = 0; idx < m_langItemList.length(); ++idx) {
        if (m_langItemList.at(idx).locale == locale) {
            cur = idx;
        }

        langList << m_langItemList.at(idx).localName;
    }
    m_cbLanguage->comboBox()->clear();
    m_cbLanguage->comboBox()->addItems(langList);


    if (cur >= 0) {
        m_cbLanguage->comboBox()->setCurrentIndex(cur);
        qInfo() << QString("Popup menu set current checked locale %1").arg(locale);
    }
}

void UserInfoFrame::onLanguageMenuSelected(int index)
{
    if (0 <= index && index < m_langItemList.length()) {
        const QString locale = m_langItemList.at(index).locale;
        CommonFunc::setSettingsValue("DI_LOCALE", locale);

        // 更新翻译词典
        Q_EMIT onChangeLanguage(locale);
        // 更新当前页面的翻译
        updateTs();

        updateTimezoneConf(locale);

        updateKeyboardLayoutConf(locale);

        initKeyboard();
    }
}

void UserInfoFrame::onNextButtonClicked()
{
    if (!CommonFunc::installWorkflow().canEditUser())
        return;
    if (!writeConf()) {
        DDialog dialog(this);
        dialog.setTitle(tr("Could not save account details"));
        dialog.setMessage(tr("The installer could not access its private configuration file. Account details have not been submitted."));
        dialog.addButton(tr("OK"), true);
        dialog.exec();
        return;
    }
    Q_EMIT next();
}

void UserInfoFrame::localizeVariantList(const QString &locale)
{
    QString oldLocale = setlocale(LC_ALL, nullptr);
    qInfo() << QString("Before localized keyboard layout, old locale is %1").arg(oldLocale);

    (void) setlocale(LC_ALL, QString(locale).append(".UTF-8").toStdString().c_str());

    const char xkbDomain[] = "xkeyboard-config";
    for (auto &iter : m_variantList) {
        iter.description = dgettext(xkbDomain, iter.description.toLocal8Bit().constData());
    }

    (void) setlocale(LC_ALL, oldLocale.toStdString().c_str());
}

bool UserInfoFrame::updateVariantListByLocale(const QString &locale)
{
    QString layoutName(CommonFunc::get2AlphaCountryCode(locale).toLower());
    m_variantList = m_layoutMap.value("us");

    // 如果所选的语言没有对应的键盘，那么键盘列表不发生改变
    if (m_layoutMap.contains(layoutName)) {
        m_variantList = m_layoutMap.value(layoutName);
    } else {
        if (!layoutName.compare("hk", Qt::CaseInsensitive) || !layoutName.compare("tw", Qt::CaseInsensitive)) {
            m_variantList = m_layoutMap.value("cn");
        }
    }

    return !m_variantList.isEmpty();
}

void UserInfoFrame::updateKeyboardLayoutConf(const QString &locale)
{
    QString layoutName(CommonFunc::get2AlphaCountryCode(locale).toLower());

    // 根据语言设置更新键盘设置数据
    if (!m_layoutMap.contains(layoutName)) {
        qWarning() << QString("There is no keyboard setting for locale %1").arg(locale);
    } else {
        m_variantList = m_layoutMap.value(layoutName);

        if (m_variantList.isEmpty()) {
            qWarning() << QString("Thers is not variant in layout:%1").arg(layoutName);
        }

        QString variant = m_variantList.first().name;
        if (layoutName == variant) {
            variant.clear();
        }

        CommonFunc::setSettingsValue("DI_LAYOUT", layoutName);
        CommonFunc::setSettingsValue("DI_LAYOUT_VARIANT", variant);
    }
}

void UserInfoFrame::updateTimezoneConf(const QString &locale)
{
    // 根据语言设置更新时区设置数据
    QString tm = CommonFunc::getSettingsString("DI_TIMEZONE");

    if (tm.isEmpty() || !TimezoneFrame::isTimezoneSetByUser()) {
        for (auto &it : m_langItemList) {
            if (it.locale == locale) {
                tm = it.timezone;
                break;
            }
        }

        if (tm.isEmpty()) {
            qWarning() << "Timezone is empty in all places";
        } else {
            CommonFunc::setSettingsValue("DI_TIMEZONE", tm);
        }
    }
}

bool UserInfoFrame::searchDevice() {
    QDBusInterface fingerprintInterface("org.deepin.dde.Authenticate1",
                                        "/org/deepin/dde/Authenticate1/Fingerprint",
                                        "org.deepin.dde.Authenticate1.Fingerprint",
                                        QDBusConnection::systemBus());
    QVariant  defaultDevice(fingerprintInterface.property("DefaultDevice"));
    if (defaultDevice.userType() != QMetaType::QString) return false;
    QString deviceName(defaultDevice.toString());

    return !deviceName.isEmpty();
}
