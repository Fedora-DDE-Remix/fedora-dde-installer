#include "PasswordManager.h"
#include "CommonFunc.h"

#include <QDebug>
#include <QRegularExpression>
#include <QProcess>

// pw-check库1052中新加了枚举变量，为了保持低版本兼容，使用定义的枚举值对应的int值
#define PW_CHARACTER_TYPE_TOO_FEW 14
#define PW_USER_PASSWD_DUPLICATE 15

PasswordManager *PasswordManager::instance()
{
    static PasswordManager pm;
    return &pm;
}

PasswordManager::PasswordManager()
{
    m_passwdMinLength = CommonFunc::getSettingsString("DI_PASSWORD_MIN_LEN_CONFIG");
    m_passwdMaxLength = CommonFunc::getSettingsString("DI_PASSWORD_MAX_LEN_CONFIG");
    m_passwdValidatePolicy = CommonFunc::getSettingsString("DI_PASSWORD_VALIDATE_CONFIG");
    m_passwdValidateRequired = CommonFunc::getSettingsString("DI_PASSWORD_VALIDATE_REQUIRED_CONFIG");
    m_strongPasswordEnable = CommonFunc::getSettingsString("DI_PASSWORD_STRONG_CHECK_CONFIG");
    m_passwdPalindromeLength = CommonFunc::getSettingsString("DI_PASSWORD_PALINGROME_LENGTH_CONFIG");
    m_passwdMonotonousLength = CommonFunc::getSettingsString("DI_PASSWORD_MONOTONOUS_LENGTH_CONFIG");
    m_passwdContinuousLength = CommonFunc::getSettingsString("DI_PASSWORD_CONTINUOUS_LENGTH_CONFIG");

    setup();
}

void PasswordManager::setup()
{
    QString info = "/etc/deepin/dde.conf";
    QString cmd = "/usr/bin/pwd-conf-update";
    QFile file(info);

    file.open(QIODevice::WriteOnly);
    if (file.isOpen()) {
        QStringList list= {"[Password]",
                       QString("STRONG_PASSWORD=%1").arg(m_strongPasswordEnable),
                       QString("PASSWORD_MIN_LENGTH=%1").arg(m_passwdMinLength),
                       QString("PASSWORD_MAX_LENGTH=%1").arg(m_passwdMaxLength),
                       QString("VALIDATE_POLICY=\"%1\"").arg(m_passwdValidatePolicy),
                       QString("VALIDATE_REQUIRED=%1").arg(m_passwdValidateRequired),
                       QString("PALINDROME_NUM=%1").arg(m_passwdPalindromeLength),
                       QString("CONSECUTIVE_SAME_CHARACTER_NUM=%1").arg(m_passwdContinuousLength),
                       QString("MONOTONE_CHARACTER_NUM=%1").arg(m_passwdMonotonousLength),
                  };

        // 更新密码配置
        file.write(list.join("\n").toUtf8() + "\n");
        file.close();

        QString out;
        if (!CommonFunc::command(cmd, QStringList(), out)) {
           qWarning() << QString("PasswordManager run cmd: %1 failed: %2").arg(cmd).arg(out);
        }
    } else {
        qWarning() << "failed file open. " << info;
    }
}

bool PasswordManager::checked(const QString &user, const QString &passwd, QString &err)
{
    return checked(user, passwd, LowerLevel, err);
}

bool PasswordManager::checked(const QString &user, const QString &passwd, int level, QString &err)
{
    bool ret = true;

    if (passwdLevel(user, passwd) < level) {
        err = ::QObject::tr("The password must have at least 8 characters"
                            ", and contain at least 3 of the four available character types: lowercase letters"
                            ", uppercase letters, numbers, and symbols");
        return false;
    }

    PW_ERROR_TYPE type = deepin_pw_check(user.toStdString().c_str(), passwd.toStdString().c_str(), level, nullptr);
    qDebug() << "PasswordManager password checked error type:" << err_to_string(type);
    if (type == PW_NO_ERR && user == passwd) {
        type = (PW_ERROR_TYPE)PW_USER_PASSWD_DUPLICATE;
    }
    switch (int(type)) {
        case PW_NO_ERR:break;

        case PW_ERR_PASSWORD_EMPTY: {
            err = ::QObject::tr("The password cannot be empty");
            ret = false;
        } break;

        case PW_ERR_LENGTH_SHORT:
        case PW_ERR_LENGTH_LONG: {
            err = ::QObject::tr("Password must be between %1 and %2 characters")
                    .arg(m_passwdMinLength)
                    .arg(m_passwdMaxLength);
            ret = false;
        } break;

        case PW_CHARACTER_TYPE_TOO_FEW:
        case PW_ERR_CHARACTER_INVALID:{
            // 优化正则表达式，正确处理包含双引号和反斜杠的特殊字符
            QString symbols = QString(m_passwdValidatePolicy).replace(QRegularExpression("[0-9a-zA-Z;]"), "");
            err = ::QObject::tr("The password should contain at least %1 of the four available character types: lowercase letters, uppercase letters, numbers, and symbols (%2)")
                    .arg(m_passwdValidateRequired).arg(symbols);
            ret = false;
        } break;

        case PW_ERR_PALINDROME: {
            err = ::QObject::tr("Password must not contain more than %1 palindrome characters").arg(m_passwdPalindromeLength);
            ret = false;
        } break;

        case PW_ERR_WORD: {
            err = ::QObject::tr("Do not use common words and combinations as password");
            ret = false;
        } break;

        case PW_ERR_PW_MONOTONE:
        case PW_ERR_PW_CONSECUTIVE_SAME: {
            err = ::QObject::tr("No more than %1 consecutive or repeated characters please").arg(m_passwdContinuousLength);
            ret = false;
        } break;
        case PW_USER_PASSWD_DUPLICATE: {
            err = ::QObject::tr("Different from the username");
            ret = false;
        } break;

        case PW_ERR_PW_REPEAT:
        case PW_ERR_PW_FIRST_UPPERM:
        case PW_ERR_PARA:
        case PW_ERR_INTERNAL:
        case PW_ERR_USER:
        default: {
            err = ::QObject::tr("It does not meet password rules");
            ret = false;
        } break;
    }

    return ret;
}

QPair<QString, QColor> PasswordManager::passwdLevel(PasswdLevel level)
{
    QMap<PasswdLevel, QPair<QString, QColor>> Passwd_Level_Map = {
        {PasswdLevel::LowerLevel, QPair<QString, QColor>(QObject::tr("Weak"), QColor("#ff5736"))},       // 红色
        {PasswdLevel::MediumLevel, QPair<QString, QColor>(QObject::tr("Medium"), QColor("#f7a11b"))},    // 橙色
        {PasswdLevel::HigherLevel, QPair<QString, QColor>(QObject::tr("Strong"), QColor("#3ec500"))}   // 绿色
    };

    return Passwd_Level_Map.value(level);
}

PasswdLevel PasswordManager::passwdLevel(const QString &user, const QString &passwd)
{
    PasswdLevel level = PasswdLevel::LowerLevel;

    // 依次获取密码等级中的高和中，剩余情况为低
    if (passwdStrength(passwd) >= 3 && passwd.size() >= 8 && (user != passwd)) {
        level = PasswdLevel::HigherLevel;
    }  else if (passwdStrength(passwd) >= 2 && passwd.size() >= 6) {
        level = PasswdLevel::MediumLevel;
    }

    return level;
}

int PasswordManager::passwdStrength(const QString &passwd)
{
    QStringList validatePolicyList = m_passwdValidatePolicy.split(";");

    int ret = int(std::count_if(validatePolicyList.cbegin(), validatePolicyList.cend(),
                      [=](const QString& policy) {
                          for (const QChar& c : policy) {
                              if (passwd.contains(c)) {
                                  return true;
                              }
                          }
                          return false;
                      }));

    return ret;
}

