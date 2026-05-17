#ifndef QTPLUGINSMANAGER_GLOBAL_H
#define QTPLUGINSMANAGER_GLOBAL_H

#include <QtCore/qglobal.h>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QObject>

#if defined(QTPLUGINSMANAGER_LIBRARY)
#  define QTPLUGINSMANAGERSHARED_EXPORT Q_DECL_EXPORT
#else
#  define QTPLUGINSMANAGERSHARED_EXPORT Q_DECL_IMPORT
#endif

#ifndef TEST
#define TEST 1
#endif

//多页面公共常量
const int glayoutLeftMargin = 100;
const int glayoutRightMargin = 100;
const int glayoutTopMargin = 60;
const int glayoutBottomMargin = 60;
const int gpartitionBntHeight = 90;
const int gpartitionBntWidth = 560;
const int glayoutSpacing = 10;
const int gspacerItem = 500;
const int glayoutMargin = 20;
const int giconsize = 32;
const int glinewidth = 2;
const int grgb_r = 245;
const int grgb_g = 245;
const int grgb_b = 245;
const int gverticallineheight = 473;
const int glistviewlayoutMargin = 80;
const int gitemwidth = 120;
const int gitemheight = 60;
const int gitemSpacing = 20;
const int gnextbuttonwidth = 360;
const int gnextbuttonheight = 36;

//设置主窗体大小
const int gmainwindowwidthbig =  1024;
const int gmainwindowheightbig = 768;
const int gmainwindowwidthsmall =  700;
const int gmainwindowheightsmall = 500;

const QString gInstallModeDefault = "default";
const QString gInstallModeAutoInstall = "auto-install";
const QString gInstallModeNoFirstBoot = "no-first-boot";
const QString gInstallModeAutoNoFirstBoot = "auto-no-first-boot";
const QString gInstallModeAll = "all";

const QString gInstallDirBeforeChroot = "hooks/before_chroot";
const QString gInstallDirInChroot = "hooks/in_chroot";
const QString gInstallDirAfterChroot = "hooks/after_chroot";
const QString gInstallDirUserConfig = "hooks/user_config";
// 本地调试脚本的目录
const QString gInstallDirTest = DI_LIVE_DIR_ENV"/oem/";

const QString gHooksManagerScript = "scripts/hook_manager.sh";

const QString gInstallConfigInit = "init";
const QString gInstallConfigGet = "get";
const QString gInstallConfigSet = "set";

const qint64 giDeviceUnitSize = 1024;
const qint64 gDeviceUnitSize = 1000;

const qint64 gEFIBackupSectors = 33;

const QString gEFILabel = "EFI";
const QString gBootLabel = "Boot";
const QString gRootaLabel = "Roota";
const QString gRootbLabel = "Rootb";
const QString gBackupLabel = "Backup";
const QString gSWAPLabel = "SWAP";
const QString gDataLabel = "_dde_data";
const QString gHomeLabel = "_dde_home";
const QStringList gNeedFormatMountPoints {"/", "/boot", "/persistent"};

const QString gCryptoLuksFileSystem = "crypto_luks";
const QString gLVM2PVFileSystem = "lvm2 pv";
const QString gSwapMountPoint = "swap";
const QString gLinuxSwapFileSystem = "linux-swap";
const QStringList gSwapFileSystems = {gLinuxSwapFileSystem, "swap"};

const QString gSystemLogDir = "/var/log/deepin-installer/";

const int gMBRMaxPartitionSize = 4;

const QString gEditOption = "edit";
const QString gDeleteOption = "delete";
const QString gNewOption = "new";
const QString gNormalOption = "normal";

const QString gLogical = "logical";
const QString gExtended = "extended";
const QString gPrimary = "primary";
//分区大小误差范围，单位MiB
const int gPartitionDelta = 1;
const qint64 gDiskThreshold = 225 * giDeviceUnitSize; //单位MiB
const qint64 gDiskExtendSize = 5 * giDeviceUnitSize; //单位MiB

enum InstallMode {
    FullDiskMode = 0,
    AdvancedMode,
    Cryptsetup,
    SaveData,
    MountMode,
    GhostMode,
    CustomMode,
    LvmFullDisk
};

#endif // QTPLUGINSMANAGER_GLOBAL_H
