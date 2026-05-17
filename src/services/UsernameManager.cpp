#include "UsernameManager.h"
#include "CommonFunc.h"

#include <QDebug>
#include <QRegularExpression>

UsernameManager *UsernameManager::instance()
{
    static UsernameManager um;
    return &um;
}

UsernameManager::UsernameManager()
{

}

//判断创建的用户名是否是系统上已有的用户名或者组名
bool UsernameManager::IsReservedUsername(const QString& username)
{
    if(m_reserveList.isEmpty()) {
        QString pwdPath = "/etc/passwd";
        QString groupPath = "/etc/group";
        QString result;
        QString log;

        if (!QFileInfo::exists(pwdPath) || !QFileInfo::exists(groupPath)) {
            return false;
        }

        QString cmd = "cut -d: -f1 ";
        if (!CommonFunc::command("/bin/bash", {"-c", cmd + pwdPath}, log)) {
            qWarning() << "cut /etc/passwd is failed:" << log;
            return false;
        }

        result.append(log);
        log.clear();
        if (!CommonFunc::command("/bin/bash", {"-c", cmd + groupPath}, log)) {
            qWarning() << "cut /etc/group is failed:" << log;
            return false;
        }
        result.append(log);
        m_reserveList = CommonFunc::qstringSplitToQStringList(result, "\n");
    }

    if (!CommonFunc::isFirstBoot() && (username == CommonFunc::getSettingsString("LIVE_USERNAME"))) {
        return false;
    }

    for (const QString& line : m_reserveList) {
        if (line == username) {
            return true;
        }
    }

    return false;
}

bool UsernameManager::checked(const QString& username, QString& err)
{
    const int minLen = CommonFunc::getSettingsInt("DI_USERNAME_MIN_LEN_CONFIG");
    const int maxLen = CommonFunc::getSettingsInt("DI_USERNAME_MAX_LEN_CONFIG");

    err = ::QObject::tr("Username must start with letters or numbers, only contain letters"
                        ", numbers, dashes (-) and underscores (_), and cannot use numbers alone");
    if (username.isEmpty() && (minLen > 0)) {
        return false;
    }

    // 创建的用户名是否符合命名规则，用户名以字母或数字开头，只允许使用字母、数字、连接符（-）和下划线（_），且不可以使用纯数字
    
    if (!QRegularExpression("^[a-zA-Z0-9][a-zA-Z0-9_-]*$").match(username).hasMatch()) {
        return false;
    }

    // TODO(chenxiong): 之后采用上面的exactMatch来写自然一点
    // 创建的用户名是否为纯数字
    // 错误信息同上
    if (QString(username).replace(QRegularExpression("[0-9]"), "").isEmpty()) {
        return false;
    }

    err = ::QObject::tr("Username must be between %1 and %2 characters")
            .arg(minLen)
            .arg(maxLen);
    if (username.length() < minLen) {
        return false;
    }

    // 错误信息同上
    if (username.length() > maxLen) {
        return false;
    }

    //创建的用户名是否为Linux系统已有的用户名或组名
    if (IsReservedUsername(username)) {
        err = ::QObject::tr("This username already exists");
        return false;
    }

    err.clear();
    return true;
}
