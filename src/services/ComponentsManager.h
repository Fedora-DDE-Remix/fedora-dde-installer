#ifndef COMPONENTSMANAGER_H
#define COMPONENTSMANAGER_H

#include <structs.h>

#include <QObject>

class ComponentsManager : public QObject
{
    Q_OBJECT
public:
    explicit ComponentsManager(QObject *parent = nullptr);
    bool initData();
    bool saveData(const QString &name, const QStringList &selects);
    QStringList getNames();
    QStringList getPages(const QString &name);
    QStringList getInstallPackages(const QString &name, const QStringList &pages);
    QStringList getUnInstallPackages(const QString &name);
    QPair<QString, QString> getTSText(const QString &id);

signals:

public slots:

private:
    QList<SPackage> m_packageList;

private:
    void GetComponentFilePath(QString &id, QString &choicepath, QString &sortpath);
    void iniPackagePage(const QJsonObject &choicesobj, const QJsonArray& info, QList<SPackagePage> &packagepages);

};

#endif // COMPONENTSMANAGER_H
