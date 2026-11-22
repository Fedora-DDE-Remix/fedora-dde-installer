#include "ComponentsManager.h"
#include "CommonFunc.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDir>

ComponentsManager::ComponentsManager(QObject *parent) : QObject(parent)
{

}


bool ComponentsManager::initData()
{
    QString err;
    bool readjsonok = false;
    QString defaultpath = "", choicepath = "", sortpath = "";

    GetComponentFilePath(defaultpath, choicepath, sortpath);

    // 读取packages_default.json
    QJsonDocument packagesdefault;
    readjsonok = CommonFunc::readJsonFile(defaultpath, packagesdefault, err);
    if (!readjsonok) {
        return false;
    }
    QJsonObject defaultobj = packagesdefault.object();

    // 读取packages_choice.json
    QJsonDocument packageschoices;
    readjsonok = CommonFunc::readJsonFile(choicepath, packageschoices, err);
    if (!readjsonok) {
        return false;
    }
    QJsonObject choicesobj = packageschoices.object();

    // 读取packages_sort.json
    QJsonDocument packagessort;
    readjsonok = CommonFunc::readJsonFile(sortpath, packagessort, err);
    if (!readjsonok) {
        return false;
    }
    QJsonArray sortarray = packagessort.array();


    for (int i = 0; i < sortarray.size(); i++) {
        QJsonObject sortobj = sortarray.at(i).toObject();
        QStringList keys = sortobj.keys();
        for (int j = 0; j < keys.size(); j++) {
            SPackage packages;
            packages.name = keys.at(j);
            iniPackagePage(choicesobj, sortobj.value(keys.at(j)).toArray(), packages.choices);
            iniPackagePage(choicesobj, defaultobj.value(keys.at(j)).toObject().value("default").toArray(), packages.defaults);
            iniPackagePage(choicesobj, defaultobj.value(keys.at(j)).toObject().value("uninstall").toArray(), packages.uninstalls);
            iniPackagePage(choicesobj, defaultobj.value(keys.at(j)).toObject().value("choice-uninstall").toArray(), packages.choiceuninstalls);
            m_packageList.append(packages);
        }
    }

    return true;
}

bool ComponentsManager::saveData(const QString &name, const QStringList &selects)
{
    QStringList componentstoinstall = getInstallPackages(name, selects);
    QStringList componentstouninstall = getUnInstallPackages(name);

    CommonFunc::setSettingsValue("DI_COMPONENT_PACKAGES", QString("%1").arg(componentstoinstall.join(" ")));
    CommonFunc::setSettingsValue("DI_COMPONENT_UNINSTALL", QString("%1").arg(componentstouninstall.join(" ")));

    return true;
}

QStringList ComponentsManager::getNames()
{
    QStringList names = {};
    for (int i = 0; i < m_packageList.size(); i++) {
        names.append(m_packageList.at(i).name);
    }
    return names;
}

QStringList ComponentsManager::getPages(const QString &name)
{
    QStringList pages = {};
    for (int i = 0; i < m_packageList.size(); i++) {
        if ( !m_packageList.at(i).name.compare(name) ) {
            for (int j = 0; j < m_packageList.at(i).choices.size(); j++) {
                pages.append(m_packageList.at(i).choices.at(j).page);
            }
        }
    }
    return pages;
}

QStringList ComponentsManager::getInstallPackages(const QString &name, const QStringList &pages)
{
    QStringList reultpages = {};
    for (int i = 0; i < m_packageList.size(); i++) {
        if ( !m_packageList.at(i).name.compare(name) ) {
            for (int j = 0; j < m_packageList.at(i).choices.size(); j++) {
                if (pages.contains(m_packageList.at(i).choices.at(j).page)) {
                    reultpages.append(m_packageList.at(i).choices.at(j).package);
                }
            }
        }
    }

    // TODO: 检查并去掉不存在的包

    return reultpages;
}

QStringList ComponentsManager::getUnInstallPackages(const QString &name)
{
    QStringList pages = {};
    for (int i = 0; i < m_packageList.size(); i++) {
        if ( !m_packageList.at(i).name.compare(name) ) {
            for (int j = 0; j < m_packageList.at(i).uninstalls.size(); j++) {
                pages.append(m_packageList.at(i).uninstalls.at(j).package);
            }
        }
    }
    return pages;
}

void ComponentsManager::iniPackagePage(const QJsonObject &choicesobj, const QJsonArray& info, QList<SPackagePage> &packagepages)
{
    for (int k = 0; k < info.size(); k++) {
        QJsonArray choicesarray = choicesobj.value(info.at(k).toString()).toArray();
        SPackagePage packagepagechoices;
        packagepagechoices.page = info.at(k).toString();
        for (int h = 0; h < choicesarray.size(); h++) {
            packagepagechoices.package.append(choicesarray.at(h).toString());
        }
        packagepages.append(packagepagechoices);
    }
}

void ComponentsManager::GetComponentFilePath(QString &defaultpath, QString &choicepath, QString &sortpath)
{
    QStringList filepaths = {};
    QStringList paths{ "/var/run/live/medium/live/", CommonFunc::getSystemEnv(DI_CONFIG_DIR_ENV) + "/settings/" };
    QString tmppath = "";
    for (const QString& path : paths) {
        tmppath = path + "packages_default.json";
        if (QFile::exists(tmppath)) {
            defaultpath = tmppath;
            break;
        }
    }
    for (const QString& path : paths) {
        tmppath = path + "packages_choice.json";
        if (QFile::exists(tmppath)) {
            choicepath = tmppath;
            break;
        }
    }
    for (const QString& path : paths) {
        tmppath = path + "packages_sort.json";
        if (QFile::exists(tmppath)) {
            sortpath = tmppath;
            break;
        }
    }
}


QPair<QString, QString> ComponentsManager::getTSText(const QString &id)
{
    return QMap<QString, QPair<QString, QString>> {
        {"core", {::QObject::tr("Core"), ::QObject::tr("Core packages for server OS.")}},
        {"base", {::QObject::tr("Base"), ::QObject::tr("Base packages for server OS.")}},
        {"Basic-Server-Environment", {::QObject::tr("Basic Server Environment"), ::QObject::tr("Applies to standard common service environments, such as Web services and database services.")}},
        {"Cloud-and-Virtualzalition", {::QObject::tr("Cloud and Virtualization"), ::QObject::tr("Applies to IaaS environments, such as virtualization, containers, and clouds.")}},
        {"Big-Data", {::QObject::tr("Big Data"), ::QObject::tr("Applies to servers that provide parallel processing of massive data.")}},
        {"Server-Environment-with-GUI", {::QObject::tr("Server Environment with GUI"), ::QObject::tr("Operates the services and applications on the graphical user interface.")}},
        {"Custom-Installation", {::QObject::tr("Custom Installation"), ::QObject::tr("Provides most package groups for custom selection, covering minimum to maximum installation.")}},
        {"debugging-tools", {::QObject::tr("Debugging Tools"), ::QObject::tr("Provide debugging package for program function and performance.")}},
        {"deepin", {::QObject::tr("Applications with GUI"), ::QObject::tr("Commonly used server application packages in graphical user interface.")}},
        {"development-tools", {::QObject::tr("Development Tools"), ::QObject::tr("Development tool packages in multiple programming languages.")}},
        {"directory-client", {::QObject::tr("Directory Client"), ::QObject::tr("Clients for integration into a network managed by a directory service.")}},
        {"dns-server", {::QObject::tr("Domain Name Service"), ::QObject::tr("Environments for running the domain name service (DNS).")}},
        {"file-server", {::QObject::tr("File and Storage Server"), ::QObject::tr("CIFS, SMB, NFS, iSCSI, iSER, and iSNS network storage server.")}},
        {"graphics", {::QObject::tr("Graphics Creation Tools"), ::QObject::tr("Software for creating and editing images.")}},
        {"load-balancer", {::QObject::tr("Load Balancer"), ::QObject::tr("Load balancing support for network traffic.")}},
        {"mariadb-server", {::QObject::tr("MariaDB Database Server"), ::QObject::tr("The MariaDB SQL database server, and associated packages.")}},
        {"mail-server", {::QObject::tr("E-mail Server"), ::QObject::tr("Allows the system to act as a SMTP and/or IMAP e-mail server.")}},
        {"network-file-system-client", {::QObject::tr("Network File System Client"), ::QObject::tr("Environments for network storage devices.")}},
        {"performance-tools", {::QObject::tr("Performance Tools"), ::QObject::tr("Packages for monitoring and diagnosing hardware devices, system programs, and applications.")}},
        {"security-tools", {::QObject::tr("Security Tools"), ::QObject::tr("Packages for integrity and trust verification.")}},
        {"web-server", {::QObject::tr("Web Server"), ::QObject::tr("Provides Java servlets and Web common service components.")}},
        {"guest-agents", {::QObject::tr("Guest Agents"), ::QObject::tr("Agents used when running under a hypervisor.")}},
        {"virtualization-client", {::QObject::tr("Virtualization Client"), ::QObject::tr("Clients for installing and managing virtualization instances.")}},
        {"virtualization-hypervisor", {::QObject::tr("Virtualization Hypervisor"), ::QObject::tr("Smallest possible virtualization host installation.")}},
        {"virtualization-tools", {::QObject::tr("Virtualization Tools"), ::QObject::tr("Tools for offline virtual image management.")}},
        {"virtualization-platform", {::QObject::tr("Virtualization Platform"), ::QObject::tr("Provides an interface for acessing and controlling virtualized guests and containers.")}},
        {"ha", {::QObject::tr("High Availability"), ::QObject::tr("Infrastructure for highly available services and/or shared storage.")}},
        {"infiniband", {::QObject::tr("InfiniBand Support"), ::QObject::tr("Software designed for supporting clustering and grid connectivity using RDMA-based InfiniBand and iWARP fabrics.")}},
        {"large-systems", {::QObject::tr("Large Systems Performance"), ::QObject::tr("Performance support tools for large systems.")}},
        {"platform-devel", {::QObject::tr("Platform Development"), ::QObject::tr("Recommended development headers and libraries for developing applications to run on Fedora.")}},
        {"internet-applications", {::QObject::tr("Internet Applications"), ::QObject::tr("Email, chat, and video conferencing software.")}},
        {"virtualization-tool-set", {::QObject::tr("Virtualization Toolset"), ::QObject::tr("Toolset for virtualization enviroment (host and guest).")}},
        {"dde", {::QObject::tr("Server Environment with GUI"), ::QObject::tr("Operates the services and applications on the graphical user interface.")}},
        {"postgresql-server", {::QObject::tr("PostgreSQL Database Server"), ::QObject::tr("The PostgreSQL SQL database server, and associated packages.")}},
    }[id];
}
