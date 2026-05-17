#include "ProgressFrame.h"
#include "ProgressUpdateFrame.h"
#include "ProgressSuccessFrame.h"
#include "ProgressFailedFrame.h"
#include "globals.h"
#include "CommonFunc.h"
#include "HooksManager.h"
#include "PartitionOperateManager.h"
#include "PartitionAutoManager.h"

#include <QProgressBar>
#include <QStackedLayout>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QTimer>

ProgressFrame::ProgressFrame(QWidget *parent):
    FramePluginInterface(parent)
{
    m_frameName = gFrameInstall;
    hideBackButton();
    hideCloseButton();
    m_progressUpdateWidget = new ProgressUpdateFrame(nullptr);

    m_progressSuccessWidget = new ProgressSuccessFrame;
    m_progressFailedWidget = new ProgressFailedFrame;

    // 主布局
    m_mainLayout = new QStackedLayout;
    m_mainLayout->setSpacing(0);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->addWidget(m_progressUpdateWidget);
    m_mainLayout->addWidget(m_progressSuccessWidget);
    m_mainLayout->addWidget(m_progressFailedWidget);
    m_mainLayout->setCurrentWidget(m_progressUpdateWidget);
    m_contentLayout->addLayout(m_mainLayout);

    connect(HooksManager::instance(), &HooksManager::progress, [=](int value){
        m_progressUpdateWidget->progressWidget()->setValue(value);
        m_progressUpdateWidget->setProgressStatus(value == 100 ? CommonFunc::getInstallStatus(value) : HooksManager::instance()->getProgressStatus());
    });
    connect(HooksManager::instance(), &HooksManager::finished, this, &ProgressFrame::finished);
    connect(HooksManager::instance(), &HooksManager::exitInstall, this, &ProgressFrame::exitInstall);
}

void ProgressFrame::showEvent(QShowEvent *event)
{
    this->startInstall();
    FramePluginInterface::showEvent(event);
    Q_EMIT m_progressUpdateWidget->startAnime();
}

void ProgressFrame::updateTs()
{
    m_progressUpdateWidget->updateTS();
}

void ProgressFrame::startInstall()
{
    QString installErr;
    // 处理无人值守安装
    if (!PartitionAutoManager::instance()->start(installErr)) {
        // 显示错误日志界面
        m_progressFailedWidget->showLog(installErr);
        m_mainLayout->setCurrentWidget(m_progressFailedWidget);
        return;
    }

    // 将分区操作的配置写入到配置文件中
    if (!PartitionOperateManager::instance()->writeConf(installErr)) {
        // 显示错误日志界面
        m_progressFailedWidget->showLog(installErr);
        m_mainLayout->setCurrentWidget(m_progressFailedWidget);
        return;
    }

#ifdef QT_DEBUG
    QStringList testList = {gInstallDirTest + gInstallDirBeforeChroot, gInstallDirTest + gInstallDirInChroot, gInstallDirTest + gInstallDirAfterChroot};
    HooksManager::instance()->setWorkspaceDirs(testList);
#else
    HooksManager::instance()->setWorkspaceDirs(CommonFunc::getSettingsStringList("DI_HOOKS_LIST"));
#endif //QT_DEBUG

    HooksManager::instance()->startInstall();

    QString curTime = QDateTime::fromMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()).toString("yyyy-MM-dd hh:mm:ss");
    CommonFunc::setSettingsValue("DI_RECORD_INSTALL_START", curTime);
}

void ProgressFrame::finished()
{
    QString curTime = QDateTime::fromMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()).toString("yyyy-MM-dd hh:mm:ss");
    CommonFunc::setSettingsValue("DI_RECORD_INSTALL_STOP", curTime);

    qInfo() << "install finished.";

    // 显示安装成功界面
    m_mainLayout->setCurrentWidget(m_progressSuccessWidget);

    if (CommonFunc::getSettingsBool("DI_REBOOT_AFTER_SETUP")) {
        CommonFunc::rebootSystem();
    }
}

void ProgressFrame::exitInstall(const QString &err)
{
    QString curTime = QDateTime::fromMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()).toString("yyyy-MM-dd hh-mm-ss");
    CommonFunc::setSettingsValue("DI_RECORD_INSTALL_STOP", curTime);

    qCritical() << "exit install." << err;
    // 显示错误日志界面
    m_progressFailedWidget->showLogFile("/var/log/deepin-installer/deepin-installer.log");
    m_mainLayout->setCurrentWidget(m_progressFailedWidget);
}

void ProgressFrame::doRecord()
{
    bool isSelectExperience = CommonFunc::getSettingsBool("DI_USER_EXPERIENCE");
    bool isSelectPrivacyPolicy = CommonFunc::getSettingsBool("DI_PRIVACY_POLICY");
    bool isFullDiskEncrypt = CommonFunc::getSettingsString(
            "DI_CRYPT_INFO", "", "/target/etc/deepin-installer/deepin-installer.conf").isEmpty();

    // 写入数据埋点的配置到/etc/deepin/deepin-user-experience
    QSettings settings("/target/etc/deepin/deepin-user-experience", QSettings::IniFormat);
    if (settings.isWritable()) {
        settings.beginGroup("ExperiencePlan");
        settings.setValue("ExperienceState", isSelectExperience);
        settings.setValue("FullDiskEncrypt", !isFullDiskEncrypt);
        settings.setValue("LicenseState", isSelectPrivacyPolicy);
        settings.endGroup();
        settings.sync();
    }
    //todo 存在内容为空的情况，先sync处理。 可以使用QSaveFile优化
    CommonFunc::sync();

    if (!isSelectExperience) {
        return;
    }

    // 解析安裝类型
    int installType = -1;
    bool cryptSetup = false;
    bool saveData = false;
    bool lvmFullDisk = false;
    updateInstallType(installType, cryptSetup, saveData, lvmFullDisk);

    // 解析安装时长信息
    QString installTime = "";
    updateInstallTime(installTime);

    // 解析根分区、交换分区的大小信息
    qint64 rootSize = 0;
    qint64 swapSize = 0;
    updateSize(rootSize, swapSize);

    // 是否创建初始化备份
    bool initRecovery = CommonFunc::getSettingsBool("DI_IS_INIT_RECOVERY");

    // 根据分区配置文件获取安装的系统对应的磁盘的信息
    QString diskTypes = "";
    updateDiskType(diskTypes);

    QJsonObject installRecordObj;
    installRecordObj.insert("installType", installType);
    installRecordObj.insert("rootSize", rootSize);
    installRecordObj.insert("swapSize", swapSize);
    installRecordObj.insert("initRecovery", initRecovery);
    installRecordObj.insert("cryptSetup", cryptSetup);
    installRecordObj.insert("saveData", saveData);
    installRecordObj.insert("lvmFullDisk", lvmFullDisk);
    installRecordObj.insert("installTime", installTime);
    installRecordObj.insert("installerVersion", CommonFunc::getVersion().replace(" ", ""));
    installRecordObj.insert("CPU", CommonFunc::getCurrentPlatform());
    installRecordObj.insert("disk", diskTypes);

    // 写入数据埋点的配置到/etc/deepin/intaller_record.json
    QJsonDocument jsonDocOut(installRecordObj);
    QString installerRecordPath = "/target/etc/deepin/installer_record.json";
    QFile file(installerRecordPath);
    file.remove();
    if (!file.open(QIODevice::ReadWrite)) {
        qWarning() << installerRecordPath << " File open error : " << file.errorString();
        return;
    }
    file.write(jsonDocOut.toJson());
    file.close();
    //todo 存在内容为空的情况，先sync处理。 可以使用QSaveFile优化
    CommonFunc::sync();
}

void ProgressFrame::updateInstallType(int &installType, bool &cryptSetup, bool &saveData, bool &lvmFullDisk)
{
    // 解析安裝类型
    //全盘安装（0）、自定义安装（1）、高级安装（2）ghost安装（3）
    //FullDiskMode=0、CustomMode=6、AdvancedMode=1、GhostMode=5

    int installTypeInt = CommonFunc::getSettingsString("DI_PARTITION_TYPE").toInt();
    if (installTypeInt == FullDiskMode) {
        installType = 0;
    } else if (installTypeInt == AdvancedMode) {
        installType = 2;
    } else if (installTypeInt == Cryptsetup) {
        cryptSetup = true;
    } else if (installTypeInt == SaveData) {
        saveData = true;
    } else if (installTypeInt == GhostMode) {
        installType = 3;
    } else if (installTypeInt == CustomMode) {
        installType = 1;
    } else if (installTypeInt == LvmFullDisk) {
        lvmFullDisk = true;
    } else {
        installType = -1;
    }
}

void ProgressFrame::updateInstallTime(QString &installTime)
{
    QString startTimeStr = CommonFunc::getSettingsString("DI_RECORD_INSTALL_START");
    QString stopTimeStr = CommonFunc::getSettingsString("DI_RECORD_INSTALL_STOP");
    QDateTime startTime = QDateTime::fromString(startTimeStr, "yyyy-MM-dd_hh-mm-ss");
    QDateTime stopTime = QDateTime::fromString(stopTimeStr, "yyyy-MM-dd_hh-mm-ss");
    qint64 intervalTime2 = stopTime.toSecsSinceEpoch() - startTime.toSecsSinceEpoch();
    int hours = intervalTime2 / 3600;
    int minute = (intervalTime2 - hours * 3600) / 60;
    int second = (intervalTime2 - hours * 3600 - minute * 60);
    installTime = QString("%1:%2:%3")
            .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minute, 2, 10, QLatin1Char('0') )
            .arg(second, 2, 10, QLatin1Char('0'));;
    qInfo() << "install time use : " << installTime; //时间差
}

void ProgressFrame::updateSize(qint64 &rootSize, qint64 &swapSize)
{
    QString err = "";
    QJsonDocument jsonDoc;
    QString partPolicyFile = CommonFunc::getSettingsString("DI_PARTITION_CONFIG");
    if (CommonFunc::readJsonFile(partPolicyFile, jsonDoc, err)) {
        // 找到root和swap分区的大小信息
        QJsonArray partOperateList = jsonDoc.array();
        for (int i = 0; i < partOperateList.size(); i++) {
            QJsonObject partOperateObject = partOperateList.at(i).toObject();
            if (!partOperateObject.value("operate").toString().compare("new")) {
                if (!partOperateObject.value("mountPoint").toString().compare("/")) {
                    rootSize = partOperateObject.value("size").toVariant().toLongLong();
                }

                if (!partOperateObject.value("filesystem").toString().compare("linux-swap")) {
                    swapSize = partOperateObject.value("size").toVariant().toLongLong();
                }
            }
        }
    }
}

void ProgressFrame::updateDiskType(QString &diskType)
{
    QJsonDocument policyDoc;
    QString err = "";
    diskType = "";

    if (!CommonFunc::readJsonFile("/etc/deepin-installer/partition_policy.json", policyDoc, err)) {
        qWarning() << "/etc/deepin-installer/partition_policy.json File open error : " << err;
        return;
    }

    QStringList devsList;
    QJsonArray partitionsArray = policyDoc.array();
    for (int i = 0; i < partitionsArray.size(); i++) {
        QJsonObject partitionItem = partitionsArray.at(i).toObject();
        if (partitionItem.value("type").toString().compare("partition")) {
            continue;
        }

        QString devicePath = partitionItem.value("device").toString();
        if (devsList.indexOf(devicePath) == -1) {
            devsList.append(devicePath);
        }
    }

    qWarning() << "devsList list is : " << devsList;

    QStringList diskTypes;
    for (QString devName : devsList) {
        QString cmd = QString("lsblk -lpn -o NAME,TRAN | grep -w %1 | awk '{print $2}'").arg(devName);
        QString cmdLog = "";
        if (CommonFunc::command("/bin/bash", {"-c", cmd}, cmdLog)) {
            if (!cmdLog.isEmpty()) {
                cmdLog.replace(" ", "");
                cmdLog.replace("\n", "");
                diskTypes.append(cmdLog);
            }
        }
    }

    diskType = diskTypes.join(",");
}
