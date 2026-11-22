#include "HostnameManager.h"
#include "CommonFunc.h"

#include <QStringList>
#include <QRegularExpression>
#include <QObject>

const int kHostnameMinLen = 1;
const int kHostnameMaxLen = 253;
const int kLabelMaxLen = 63; // 主机名中以.分割的各段称为label

HostnameManager *HostnameManager::instance()
{
    static HostnameManager hm;
    return &hm;
}

HostnameManager::HostnameManager()
{

}

bool HostnameManager::checked(const QString &hostname, QString &err)
{
    const QStringList reserved = CommonFunc::getSettingsStringList("DI_HOSTNAME_RESERVED_CONFIG");

    if (hostname.isEmpty()) {
        err = ::QObject::tr("Please input computer name");
        return false;
    }

    err = ::QObject::tr("Please input a computer name longer than %1 characters and "
                        "shorter than %2 characters")
                        .arg(kHostnameMinLen)
                        .arg(kHostnameMaxLen);
    if (hostname.length() < kHostnameMinLen) {
        return false;
    }

    // 错误信息同上
    if (hostname.length() > kHostnameMaxLen) {
        return false;
    }

    if (reserved.contains(hostname)) {
        err = ::QObject::tr("Computer name already exists, please input another one");
        return false;
    }

    const QStringList parts = hostname.split('.');
    if (parts.isEmpty()) {
        err = ::QObject::tr("Computer name can only contain letters, numbers, and dashes (-)"
                            ", and cannot start or end with a dash (-)");
        return false;
    }

    for (const QString& part : parts) {
        if (part.length() > kLabelMaxLen) {
            err = ::QObject::tr("Please input a computer name longer than %1 characters and "
                                "shorter than %2 characters")
                                .arg(kHostnameMinLen)
                                .arg(kLabelMaxLen);
            return false;
        }

        if (part.startsWith('-')
            || part.endsWith('-')
            || !QRegularExpression("^[a-z0-9\\-]{1,63}$", QRegularExpression::CaseInsensitiveOption).match(part).hasMatch()) {
            err = ::QObject::tr("Computer name can only contain letters, numbers, and dashes (-)"
                                ", and cannot start or end with a dash (-)");
            return false;
        }
    }

    err.clear();
    return true;
}
