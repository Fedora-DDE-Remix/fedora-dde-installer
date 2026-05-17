//
// Created by uos on 2024/5/7.
//

#include "ScreenMonitor.h"
#include "CommonFunc.h"
#include <QGuiApplication>
#include <QDebug>
#include <QRegularExpression>
#include <QScreen>




ScreenMonitor::ScreenMonitor(QObject *parent) : QThread(parent)
{

}

void ScreenMonitor::stop()
{
    m_stop = true;
}

void ScreenMonitor::run()
{
    bool canChangeResolution = CommonFunc::getSettingsString("DI_DESKTOP_ENV") == "x11";
    qInfo() << "canChangeResolution=" << canChangeResolution;
    qInfo() << "scaled=" << (qAbs(qApp->devicePixelRatio() - 1.0) > 0.00001);

    while (!m_stop) {
        auto defaultResolution = CommonFunc::getSettingsStringList("DI_DEFAULT_RESOLUTION");
        QMap<QString, ScreenInfo> currScreens = getScreenInfoByXrandr();
        QString lastAvailableSize = QString("%1x%2").arg(qApp->primaryScreen()->availableSize().width()).arg(qApp->primaryScreen()->availableSize().height());
        bool isAvailableSizeOk = lastAvailableSize == "1920x1080";
        
#ifndef QT_DEBUG
        if (currScreens.size() == 1) { //只有一个显示器
            auto it = currScreens.begin();
            if (!CommonFunc::isVirtualMachine()) {
                QString output;
                if (!canChangeResolution) {  //华为设备由于x11环境下驱动问题，4K显示器设置1K分辨率时存在概率性无法显示画面
                    if (it.value().optimalResolution.isEmpty() && !it.value().resolutionList.isEmpty()) { //华为pangux设备连接部分显示器时获取不到最优分辨率，此时设置最大分辨率
                        if (!CommonFunc::command("xrandr", QStringList() << "--output" << currScreens.firstKey() << "--primary" << "--mode" << it.value().resolutionList.first(), output)) {
                            qWarning() << output;
                        }
                    } else {
                        if (!CommonFunc::command("xrandr", QStringList() << "--output" << currScreens.firstKey() << "--primary" << "--auto", output)) {
                            qWarning() << output;
                        }
                    }
                } else if (it.value().resolutionList.contains("1920x1080") && it.value().currentResolution != "1920x1080") {
                    qInfo() << "Set resolution: " << it.value().currentResolution << " to 1920x1080";
                    if (setResolution(it.key(), "1920x1080")) {
                        qInfo() << "Set resolution: " << it.value().currentResolution << " to 1920x1080 success";
                    } else {
                        qWarning() << "Set resolution: " << it.value().currentResolution << " to 1920x1080 failed!";
                    }
                } else if (!isAvailableSizeOk) {
                    if (setResolution(it.key(), it.value().optimalResolution)) {
                        qInfo() << "Set resolution: " << it.value().currentResolution << " to " << it.value().optimalResolution << " success";
                    } else {
                        qWarning() << "Set resolution: " << it.value().currentResolution << " to " << it.value().optimalResolution << " failed!";
                    }
                }
            } else if (!defaultResolution.contains(it.value().currentResolution)) { //设置虚拟机分辨率
                for (const auto &resolution : defaultResolution) {
                    if (it.value().resolutionList.contains(resolution) && it.value().currentResolution != resolution) {
                        qInfo() << "Set resolution:" << resolution << "for screen:" << it.key();
                        if (setResolution(it.key(), resolution)) {
                            break;
                        } else {
                            qWarning() << "Set resolution:" << resolution << "for screen:" << it.key() << "failed!";
                        }
                    }
                }
            }
        } else if (currScreens.size() >= 2) { //多屏
            QString primaryScreenName;
            QString slaveScreenName;
            bool noCurrResolution = false;
            for (auto it = currScreens.begin(); it != currScreens.end(); it++) {
                if (it.value().isPrimary) {
                    primaryScreenName = it.key();
                } else {
                    slaveScreenName = it.key();
                }

                if (it.value().currentResolution.isEmpty()) {
                    qWarning() << it.key() << "resolution is empty, set default resolution";
                    if (it.value().optimalResolution.isEmpty() && !it.value().resolutionList.isEmpty()) {
                        setResolution(it.key(), it.value().resolutionList.first());
                    } else {
                        QStringList args;
                        args << "--output" << it.key()  << "--auto";
                        QString err;
                        if (!CommonFunc::command("xrandr", args, err)) {
                            qWarning() << "Failed to set primary screen: " << err;
                            noCurrResolution = true;
                        }
                    }
                }
            }

            if (noCurrResolution) {
                continue;
            }

            if (primaryScreenName.isEmpty()) {
                qInfo() << "No primary screen, set primary screen to " << currScreens.firstKey();
                QStringList args;
                args << "--output" << currScreens.firstKey()  << "--primary";
                QString err;
                if (!CommonFunc::command("xrandr", args, err)) {
                    qWarning() << "Failed to set primary screen: " << err;
                }
                continue;
            } else {
                //qInfo() << "primaryScreenName:" << primaryScreenName << "  currentResolution:" << currScreens[primaryScreenName].currentResolution;
                //qInfo() << "slaveScreenName:" << slaveScreenName << "  currentResolution:" << currScreens[slaveScreenName].currentResolution;
                if (!canChangeResolution) {
                    if (currScreens[primaryScreenName].currentResolution != currScreens[primaryScreenName].optimalResolution) {
                        if (currScreens[primaryScreenName].optimalResolution.isEmpty() && !currScreens[primaryScreenName].resolutionList.isEmpty()) {
                            setResolution(primaryScreenName, currScreens[primaryScreenName].resolutionList.first());
                        } else {
                            QString output;
                            if (!CommonFunc::command("xrandr", QStringList() << "--output" << primaryScreenName << "--auto", output)) {
                                qWarning() << output;
                            }
                        }
                    }

                    if (currScreens[slaveScreenName].currentResolution != currScreens[slaveScreenName].optimalResolution) {
                        if (currScreens[slaveScreenName].optimalResolution.isEmpty() && !currScreens[slaveScreenName].resolutionList.isEmpty()) { //华为pangux设备连接部分显示器时获取不到最优分辨率，此时设置最大分辨率
                            setResolution(slaveScreenName, currScreens[primaryScreenName].resolutionList.first());
                        } else {
                            QString output;
                            if (!CommonFunc::command("xrandr", QStringList() << "--output" << slaveScreenName << "--auto", output)) {
                                qWarning() << output;
                            }
                        }
                    }


                    if (currScreens[primaryScreenName].currentResolution == currScreens[slaveScreenName].currentResolution) {
                        //qInfo() << "setDuplicateScreen";
                        setDuplicateScreen(primaryScreenName, slaveScreenName);
                    }
                } else {
                    //qInfo() << "setSameResolution";
                    auto sameResolution = findSameResolution(currScreens);
                    if (currScreens[primaryScreenName].currentResolution != sameResolution) {
                        setResolution(primaryScreenName, sameResolution);
                    }
                    if (currScreens[slaveScreenName].currentResolution != sameResolution){
                        setResolution(slaveScreenName, sameResolution);
                    }
                    setDuplicateScreen(primaryScreenName, slaveScreenName);
                }
            }
        }
#endif
        if (m_screens != currScreens) {
            m_screens = currScreens;
// #ifndef QT_DEBUG
//             QString output;
//             // 华为设备部分显示器需要来回切换tty才能正常显示
//             // 如果是桌面环境就不需要切换tty
//             if (!CommonFunc::isInDesktop()) {
//                 CommonFunc::command("chvt", QStringList() << "2", output);
//                 CommonFunc::command("chvt", QStringList() << "1", output);
//             }
// #endif
        }

        if (CommonFunc::isVirtualMachine()) {
            if (!m_screens.isEmpty() && defaultResolution.contains(m_screens.first().currentResolution)) {
                Q_EMIT screenReady();
            }
        } else if (!m_screens.isEmpty()) {
            if (isAvailableSizeOk) {
                Q_EMIT screenReady();
            }
        }
        msleep(1000);
    }
}

QMap<QString, ScreenInfo> ScreenMonitor::getScreenInfoByXrandr()
{
    QRegularExpression regExp("(\\d+)x(\\d+)");
    QMap<QString, ScreenInfo> currScreens;
    QString output;
    CommonFunc::command("/bin/bash", QStringList() << "-c" << "xrandr", output);
    auto content = output.split("\n");
    bool flag = false;
    for (auto it = content.begin(); it != content.end(); it++) {
        QString line = *it;
        ScreenInfo screenInfo;
        static QString screenName;
        if (line.contains(" connected")) {
            if (line.split(" ").size() > 0) {
                screenName = line.split(" ").at(0);
            }

            if (line.contains(" connected primary") && !line.contains("disconnected")) {
                screenInfo.isPrimary = true;
            } else {
                screenInfo.isPrimary = false;
            }

            flag = true;
        }


        if (flag) {
            auto find = currScreens.find(screenName);
            if (find != currScreens.end()) {
                screenInfo = find.value();
            }

            if (regExp.match(line).hasMatch() && !line.contains(" connected")) {
                screenInfo.resolutionList.append(regExp.match(line).captured(0));
                if (line.contains("+")) {
                    screenInfo.optimalResolution = regExp.match(line).captured(0);
                }
                if (line.contains("*")) {
                    screenInfo.currentResolution = regExp.match(line).captured(0);
                }
                currScreens.insert(screenName, screenInfo);
            } else if (!regExp.match(line).hasMatch() && !line.contains(" connected")) {
                flag = false;
            }
        } else {
            screenName.clear();
        }
    }
    return currScreens;
}

bool ScreenMonitor::setResolution(const QString &screenName, const QString &resolution)
{
    QStringList args;
    args << "--output" << screenName << "--mode" << resolution;
    QString err;
    if (!CommonFunc::command("xrandr", args, err)) {
        qWarning() << err;
        return false;
    }
    return true;
}

QString ScreenMonitor::findSameResolution(QMap<QString, ScreenInfo> screens)
{
    QString sameResolution;
    QStringList commonResolutions;
    for (auto it = screens.begin(); it != screens.end(); ++it) {
        for (const QString& resolution : it.value().resolutionList) {
            commonResolutions.append(resolution);
        }
    }

    for (QString resolution : commonResolutions) {
        int count = 0;
        for (auto it = screens.begin(); it != screens.end(); ++it) {
            if (it.value().resolutionList.contains(resolution)) {
                count++;
            }
        }
        if (count >= screens.size()) {
            sameResolution =  resolution;
            break;
        }
    }
    return sameResolution;
}

bool ScreenMonitor::setDuplicateScreen(const QString &primaryScreenName, const QString &slaveScreenName)
{
    QStringList args;
    args << "--output" << slaveScreenName << "--same-as" << primaryScreenName;
    QString err;
    if (!CommonFunc::command("xrandr", args, err)) {
        qWarning() << "Failed to set slave screen: " << err;
        return false;
    }
    return true;
}
