//
// Created by uos on 2024/5/7.
//

#ifndef DEEPIN_INSTALLER_SCREENMONITOR_H
#define DEEPIN_INSTALLER_SCREENMONITOR_H
#include <QThread>
#include <QMap>



struct ScreenInfo{
    QString optimalResolution;//最优分辨率
    QString currentResolution;//当前分辨率
    QStringList resolutionList;//分辨率列表
    bool isPrimary;//是否是主屏
    bool operator==(const ScreenInfo &other) const {
        return optimalResolution == other.optimalResolution &&
               currentResolution == other.currentResolution &&
               resolutionList == other.resolutionList &&
               isPrimary == other.isPrimary;
    }
};

class ScreenMonitor : public QThread
{
    Q_OBJECT
public:
    explicit ScreenMonitor(QObject *parent = nullptr);
    void stop();
    void run() override;
Q_SIGNALS:
    void screenReady();
private:
    QMap<QString, ScreenInfo> getScreenInfoByXrandr();
    bool setResolution(const QString &screenName, const QString &resolution);
    QString findSameResolution(QMap<QString, ScreenInfo> screens);
    bool setDuplicateScreen(const QString &primaryScreenName, const QString &slaveScreenName);
private:
    QMap<QString, ScreenInfo> m_screens;
    bool m_stop = false;
};


#endif //DEEPIN_INSTALLER_SCREENMONITOR_H
