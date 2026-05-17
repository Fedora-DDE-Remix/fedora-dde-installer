
#ifndef DEEPIN_INSTALLER_ISOVERIFY_H
#define DEEPIN_INSTALLER_ISOVERIFY_H

#include <QThread>
class ISOVerify : public QThread
{
    Q_OBJECT
public:
    explicit ISOVerify(QObject* parent = nullptr);
    void run() override;
Q_SIGNALS:
    void verified(bool);

private:
    bool verifyHandle(const QString &sourceFilePath,
                      const QString& verifyFilePath, QString &err);
};


#endif //DEEPIN_INSTALLER_ISOVERIFY_H
