#ifndef USERNAMEMANAGER_H
#define USERNAMEMANAGER_H

#include <QString>
#include <QStringList>

class UsernameManager
{
public:
    static UsernameManager* instance();

    // 传入检测的用户名，返回数据是否校验通过，如果为false，通过err参数可以获得具体的错误信息。
    // 校验规则 1:用户名只能包含大小写字母、数字、连接符（-）和下划线（_）；
    //         2:用户名只能以字母或数字开头；
    //         3:用户名长度不能超过32个字符，不能小于3个字符；
    //         4:不能只包含数字；
    //         5:判断用户名是否是系统已存在的用户名或组名
    bool checked(const QString& username, QString& err);

private:
    UsernameManager();
    UsernameManager(const UsernameManager&) = delete;

    bool IsReservedUsername(const QString& username);
private:
    QStringList m_reserveList;
};

#endif // USERNAMEMANAGER_H
