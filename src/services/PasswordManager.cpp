#include "PasswordManager.h"
#include "CommonFunc.h"

#include <algorithm>

PasswordManager *PasswordManager::instance()
{
    static PasswordManager manager;
    return &manager;
}

PasswordManager::PasswordManager()
{
    m_passwdMinLength = CommonFunc::getSettingsString("DI_PASSWORD_MIN_LEN_CONFIG");
    m_passwdMaxLength = CommonFunc::getSettingsString("DI_PASSWORD_MAX_LEN_CONFIG");
    m_passwdValidatePolicy = CommonFunc::getSettingsString("DI_PASSWORD_VALIDATE_CONFIG");
    m_passwdValidateRequired = CommonFunc::getSettingsString("DI_PASSWORD_VALIDATE_REQUIRED_CONFIG");
}

bool PasswordManager::checked(const QString &user, const QString &password, QString &error)
{
    return checked(user, password, LowerLevel, error);
}

bool PasswordManager::checked(const QString &user, const QString &password, int level, QString &error)
{
    error.clear();
    const int minimum = qMax(8, m_passwdMinLength.toInt());
    const int maximum = qMax(minimum, m_passwdMaxLength.toInt());
    if (password.size() < minimum || password.size() > maximum) {
        error = QObject::tr("Password must be between %1 and %2 characters").arg(minimum).arg(maximum);
        return false;
    }
    if (std::any_of(password.cbegin(), password.cend(), [](QChar character) { return character.isNull() || character.category() == QChar::Other_Control; })) {
        error = QObject::tr("The password must not contain control characters.");
        return false;
    }
    if (!user.isEmpty() && user.compare(password, Qt::CaseInsensitive) == 0) {
        error = QObject::tr("The password must be different from the username.");
        return false;
    }
    const int types = qBound(1, m_passwdValidateRequired.toInt(), 4);
    if (passwdStrength(password) < types || passwdLevel(user, password) < level) {
        error = QObject::tr("Use a longer password with a mix of letters, numbers or symbols.");
        return false;
    }
    return true;
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

