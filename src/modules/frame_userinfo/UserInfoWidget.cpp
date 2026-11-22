#include "UserInfoWidget.h"
#include "CommonFunc.h"
#include "AvatarButton.h"
#include "SystemInfoTip.h"
#include "PasswordManager.h"
#include "UsernameManager.h"
#include "HostnameManager.h"
#include "KeyboardManager.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QAction>
#include <QCheckBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QLabel>
#include <QDebug>
#include <QRegularExpressionValidator>
#include <QTimer>
#include <QApplication>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <algorithm>
#include <unistd.h>

const int kWidthRange = 30;
const int kInfoItemSpace = 35;
const int kInputWidgetWidth = 340;
const int kLevelWidgetWidth = 100;

UserInfoWidget::UserInfoWidget(QWidget *partent)
    : QWidget (partent)
    , m_isHostnameEditedManually (false)
{
    initUI();
    updateTs();

    QTimer::singleShot(100, this, &UserInfoWidget::readConf);
}

void UserInfoWidget::showEvent(QShowEvent* event)
{
    QFile stateFile("/run/fedora-dde-installer/state.json");
    if (stateFile.open(QIODevice::ReadOnly)) {
        const QString createdUser = QJsonDocument::fromJson(stateFile.readAll()).object().value("user").toString();
        if (!createdUser.isEmpty()) {
            // A partial configuration retry must finish the account already
            // created in the target rather than create a second administrator.
            m_usernameComboBox->setCurrentText(createdUser);
            m_usernameComboBox->setEnabled(false);
            m_usernameEdit->setEnabled(false);
        }
    }
    checkNextButtonEnable();
    QWidget::showEvent(event);
}

void UserInfoWidget::initUI()
{
    // 大小写锁定提示图标
    m_capsLock = new QAction();
    m_capsLock->setIcon(QIcon(":/images/capslock.svg"));

    // 新建用户名
    m_usernameLabel = new QLabel;
    m_usernameEdit = new DLineEdit;
    m_usernameEdit->setContextMenuPolicy(Qt::NoContextMenu);

    m_usernameComboBox = new DComboBox;
    m_usernameComboBox->setFixedWidth(kInputWidgetWidth);
    m_usernameComboBox->setLineEdit(m_usernameEdit->lineEdit());
    m_usernameComboBox->setMaxVisibleItems(5);

    m_usernameReuseTip = new QLabel;
    m_usernameReuseTip->setFixedWidth(kInputWidgetWidth);
    m_usernameReuseTip->setFixedHeight(kInfoItemSpace);
    m_usernameReuseTip->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QFont font;
    font.setPixelSize(12);
    m_usernameReuseTip->setFont(font);

    m_usernameCheckTipPadding = new QLabel("");

    QHBoxLayout *usernameReuseTipLayout = new QHBoxLayout;
    usernameReuseTipLayout->setContentsMargins(0, 0, 0, 0);
    usernameReuseTipLayout->setSpacing(0);
    usernameReuseTipLayout->addStretch();
    usernameReuseTipLayout->addWidget(m_usernameCheckTipPadding, 0, Qt::AlignLeft | Qt::AlignVCenter);
    usernameReuseTipLayout->addWidget(m_usernameReuseTip, 0, Qt::AlignRight | Qt::AlignVCenter);
    usernameReuseTipLayout->addStretch();


    connect(m_usernameEdit, &DLineEdit::textChanged, [this]{
        m_usernameEdit->hideAlertMessage();
        m_usernameEdit->setAlert(false);

        if (!CommonFunc::getSettingsBool("DI_LOCK_HOSTNAME_CONFIG")) {
            QString name = m_usernameEdit->text();
            if (!name.isEmpty()) {
                name=name.trimmed();
                m_hostnameEdit->setText(name + CommonFunc::getSettingsString("DI_HOSTNAME_AUTO_SUFFIX_CONFIG"));
            } else {
                m_hostnameEdit->clear();
            }
        }
        checkNextButtonEnable();
    });

    connect(m_usernameEdit, &DLineEdit::editingFinished, [this] {
        if (!m_hostnameEdit->hasFocus() &&
        !m_passwordEdit->hasFocus() &&
        !m_passwordCheckEdit->hasFocus() &&
        !m_rootPasswordEdit->hasFocus() &&
        !m_rootPasswordCheckEdit->hasFocus()) {
            return ;
        }

        QString msg;
        if (!validateUsername(m_usernameEdit->text(), msg) && m_usernameComboBox->isActiveWindow()) {
            m_usernameEdit->showAlertMessage(msg);
            m_usernameEdit->setAlert(true);
        }
    });

    connect(m_usernameEdit, &DLineEdit::focusChanged, this, [=] (const bool onFocus){
        this->updateEditCapsLockState(this->m_usernameEdit, onFocus);
        m_usernameEdit->hideAlertMessage();
        //m_usernameEdit->setAlert(false);
    });

    m_usernameEdit->lineEdit()->setPlaceholderText(::QObject::tr("Username"));

    QHBoxLayout *usernameLayout = new QHBoxLayout;
    usernameLayout->setContentsMargins(0, 0, 0, 0);
    usernameLayout->setSpacing(0);
    usernameLayout->addStretch();
    usernameLayout->addWidget(m_usernameLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
    usernameLayout->addWidget(m_usernameComboBox, 0, Qt::AlignRight | Qt::AlignVCenter);
    usernameLayout->addStretch();

    // 主机名
    m_hostnameLabel = new QLabel;
    m_hostnameEdit = new DLineEdit;
    m_hostnameEdit->setContextMenuPolicy(Qt::NoContextMenu);
    m_hostnameEdit->setFixedWidth(kInputWidgetWidth);

    connect(m_hostnameEdit, &DLineEdit::textChanged, [this](){
        m_hostnameEdit->hideAlertMessage();
        m_hostnameEdit->setAlert(false);
        checkNextButtonEnable();
    });

    connect(m_hostnameEdit, &DLineEdit::editingFinished, [this](){
        if (!m_usernameEdit->hasFocus() &&
            !m_passwordEdit->hasFocus() &&
            !m_passwordCheckEdit->hasFocus() &&
            !m_rootPasswordEdit->hasFocus() &&
            !m_rootPasswordCheckEdit->hasFocus()) {
            return ;
        }

        QString msg;
        if (!validateHostname(m_hostnameEdit->text(), msg)) {
            m_hostnameEdit->showAlertMessage(msg);
            m_hostnameEdit->setAlert(true);
        }
    });

    connect(m_hostnameEdit, &DLineEdit::textEdited, this
            , &UserInfoWidget::onHostnameEdited);
    connect(m_hostnameEdit, &DLineEdit::focusChanged, this, [=] (const bool onFocus){
        this->updateEditCapsLockState(this->m_hostnameEdit, onFocus);
        m_hostnameEdit->hideAlertMessage();
        //m_hostnameEdit->setAlert(false);
    });

    m_hostnameEdit->lineEdit()->setPlaceholderText(::QObject::tr("Computer name"));

    QHBoxLayout *hostnameLayout = new QHBoxLayout;
    hostnameLayout->setContentsMargins(0, 0, 0, 0);
    hostnameLayout->setSpacing(0);
    hostnameLayout->addStretch();
    hostnameLayout->addWidget(m_hostnameLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
    hostnameLayout->addWidget(m_hostnameEdit, 0, Qt::AlignRight | Qt::AlignVCenter);
    hostnameLayout->addStretch();

    // 密码等级控件
    m_passwdLevelLabel = new QLabel;
    m_passwdLevelLabel->setFixedWidth(kLevelWidgetWidth);
    QLabel *spaceLabel = new QLabel;
    spaceLabel->setFixedWidth(kLevelWidgetWidth);

    // 新建用户密码
    m_passwordLabel = new QLabel;
    m_passwordEdit = new DPasswordEdit;
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setContextMenuPolicy(Qt::NoContextMenu);
    m_passwordEdit->layout()->setSpacing(7);
    m_passwordEdit->setFixedWidth(kInputWidgetWidth);

    connect(m_passwordEdit, &DLineEdit::textChanged, [this](){
        m_passwordEdit->hideAlertMessage();
        m_passwordEdit->setAlert(false);
        updatePasswdLevel();
        checkNextButtonEnable();
    });

    connect(m_passwordEdit, &DLineEdit::focusChanged, this, [=] (const bool onFocus){
        this->updateEditCapsLockState(this->m_passwordEdit, onFocus);
        m_passwordEdit->hideAlertMessage();
        //m_passwordEdit->setAlert(false);
    });

    connect(m_passwordEdit, &DPasswordEdit::editingFinished, this, [=](){
        QString msg;
        if (!validatePassword(m_usernameEdit->text(), m_passwordEdit->text(), msg)) {
            m_passwordEdit->showAlertMessage(msg);
            m_passwordEdit->setAlert(true);
        }
    });

    QHBoxLayout *passwordLayout = new QHBoxLayout;
    passwordLayout->setContentsMargins(0, 0, 0, 0);
    passwordLayout->setSpacing(0);
    passwordLayout->addStretch();
    passwordLayout->addWidget(spaceLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
    passwordLayout->addSpacing(5);
    passwordLayout->addWidget(m_passwordLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
    passwordLayout->addWidget(m_passwordEdit, 0, Qt::AlignRight | Qt::AlignVCenter);
    passwordLayout->addSpacing(5);
    passwordLayout->addWidget(m_passwdLevelLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
    passwordLayout->addStretch();

    // 新建用户密码确认
    m_passwordCheckLabel = new QLabel;
    m_passwordCheckEdit = new DPasswordEdit;
    m_passwordCheckEdit->setEchoMode(QLineEdit::Password);
    m_passwordCheckEdit->lineEdit()->setReadOnly(m_passwordEdit->lineEdit()->isReadOnly());
    m_passwordCheckEdit->setContextMenuPolicy(Qt::NoContextMenu);
    m_passwordCheckEdit->layout()->setSpacing(7);
    m_passwordCheckEdit->setFixedWidth(kInputWidgetWidth);

    connect(m_passwordCheckEdit, &DLineEdit::textChanged, [this](){
        m_passwordCheckEdit->hideAlertMessage();
        m_passwordCheckEdit->setAlert(false);
        checkNextButtonEnable();
    });

    connect(m_passwordCheckEdit, &DLineEdit::focusChanged, this, [=] (const bool onFocus){
        this->updateEditCapsLockState(this->m_passwordCheckEdit, onFocus);
        m_passwordCheckEdit->hideAlertMessage();
        //m_passwordCheckEdit->setAlert(false);
    });

    QHBoxLayout *passwordCheckLayout = new QHBoxLayout;
    passwordCheckLayout->setContentsMargins(0, 0, 0, 0);
    passwordCheckLayout->setSpacing(0);
    passwordCheckLayout->addStretch();
    passwordCheckLayout->addWidget(m_passwordCheckLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
    passwordCheckLayout->addWidget(m_passwordCheckEdit, 0, Qt::AlignRight | Qt::AlignVCenter);
    passwordCheckLayout->addStretch();

    m_passwordEdit->lineEdit()->setPlaceholderText(::QObject::tr("Password"));
    m_passwordCheckEdit->lineEdit()->setPlaceholderText(::QObject::tr("Repeat password"));

    // 配置root用户开关
    m_setRootPasswordCheck = new QCheckBox;
    m_setRootPasswordCheck->setChecked(false);

    // root用户名显示
    m_rootUserLabel = new QLabel;
    DLineEdit* rootUser = new DLineEdit;
    rootUser->setEnabled(false);
    rootUser->setFixedWidth(kInputWidgetWidth - kWidthRange);
    rootUser->setContextMenuPolicy(Qt::NoContextMenu);
    rootUser->lineEdit()->setPlaceholderText("root");

    QHBoxLayout *rootUserFrameLayout = new QHBoxLayout;
    rootUserFrameLayout->setContentsMargins(0, 0, 0, 0);
    rootUserFrameLayout->setSpacing(0);
    rootUserFrameLayout->addStretch();
    rootUserFrameLayout->addWidget(m_rootUserLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
    rootUserFrameLayout->addWidget(rootUser, 0, Qt::AlignRight | Qt::AlignVCenter);
    rootUserFrameLayout->addStretch();

    // root用户密码
    m_rootPasswordLabel = new QLabel;
    m_rootPasswordEdit = new DPasswordEdit;
    m_rootPasswordEdit->setEchoMode(QLineEdit::Password);
    m_rootPasswordEdit->setContextMenuPolicy(Qt::NoContextMenu);
    m_rootPasswordEdit->setFixedWidth(kInputWidgetWidth - kWidthRange);

    connect(m_rootPasswordEdit, &DLineEdit::textChanged, [this](){
        m_rootPasswordEdit->hideAlertMessage();
        m_rootPasswordEdit->setAlert(false);
        checkNextButtonEnable();
    });
    connect(m_rootPasswordEdit, &DLineEdit::focusChanged, this, [=] (const bool onFocus){
        this->updateEditCapsLockState(this->m_rootPasswordEdit, onFocus);
        m_rootPasswordEdit->hideAlertMessage();
        m_rootPasswordEdit->setAlert(false);
    });

    // root密码等级控件
    m_rootPasswdLevelLabel = new QLabel;
    m_rootPasswdLevelLabel->setFixedWidth(kLevelWidgetWidth);

    QLabel *rootSpaceLabel = new QLabel;
    rootSpaceLabel->setFixedWidth(kLevelWidgetWidth);
    QHBoxLayout *rootPasswordLayout = new QHBoxLayout;
    rootPasswordLayout->setContentsMargins(0, 0, 0, 0);
    rootPasswordLayout->setSpacing(0);
    rootPasswordLayout->addStretch();
    rootPasswordLayout->addWidget(rootSpaceLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
    rootPasswordLayout->addSpacing(5);
    rootPasswordLayout->addWidget(m_rootPasswordLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
    rootPasswordLayout->addWidget(m_rootPasswordEdit, 0, Qt::AlignRight | Qt::AlignVCenter);
    rootPasswordLayout->addSpacing(5);
    rootPasswordLayout->addWidget(m_rootPasswdLevelLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
    rootPasswordLayout->addStretch();

    // root用户密码确认
    m_rootPasswordCheckLabel = new QLabel;
    m_rootPasswordCheckEdit = new DPasswordEdit;
    m_rootPasswordCheckEdit->setEchoMode(QLineEdit::Password);
    m_rootPasswordCheckEdit->setContextMenuPolicy(Qt::NoContextMenu);
    m_rootPasswordCheckEdit->setFixedWidth(kInputWidgetWidth - kWidthRange);

    connect(m_rootPasswordCheckEdit, &DLineEdit::textChanged, [this](){
        m_rootPasswordCheckEdit->hideAlertMessage();
        m_rootPasswordCheckEdit->setAlert(false);
        checkNextButtonEnable();
    });

    connect(m_rootPasswordCheckEdit, &DLineEdit::focusChanged, this, [=] (const bool onFocus){
        this->updateEditCapsLockState(this->m_rootPasswordCheckEdit, onFocus);
        m_rootPasswordCheckEdit->hideAlertMessage();
        m_rootPasswordCheckEdit->setAlert(false);
    });

    QHBoxLayout *rootPasswordCheckLayout = new QHBoxLayout;
    rootPasswordCheckLayout->setContentsMargins(0, 0, 0, 0);
    rootPasswordCheckLayout->setSpacing(0);
    rootPasswordCheckLayout->addStretch();
    rootPasswordCheckLayout->addWidget(m_rootPasswordCheckLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
    rootPasswordCheckLayout->addWidget(m_rootPasswordCheckEdit, 0, Qt::AlignRight | Qt::AlignVCenter);
    rootPasswordCheckLayout->addStretch();

    // 应用折叠控件封装root用户信息编辑控件
    QVBoxLayout* rootInfoLayout = new QVBoxLayout;
    rootInfoLayout->setContentsMargins(0, 0, 0, 0);
    rootInfoLayout->setSpacing(0);

    rootInfoLayout->addLayout(rootUserFrameLayout);
    rootInfoLayout->addSpacing(kInfoItemSpace);
    rootInfoLayout->addLayout(rootPasswordLayout);
    rootInfoLayout->addSpacing(kInfoItemSpace);
    rootInfoLayout->addLayout(rootPasswordCheckLayout);

    QWidget *rootInfoWidget = new QWidget;
    rootInfoWidget->setLayout(rootInfoLayout);

    m_wrapWidget = new WrapWidget(m_setRootPasswordCheck, rootInfoWidget);
    connect(m_setRootPasswordCheck, &QCheckBox::clicked, this, [=] (const bool checked){
        emit m_wrapWidget->headClicked(checked);
        this->checkNextButtonEnable();
    });

    // 主布局加入各个控件
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    int itemSpace = CommonFunc::isBigScreen() ? kInfoItemSpace : 10;
    layout->addLayout(usernameLayout);
    layout->addLayout(usernameReuseTipLayout, Qt::AlignLeft);
    //layout->addSpacing(itemSpace);
    layout->addLayout(hostnameLayout);
    layout->addSpacing(itemSpace);
    layout->addLayout(passwordLayout);
    layout->addSpacing(itemSpace);
    layout->addLayout(passwordCheckLayout);
    layout->addSpacing(itemSpace);

    m_setRootPasswordCheck->setChecked(false);
    m_wrapWidget->hide();

    setLayout(layout);

    connect(KeyboardManager::instance()
            , &KeyboardManager::capslockStatusChanged
            , this
            , &UserInfoWidget::updateCapsLockState);
}

void UserInfoWidget::readConf()
{
    // No live-session accounts or first-boot state are imported into the target.
    m_usernameReuseTip->hide();
    const QString username = CommonFunc::getSettingsString("DI_USERNAME");
    const QString hostname = CommonFunc::getSettingsString("DI_HOSTNAME");
    if (!username.isEmpty())
        m_usernameComboBox->setCurrentText(username);
    if (!hostname.isEmpty())
        m_hostnameEdit->setText(hostname);
}

bool UserInfoWidget::writeConf()
{
    // Secrets stay in the root-only runtime file. Never place a password in a
    // command argument, a log message, or a reversibly encrypted static-key file.
    const QString path = CommonFunc::getSystemEnv(DI_CONFIG_FILE_ENV);
    const QFileInfo info(path);
    const QFileInfo parent(info.absolutePath());
    if (QDir::cleanPath(path) != "/run/fedora-dde-installer/installer.conf"
            || geteuid() != 0 || !info.isFile() || info.isSymLink() || info.ownerId() != 0
            || parent.isSymLink() || parent.ownerId() != 0
            || !QFile::setPermissions(parent.absoluteFilePath(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner)
            || !QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
        return false;
    }
    CommonFunc::setSettingsValue("DI_USERNAME", m_usernameComboBox->currentText());
    CommonFunc::setSettingsValue("DI_HOSTNAME", m_hostnameEdit->text());
    CommonFunc::setSettingsValue("DI_PASSWORD_ENCRYPTION_CONFIG", false);
    CommonFunc::setSettingsValue("DI_ROOT_PASSWORD", "");
    CommonFunc::setSettingsValue("DI_PASSWORD", m_passwordEdit->text());
    return QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner)
            && CommonFunc::getSettingsString("DI_PASSWORD") == m_passwordEdit->text();
}

void UserInfoWidget::onUsernameTextChanged()
{
    QString msg;

    if (!validateUsername(m_usernameEdit->text(), msg) && m_usernameEdit->isActiveWindow()) {
        m_usernameEdit->showAlertMessage(msg);
        m_usernameEdit->setAlert(true);
    }
}

void UserInfoWidget::onHostnameTextChanged()
{
    QString msg;

    if (!validateHostname(m_hostnameEdit->text(), msg) && m_hostnameEdit->isActiveWindow()) {
        m_hostnameEdit->showAlertMessage(msg);
        m_hostnameEdit->setAlert(true);
    }
}

// 控件之间输入有联动（比如用户名和主机名）
void UserInfoWidget::checkNextButtonEnable()
{
    bool enable = true;

    enable = !m_usernameEdit->text().isEmpty()
             && !m_hostnameEdit->text().isEmpty()
             && !m_passwordEdit->text().isEmpty()
             && !m_passwordCheckEdit->text().isEmpty();

    if (m_setRootPasswordCheck->isChecked()) {
        enable = enable
                 && !m_rootPasswordEdit->text().isEmpty()
                 && !m_rootPasswordCheckEdit->text().isEmpty();
    }

    emit nextButtonEnable(enable);
}

bool UserInfoWidget::checkAllEditText()
{
    QString msg;

    if (!validateUsername(m_usernameEdit->text(), msg)) {
        m_usernameEdit->showAlertMessage(msg);
        m_usernameEdit->setAlert(true);
        return false;
    }

    if (!validateHostname(m_hostnameEdit->text(), msg)) {
        m_hostnameEdit->showAlertMessage(msg);
        m_hostnameEdit->setAlert(true);
        return false;
    }

    if (!validatePassword(m_usernameEdit->text(), m_passwordEdit->text(), msg)) {
        m_passwordEdit->showAlertMessage(msg);
        m_passwordEdit->setAlert(true);
        return false;
    }

    if (!validateCheckPassword(m_passwordEdit->text(), m_passwordCheckEdit->text(), msg)) {
        m_passwordCheckEdit->showAlertMessage(msg);
        m_passwordCheckEdit->setAlert(true);
        return false;
    }

    if (m_setRootPasswordCheck->isChecked()) {
        if (!validatePassword("root", m_rootPasswordEdit->text(), msg)) {
            m_rootPasswordEdit->showAlertMessage(msg);
            m_rootPasswordEdit->setAlert(true);
            return false;
        }

        if (!validateCheckPassword(m_rootPasswordEdit->text(), m_rootPasswordCheckEdit->text(), msg)) {
            m_rootPasswordCheckEdit->showAlertMessage(msg);
            m_rootPasswordCheckEdit->setAlert(true);
            return false;
        }
    }

    return true;
}

void UserInfoWidget::onUsernameEdited()
{
    m_usernameEdit->hideAlertMessage();
    m_usernameEdit->setAlert(false);

    // Be modified by caimc for bug 186377 in 2023/02/13
    if (!m_isHostnameEditedManually && !CommonFunc::getSettingsBool("DI_LOCK_HOSTNAME_CONFIG")) {
        QString name = m_usernameEdit->text();
        if (!name.isEmpty()) {
            name = name.replace("_", "")
                   + CommonFunc::getSettingsString("DI_HOSTNAME_AUTO_SUFFIX_CONFIG");
        }

        m_hostnameEdit->setText(name);
    }

    checkNextButtonEnable();
    updatePasswdLevel();
}

void UserInfoWidget::onHostnameEdited()
{
    m_hostnameEdit->hideAlertMessage();
    m_hostnameEdit->setAlert(false);

    m_isHostnameEditedManually = true;

    checkNextButtonEnable();
}

bool UserInfoWidget::validateUsername(const QString &username, QString& msg)
{
    return UsernameManager::instance()->checked(username, msg);
}

bool UserInfoWidget::validateHostname(const QString &hostname, QString& msg)
{
    return HostnameManager::instance()->checked(hostname, msg);
}

bool UserInfoWidget::validatePassword(const QString &user,
                                      const QString &passwd,
                                      QString& msg)
{
    return PasswordManager::instance()->checked(user, passwd, msg);
}

bool UserInfoWidget::validateCheckPassword(const QString &passwd, const QString &checkPasswd, QString &msg)
{
    if (passwd != checkPasswd) {
        msg = ::QObject::tr("Passwords do not match");
        return false;
    }

    return true;
}

void UserInfoWidget::updatePasswdLevel()
{
    QPalette palette;
    PasswdLevel level;

    m_passwdLevelLabel->clear();
    if (!m_passwordEdit->text().isEmpty()) {
        level = PasswordManager::instance()->passwdLevel(m_usernameEdit->text(), m_passwordEdit->text());
        palette = m_passwdLevelLabel->palette();
        palette.setColor(QPalette::Text,  PasswordManager::instance()->passwdLevel(level).second);

        m_passwdLevelLabel->setPalette(palette);
        m_passwdLevelLabel->setForegroundRole(QPalette::Text);
        m_passwdLevelLabel->setText(PasswordManager::instance()->passwdLevel(level).first);
    }
}

void UserInfoWidget::updateRootPasswdLevel()
{
    QPalette palette;
    PasswdLevel level;

    if (!m_setRootPasswordCheck->isChecked()) {
        return;
    }

    m_rootPasswdLevelLabel->clear();
    if (!m_rootPasswordEdit->text().isEmpty()) {
        level = PasswordManager::instance()->passwdLevel("root", m_rootPasswordEdit->text());
        palette = m_rootPasswdLevelLabel->palette();
        palette.setColor(QPalette::Text, PasswordManager::instance()->passwdLevel(level).second);

        m_rootPasswdLevelLabel->setPalette(palette);
        m_rootPasswdLevelLabel->setForegroundRole(QPalette::Text);
        m_rootPasswdLevelLabel->setText(PasswordManager::instance()->passwdLevel(level).first);
    }
}

void UserInfoWidget::updateCapsLockState(bool lock)
{
    QLineEdit* edit = dynamic_cast<QLineEdit*>(focusWidget());
    if (edit == nullptr) {
        return;
    }

    if (lock) {
        edit->addAction(m_capsLock, QLineEdit::TrailingPosition);
        return;
    }

    edit->removeAction(m_capsLock);
}

void UserInfoWidget::updateEditCapsLockState(DLineEdit *edit, bool lock)
{
    if (lock && KeyboardManager::instance()->isCapslockOn()) {
        edit->lineEdit()->addAction(m_capsLock, QLineEdit::TrailingPosition);
        return;
    }

    edit->lineEdit()->removeAction(m_capsLock);
}

void UserInfoWidget::updateTs()
{
    m_usernameLabel->setText(::QObject::tr("Username").append(" :"));
    m_usernameEdit->lineEdit()->setPlaceholderText(::QObject::tr("Username"));
    m_usernameReuseTip->setText(QObject::tr("Create an account with the same name to reuse"));

    m_hostnameLabel->setText(::QObject::tr("Computer name").append(" :"));
    m_hostnameEdit->lineEdit()->setPlaceholderText(::QObject::tr("Computer name"));

    m_passwordLabel->setText(::QObject::tr("Password").append(" :"));
    m_passwordEdit->lineEdit()->setPlaceholderText(::QObject::tr("Password"));

    m_passwordCheckLabel->setText(::QObject::tr("Repeat password").append(" :"));
    m_passwordCheckEdit->lineEdit()->setPlaceholderText(::QObject::tr("Repeat password"));

    m_setRootPasswordCheck->setText(::QObject::tr("Enable root user"));

    m_rootUserLabel->setText(::QObject::tr("Username").append(" :"));

    m_rootPasswordLabel->setText(::QObject::tr("Root password").append(" :"));
    m_rootPasswordEdit->lineEdit()->setPlaceholderText(::QObject::tr("Root password"));

    m_rootPasswordCheckLabel->setText(::QObject::tr("Repeat root password").append(" :"));
    m_rootPasswordCheckEdit->lineEdit()->setPlaceholderText(::QObject::tr("Repeat root password"));

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    int array[] = {m_usernameLabel->fontMetrics().horizontalAdvance(m_usernameLabel->text()),
                   m_hostnameLabel->fontMetrics().horizontalAdvance(m_hostnameLabel->text()),
                   m_passwordLabel->fontMetrics().horizontalAdvance(m_passwordLabel->text()),
                   m_passwordCheckLabel->fontMetrics().horizontalAdvance(m_passwordCheckLabel->text())};
#else
    int array[] = {m_usernameLabel->fontMetrics().width(m_usernameLabel->text()),
                   m_hostnameLabel->fontMetrics().width(m_hostnameLabel->text()),
                   m_passwordLabel->fontMetrics().width(m_passwordLabel->text()),
                   m_passwordCheckLabel->fontMetrics().width(m_passwordCheckLabel->text())};
#endif
    int maxValue = *std::max_element(array, array + sizeof(array)/sizeof(array[0])) + 20;
    m_usernameLabel->setFixedWidth(maxValue);
    m_hostnameLabel->setFixedWidth(maxValue);
    m_passwordLabel->setFixedWidth(maxValue);
    m_passwordCheckLabel->setFixedWidth(maxValue);
    m_rootUserLabel->setFixedWidth(maxValue + kWidthRange);
    m_rootPasswordLabel->setFixedWidth(maxValue + kWidthRange);
    m_rootPasswordCheckLabel->setFixedWidth(maxValue + kWidthRange);
    m_usernameCheckTipPadding->setFixedWidth(maxValue);

    updatePasswdLevel();
    updateRootPasswdLevel();
}
