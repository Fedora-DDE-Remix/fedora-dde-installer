#ifndef HOSTNAMEMANAGER_H
#define HOSTNAMEMANAGER_H

#include <QString>

class HostnameManager
{
public:
    static HostnameManager* instance();

    // Pass in the host name of the detection and return whether the data is verified.
    // If the value is false, use the err parameter to obtain specific error information.

    // Check hostname (machine name) based on the following rules:
    //   * Only letters(a-z, A-Z) and numbers(0-9) can be used as prefix/suffix;
    //   * Dot(.) is used to separate domain and sub-domain;
    //   * Underscore(_) and dash(-) are used to concat letters and numbers.
    // Parameter list:
    //  * |hostname|, hostname to be validated.
    //  * |reserved|, a list of hostname reserved by system, normally it only
    //    contains "localhost".
    // For more information about hostname, please visit:
    //  * https://en.wikipedia.org/wiki/Hostname
    //  * https://tools.ietf.org/html/rfc1123#page-13
    bool checked(const QString& hostname, QString& err);

private:
    HostnameManager();
    HostnameManager(const HostnameManager&) = delete;
};

#endif // HOSTNAMEMANAGER_H
