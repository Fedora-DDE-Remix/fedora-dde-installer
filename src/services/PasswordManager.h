/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PASSWORDMANAGER_H
#define PASSWORDMANAGER_H

#include <QString>
#include <QColor>
#include <QPair>

enum PasswdLevel {
    LowerLevel = 1,
    MediumLevel,
    HigherLevel
};

class PasswordManager {
public:
    static PasswordManager* instance();

    // 传入检测的用户名和密码，返回数据是否校验通过，如果为false，通过err参数可以获得具体的错误信息。
    bool checked(const QString &user, const QString &passwd, QString &err);
    bool checked(const QString &user, const QString &passwd, int level, QString &err);

    QPair<QString, QColor> passwdLevel(PasswdLevel level);
    PasswdLevel passwdLevel(const QString &user, const QString &passwd);

private:
    PasswordManager();
    PasswordManager(const PasswordManager&) = delete;

    int passwdStrength(const QString &passwd);

private:
    QString m_passwdValidatePolicy;        // 密码策略配置
    QString m_passwdValidateRequired;      // 密码强度配置
    QString m_passwdMaxLength;             // 密码最大长度配置
    QString m_passwdMinLength;             // 密码最小长度配置
};

#endif  // PASSWORDMANAGER_H
