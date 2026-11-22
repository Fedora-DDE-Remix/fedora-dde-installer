#ifndef COMMONFUNC_H
#define COMMONFUNC_H

#include "structs.h"
#include "InstallWorkflow.h"

#include <QStringList>
#include <QString>
#include <QStack>
#include <QSize>
#include <QList>
#include <QHash>
#include <QMap>
#include <QPixmap>
#include <QLayout>
#include <QDir>
#include <QLabel>

class CommonFunc {
public:
    static InstallWorkflow &installWorkflow();

    // 读取环境变量的接口
    static QString getSystemEnv(const QString &key);

    // 目录文件操作接口
    static QMap<QString, QStringList> getDirs(const QStringList& dirs);
    static QStringList getDirs(const QString &dir, const QStringList &nameFilters = {});
    static QString readFile(const QString& path);
    static QString getInstallStatus(int status);

    // 图片处理函数
    static QPixmap getRoundRectPixmap(QPixmap srcPixMap, const QSize & size, int radius);
    const static QPixmap renderPixmap(const QString &path);
    static QPixmap loadSvgImage(const QString &strPath, int iWidth, int iHeight);
    static QPixmap renderSVG(const QString &path, const QSize &size);
    const static QPixmap hidpiPixmap(const QString &path, const QSize &size);

    // 配置处理接口
    static void initConfig(const QString &filePath);
    static QStack<SPluginInfo> getPlugins(const QString &jsonName);
    static QVariant getSettingsValue(const QString &key, const QString &section = QString(), const QString &filePath = QString());
    static QStringList getSettingsStringList(const QString &key, const QString &section = QString(), const QString &filePath = QString());
    static QString getSettingsString(const QString &key, const QString &section = QString(), const QString &filePath = QString());
    static int getSettingsInt(const QString &key, const QString &section = QString(), const QString &filePath = QString());
    static bool getSettingsBool(const QString &key, const QString &section = QString(), const QString &filePath = QString());
    static void setSettingsValue(const QString &key, const QVariant &value, const QString &section = QString(), const QString &filePath = QString());
    static QString getCurrentPlatform();
    static QString getCurrentOsType();
    static QString getCurrentOsVersion();
    static QString getVersion();  // 获取安装器版本
    static OSType getCurrentOs();
    static QString isNameDeepin();

    static bool getSettingsPolicy(QJsonDocument &policyDoc, QString &err);

    static LanguageList getLanguageList();
    static QString getSystemLogo();
    static QString getMainGifAnimations();      // 获取主界面的gif动画
    static QStringList getMainAnimations();     // 获取主界面的动画
    static QStringList getInstallAnimations();  // 获取安装界面动画
    static QStringList getConfigAnimations();   // 获取优化界面动画
    static QString getPartitionPolicy();
    static QString getUIDefaultFont();

    //oem是否定制了某项，entry->oem目录下settings.ini里面的条目
    static bool oemHasConfigEntry(const QString &entry);

    static QStringList getPartitionMountPoints(const SLVMInfo &free);
    static QStringList getPartitionMountPoints(const SPartitionInfo &free);
    static QString     getPartitionMountPoint(const QString text);
    static QString     getPartitionMountText(const QString text);

    static QString     labelToMountPoint(const QString &label);

    // 显示大小适配相关的接口
    static bool isBigScreen();
    static QSize getShowSize();             // 显示空间适配的大小
    static QSize getInnerSize();            // 内部空间适配的大小
    static int getInnerMargin();
    static int getTopMargin();
    static int getBottomMargin();
    static QSize getDiskIconSize();
    static QSize getInstallResultIconSize();
    static QSize getTimezoneMapSize();
    static QSize getQRPicSize();
    static QSize getAnimationPicSize();


    // 加密密钥文件
    static bool cryptKeyFile(QString &keyPath, QString &err);
    static bool bindRecoveryKeyToDev(const QString &diskPwd, const QStringList &cryptDevList, const QString &keyStr, QString &err);
    static bool isCryptDisk();

    // 单位换算接口
    static double mToG(qint64 size);
    static qint64 gToM(double size);
    static qint64 gToM(qint64 size);
    static qint64 bytesToM(qint64 size);
    static qint64 bytesToG(qint64 size);
    static qint64 kbToMB(qint64 size);
    static qint64 mToSectors(qint64 size, qint64 sectorsSize);
    static qint64 sectorsToM(qint64 sectorsCnt, qint64 sectorsSize);
    static qint64 percentageToM(qint64 size, const QString &percentage);
    static qint64 strToM(const QString &size);

    static QString showSizeUnit(double &size, int bit = 1);
    static QString showSizeUnit(double &size, double &totai, int flag = 1);

    // 获取内存大小,单位MB
    static qint64 getMemSize();
    static qint64 getSwapSize();

    // 获取分区的结束扇区的位置
    static qint64 getPartitionEndPoint(qint64 startPoint, qint64 partSize, qint64 diskSize, qint64 sectors);
    static qint64 getPartitionEndPoint(qint64 partEndPoint, qint64 diskSize, qint64 sectors);
    static qint64 getPartitionStartPoint(qint64 startPoint, qint64 sectors);

    static QString getEmptyMountPoint();
    static std::pair<QString, bool> getPartitionStartPointText();
    static std::pair<QString, bool> getPartitionEndPointText();

    // 命令执行相关接口
    static bool execHook(const QString &hooksPath, QString &log);
    static bool command(const QString cmd, const QStringList &args, QString &log);
    static bool commandPrintLog(const QString cmd, const QStringList &args, QString &log);
    static bool getCmdListReturn(const QString &cmd, QStringList &cmdLog, QString &err);

    // 前端界面接口
    static void deleteLayout(QLayout *layout); // 清理界面所有布局以及布局中空间
    static void setLabelTextAndTooltip(QLabel *label, const QString& strText, const int& textwidth); //设置label上的文字及缺省tooltip

    // 通过分区的挂载点判断是否是启动盘
    static bool isStartDisk(const QList<SPartitionInfo> &part);

    // 读取配置文件接口
    static bool readJsonFile(const QString& filepath, QJsonDocument &policydoc, QString &err);

    // tpm安装
    static bool isTPM();
    // ghost安装
    static bool isGhost();
    static EGhostCode checkUimagePath(const QString &uimagePath, const QString &devs, QString &out);

    // 是否有后配置阶段，true有false无
    static bool isFirstBoot();

    // 引导判断的接口
    static bool isUEFI();
    static bool isSW();

    // 判断是否是pxe安装
    static bool isPxeInstall();

    // 设备相关接口
    static bool checkDisklabelType(const SDeviceInfo &deviceinfo);
    static bool setDisklabelType(const SDeviceInfo &deviceinfo, QString &err);
    static bool umountOnDevice(const QString &devicePath, QString &err);
    static bool closeVgOnDevice(const QString &devicePath, QString &err);
    static bool closeCryptOnDevice(const QString &devicePath, QString &err);
    static bool clearDeviceStatus(const QString &devicePath, QString &err);
    static bool checkDeviceExist(const QString &devicePath, QString &err);

    // 分区相关接口
    static bool editPartition(SPartitionInfo &info, QString &err);
    static bool creatPartition(SPartitionInfo &info, QString &err);
    static bool deletePartition(SPartitionInfo &info, QString &err);
    static bool formatPartition(SPartitionInfo &info, QString &err);
    static bool formatPartitionOK(SPartitionInfo &info, QString &err);
    static bool formatPartitionOK(SLVMInfo &info, QString &err);
    static bool getPartitionFileSystem(const QString &path, QString &out);
    static bool sync();
    static bool checkMount(const QString &mountPoint, const QString &device);

    static QJsonObject getPartitionInfo(const QString &path, QString &err);
    // 用于挂载目标系统的挂载点
    static bool mountToTarget(const QString &device, const QString &mountPoint, const QString &option, QString &err);
    // 通用的挂载接口
    static bool mount(const QString &device, const QString &mountPoint, const QString &option, QString &err);
    static bool umount(const QString &device, QString &err);
    static bool isMounted(const QString &path, QString &err);
    static bool partprobe(const QString &path, QString &out);
    static bool cryptsetupPartition(const QString password, const QString label, QString &pv, QString &err);
    static bool findCryptVolum(const QString &partPath, QString &out, QString &err);
    static bool findCryptVolumByLVM(const QString &partPath, QString &out, QString &err);
    // 解密加密卷
    static bool deCryptPartition(const QString password, QString &path, QString &err);
    static QStringList getCryptDevPaths();

    static bool checkIsPartitionEncrypt(const SPartitionInfo &partInfo);
    // 挂载位置是否可写
    static bool mountPointWriteable(const QString &mountPoint);

    // 判断free分区是否包含在扩展分区中间
    static bool isPartContains(const SPartitionInfo &extension, const SPartitionInfo &free);
    static bool isPartContinuous(const SPartitionInfo &fir, const SPartitionInfo &sec);

    // 对齐分区到最近的兆字节
    static qint64 alignPE(qint64 lvSize, qint64 peSize, qint64 peTotal);
    static bool getDevicePaths(const QString &device, QStringList &paths, QString &err);

    // 根据分区类型获取分区名称
    static QString getPartitionTypeName(EPartitionType type);
    // 根据分区操作获取操作名称
    static QString getPartitionOperateName(EPartitionOperate operate);

    static QList<SDeviceInfo> sortDevice(const QList<SDeviceInfo> &devs);

    // lvm相关接口
    static qint64 strGetInt(const QString &str);
    static bool getLVMPESize(const QString &vgName, qint64 &peSize, QString &err);
    static bool getLVMPETotal(const QString &vgName, qint64 &peTotal, QString &err);
    static bool getLVMFreePETotal(const QString &vgName, qint64 &freePeTotal, QString &err);
    static bool createPV(const QString &pvPath, QString &err);
    static bool createVG(const SVGInfo &info, QString &err);
    static bool deleteVG(const SVGInfo &info, QString &err);
    static bool createLVM(SLVMInfo &info, QString &err);
    static bool deleteLVM(const SLVMInfo &info, QString &err);
    static bool editLVM(const SLVMInfo &info, QString &err);
    static SPartitionInfo find(const QString &id, const QList<SPartitionOperate> &infos);
    //读取fstab文件
    static void readFSTab(const QString &tabFile, QList<FSTabInfo> &tabList);

    static bool isFullDiskbyFSTab(const QList<FSTabInfo> &tabList);

    // 重启系统
    static bool rebootSystem();

    // 关闭系统

    static SPartitionInfo getEFIPartitionInfo(const SDeviceInfo &dev);
    static SPartitionInfo getBootPartitionInfo(const SDeviceInfo &dev);
    static long getLabelStartNum();
    static QString getPartitionLabel(const SPartitionInfo &part);
    static QString getPartitionLabel(const SLVMInfo &part);

    //类型转换
    static QString bootloaderToString(bool isUefi);
    static EPartitionOperate qstringToEnumOperate(const QString &strParttitonOperate);
    static EPartitionType qstringToEnumType(const QString &type);
    static EDisklabelType qstringToEnumDisklabel(const QString &strDisklabel);
    static QString diskLabelToString(EDisklabelType type);
    static QString enumOperateToString(EPartitionOperate parttitonOperate);
    static QString enumTypeToString(EPartitionType partitionType);
    static QString spaceUsageToString(ESpaceUsage usage);
    static QMap<QString, EPartitionType> partitionTypeToText(EPartitionType partitionType);
    static QMap<QString, EPartitionType> partitionTypeToText();

    static QMap<QString, EOpeartType> opeartTypeToText(EOpeartType opeartType);
    static QMap<QString, EOpeartType> opeartTypeToText();
    static QString getOpeartTypeText(EOpeartType opeartType);
    static QString boolToString(bool value);

    // 返回所有可用的头像
    static QStringList getAvatars();

    // 根据locale获取两位大写的国家字母代码
    static QString get2AlphaCountryCode(const QString &locale);

    // 获取当前登录的用户名
    static bool getCurrentLoginUserName(QString &name, QString &err);

    static QString getFileSystem(const QString &fileSysytem);

    static void createFile(const QString &filePath);
    //虚拟机
    static bool isVirtualMachine();

    static qint64 availSizeFromMountPoint(const QString &mountPoint, QString &err);

    static QStringList qstringSplitToQStringList(const QString &srcString, const QString &splitString);

    static bool isInDesktop();


    static void setRecoveryKey();

    // 根据传入的宽度和字体，对srcStr进行换行处理
    static QString wrapWords(const QString &srcStr, int distWidth, const QFontMetrics &fm);

    // 查找已经存在的用户目录
    static QStringList findOldUsers(const QString &homePath);
};

#endif // COMMONFUNC_H

