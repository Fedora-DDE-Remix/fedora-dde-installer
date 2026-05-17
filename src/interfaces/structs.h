#ifndef STRUCTS_H
#define STRUCTS_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>

struct SPluginInfo {
    QString name;       // 插件名字，用于界面显示
    QString path;       // 插件路径，用于加载插件
    QString version;    // 插件版本
};

enum class EPartitionOperate
{
    PartitionCreate,
    PartitionDelete,
    PartitionEdit,
    PartitionNormal
};

enum class  EDisklabelType {
    GPT,
    MsDos,
    Others
};

enum class  ESpaceUsage {
    FREE,
    USE
};

enum class  EPartitionType {
    PartitionTypeLogical,
    PartitionTypeExtension,
    PartitionTypePrimary
};

struct SPartitionInfo
{
    QString id;             // 前端生成唯一标识，用于前后端界面操作时判断的标识符
    QString partUuid;       // 分区的partuuid
    ESpaceUsage usage;      // 是否是一个已经分区的空间
    EPartitionType partitionType;    // 分区的类型
    QString filesystem;     // 文件系统
    QString mountPoint;     // 挂载点
    qint64 startPoint = 0;      // 起始位置
    qint64 endPoint = 0;        // 终止位置
    qint64 size = 0;            // 分区大小, 单位M
    qint64 availSize = 0;       // 分区可用大小，单位M
    qint64 byteSize = 0;       // 分区大小，单位bytes
    qint64 sectorSize  = 0;      // 逻辑扇区大小
    qint64 phySectorSize = 0;   // 物理扇区大小
    QString label;          // 分区盘标
    QString device;         // 分区所在的设备
    QString path;           // 分区的路径
    QString cryptPassword;  // 分区的加密密码
    int index = -1;              // 分区索引号，用于删除分区时的参数
    bool isStartPoint = true;      // 开始位置还是结束位置进行分区
    qint64 deviceSize = 0;      // 分区所对应的设备的size
    bool isEFIPartition = false;    // 是否是EFI分区
    bool isMergeVG      = false;    // 标记该分区是否创建合成VG
    bool isNeedFormat   = false;    // 标记该分区是否需要格式化
    bool isStartDevicePart  = false; // 标记该分区是否为启动盘设备的分区
    bool isCanCrypt        = false;    // 标记该分区是否可加密
    bool isCrypt        = false;    // 标记该分区是否加密
    bool decryptStatus = false;         // 标记该分区解密状态
    bool isCryptHeader = false;       // 标记该分区是否加密头
    bool isNeedCryptHeader = false;       // 标记该分区是否需要创建加密头

    bool operator==(const SPartitionInfo &info) const {return this->id == info.id;}
    // 降序排序
    bool operator<(const SPartitionInfo &info) const {return this->startPoint < info.startPoint;}

    QJsonObject marshal() const {
        QJsonObject json;
        json.insert("id", this->id);
        switch (this->usage) {
            case ESpaceUsage::FREE:
                json.insert("usage", "FREE");
                break;
            case ESpaceUsage::USE:
                json.insert("usage", "USE");
                break;
            default: break;
        }

        switch (this->partitionType) {
            case EPartitionType::PartitionTypeLogical:
                json.insert("partitionType", "Logical");
                break;
            case  EPartitionType::PartitionTypeExtension:
                json.insert("partitionType", "Extension");
                break;
            case EPartitionType::PartitionTypePrimary:
                json.insert("partitionType", "Primary");
                break;
            default: break;
        }
        json.insert("filesystem", this->filesystem);
        json.insert("mountPoint", this->mountPoint);
        json.insert("startPoint", this->startPoint);
        json.insert("endPoint", this->endPoint);
        json.insert("size", QString::number(this->size));
        json.insert("availSize", QString::number(this->availSize));
        json.insert("byteSize", QString::number(this->byteSize));
        json.insert("sectorSize", QString::number(this->sectorSize));
        json.insert("phySectorSize", QString::number(this->phySectorSize));
        json.insert("label", this->label);
        json.insert("device", this->device);
        json.insert("path", this->path);
        json.insert("cryptPassword", this->cryptPassword);
        json.insert("index", this->index);
        json.insert("isStartPoint", this->isStartPoint);
        json.insert("deviceSize", QString::number(this->deviceSize));
        json.insert("isEFIPartition", this->isEFIPartition);
        json.insert("isMergeVG", this->isMergeVG);
        json.insert("isNeedFormat", this->isNeedFormat);
        json.insert("isStartDevicePart", this->isStartDevicePart);
        json.insert("isCanCrypt", this->isCanCrypt);
        json.insert("isCrypt", this->isCrypt);
        json.insert("decryptStatus", this->decryptStatus);
        json.insert("isCryptHeader", this->isCryptHeader);
        json.insert("partUuid", this->partUuid);

        return json;
    }
    QString toString() const{
        auto json = marshal();
        QJsonDocument doc(json);
        return doc.toJson(QJsonDocument::Indented);
    }
};
typedef QList<SPartitionInfo> PartitionInfoList;

struct SVGInfo
{
    QString id;
    QString name;
    QStringList pvIds;
    qint64 size = 0;        // VG大小，单位M
    qint64 byteSize = 0;    // VG大小，单位bytes
    bool isCryptHeader = false;       // 标记该分区是否加密头

    bool operator==(const SVGInfo &info) const {return this->id == info.id;}
    bool operator<(const SVGInfo &info) const {return this->name < info.name;}
    QJsonObject marshal() const {
        QJsonObject json;
        json.insert("id", this->id);
        json.insert("name", this->name);
        json.insert("pvIds", pvIds.join(";"));
        json.insert("size", QString::number(this->size));
        json.insert("byteSize", QString::number(this->byteSize));
        json.insert("isCryptHeader", this->isCryptHeader);

        return json;
    }
    QString toString() const{
        auto json = marshal();
        QJsonDocument doc(json);
        return doc.toJson(QJsonDocument::Indented);
    }
};

struct SLVMInfo
{
    QString id;             // 前端生成分区唯一标识，用于前后端界面操作分区时判断的标识符
    QString name;     //lvm的名称
    QString filesystem;
    QString mountPoint;
    QString label;
    QString path;
    qint64 size = 0;            // LVM大小，单位M
    qint64 byteSize = 0;        // LVM大小，单位bytes
    QString vgName;
    QString cryptPassword;  // 分区的加密密码
    ESpaceUsage usage;      // 是否是一个已经分区的空间
    bool isNeedFormat = false;    // 标记该分区是否需要格式化
    bool isCanCrypt        = false;    // 标记该分区是否可加密
    bool isCrypt        = false;    // 标记该分区是否加密
    bool decryptStatus = false;         // 标记该分区解密状态
    bool isCryptHeader = false;       // 标记该分区是否加密头

    bool operator==(const SLVMInfo &info) const {return this->id == info.id;}

    QJsonObject marshal() const {
        QJsonObject json;
        json.insert("id", this->id);
        json.insert("name", this->name);
        json.insert("filesystem", this->filesystem);
        json.insert("mountPoint", this->mountPoint);
        json.insert("label", this->label);
        json.insert("path", this->path);
        json.insert("size", QString::number(this->size));
        json.insert("byteSize", QString::number(this->byteSize));
        json.insert("vgName", this->vgName);
        json.insert("cryptPassword", this->cryptPassword);
        switch (this->usage) {
            case ESpaceUsage::FREE:
                json.insert("usage", "FREE");
                break;
            case ESpaceUsage::USE:
                json.insert("usage", "USE");
            default:
                break;
        }
        json.insert("isNeedFormat", this->isNeedFormat);
        json.insert("isCanCrypt", this->isCanCrypt);
        json.insert("isCrypt", this->isCrypt);
        json.insert("decryptStatus", this->decryptStatus);
        return json;
    }

    QString toString() const{
        auto json = marshal();
        QJsonDocument doc(json);
        return doc.toJson(QJsonDocument::Indented);
    }
};

struct SDeviceInfo {
    QString name;                     // 磁盘的名字
    QString device;                   // 磁盘的设备文件
    QString filesystem;               // 文件系统
    QString mountPoint;               // 挂载点
    QString transporttype;            // 设备传输类型,用于区分sata、nvme、usb设备
    QString cryptPartPath;            // 设备里面加密分区的路径
    QString ptuuid;                   // 磁盘的ptuuid
    qint64 size = 0;                  // 磁盘大小，单位M
    qint64 availSize = 0;             // 磁盘可用大小，单位M
    qint64 byteSize = 0;             // 磁盘大小，单位bytes
    qint64 sectors = 0;              // 逻辑扇区大小
    qint64 physectors = 0;           // 物理扇区大小
    qint64 freesize = 0;             // 未使用的容量大小，单位M
    qint64 freeByteSize = 0;          // 未使用的容量大小，单位byte
    EDisklabelType type;              // 磁盘格式
    bool isFormat = false;            // 是否格式化磁盘
    bool rota     = false;            // 判断是ssd设备(false),还是hdd设备(true)
    bool isStartDevice = false;       // 标记是否是启动盘设备
    bool isCrypt = false;             // 标记该磁盘是否是加密设备
    bool decryptStatus = false;       // 标记该分区解密状态
    bool isCreateCryptHeader = false; // 标记是否需要创建加密头
    bool isVentoyDevice = false;       // 标记是否是ventoy设备

    bool operator==(const SDeviceInfo &info) const {return !info.device.isEmpty() && this->device == info.device;}
    bool operator!=(const SDeviceInfo &info) const {return this->device != info.device;}
    bool operator< (const SDeviceInfo &info) const {return this->device < info.device;}

    QJsonObject marshal() const {
        QJsonObject jsonObject;
        jsonObject.insert("name", name);
        jsonObject.insert("device", device);
        jsonObject.insert("filesystem", filesystem);
        jsonObject.insert("mountPoint", mountPoint);
        jsonObject.insert("transporttype", transporttype);
        jsonObject.insert("cryptPartPath", cryptPartPath);
        jsonObject.insert("size", QString::number(size));
        jsonObject.insert("availSize", QString::number(availSize));
        jsonObject.insert("byteSize", QString::number(byteSize));
        jsonObject.insert("sectors", QString::number(sectors));
        jsonObject.insert("physectors", QString::number(physectors));
        jsonObject.insert("freesize", QString::number(freesize));
        jsonObject.insert("freeByteSize", QString::number(freeByteSize));
        jsonObject.insert("type", static_cast<int>(type));
        jsonObject.insert("isFormat", isFormat);
        jsonObject.insert("rota", rota);
        jsonObject.insert("isStartDevice", isStartDevice);
        jsonObject.insert("isCrypt", isCrypt);
        jsonObject.insert("isFormat", isFormat);
        jsonObject.insert("decryptStatus", decryptStatus);
        jsonObject.insert("isCreateCryptHeader", isCreateCryptHeader);
        jsonObject.insert("ptuuid", ptuuid);
        return jsonObject;
    }
    QString toString() const{
        auto json = marshal();
        QJsonDocument doc(json);
        return doc.toJson(QJsonDocument::Indented);
    }
};

struct SDeviceOperate {
    EPartitionOperate operate;
    SDeviceInfo device;

    bool operator==(const SDeviceOperate &info) const {return this->device == info.device;}
};

struct SPartitionOperate {
    EPartitionOperate operate;
    SPartitionInfo partition;

    bool operator==(const SPartitionOperate &info) const {return this->partition == info.partition;}
};

struct SLVMOperate {
    EPartitionOperate operate;
    SLVMInfo lvm;

    bool operator==(const SLVMOperate &info) const {return this->lvm.id == info.lvm.id;}
};

struct SVGOperate {
    EPartitionOperate operate;
    SVGInfo vg;

    bool operator==(const SVGOperate &info) const {return this->vg.id == info.vg.id;}
};

struct SLanguageItem {
    QString name;  // English name.
    QString locale;  // 语言的名称, 像en_US.
    QString localName;  // Localized name.
    QString timezone;  // 默认的时区, 可能会为空 "".
};
typedef QList<SLanguageItem> LanguageList;


struct SPackagePage {
    QString page;
    QStringList package;
};

struct SPackage {
    QString name;
    QList<SPackagePage> defaults;
    QList<SPackagePage> uninstalls;
    QList<SPackagePage> choiceuninstalls;
    QList<SPackagePage> choices;
};

struct SDeviceSizeInfo {
    QString color;      // 空间对应的色块显示颜色
    QString text;       // 空间对应的大小显示文案
    qint64  size = 0;       // 空间对应的大小
    bool edit;          // 是否绑定编辑按钮，目前只绑定系统空间

    bool operator==(const SDeviceSizeInfo &info) const {return this->color == info.color;}
};

enum class  EDataType {
    DEVICE,
    PARTATIONS,
    VG,
    LV
};

struct SUimgFileInfo {
    QString path;           // 文件路径
    QString device;         //文件所在设备
    SUimgFileInfo() = default;
    SUimgFileInfo(const QString &filePath, const QString &dev):path(filePath), device(dev) {
        ;
    }

    bool operator==(const SUimgFileInfo &info) const {return this->path == info.path;}
};

inline uint qHash(const SUimgFileInfo &key, uint seed)
{
    return qHash(key.path, seed) ^ qHash(key.device, seed);
}

struct SDataInfo {
    QString id;
    QString name;
    qint64 size = 0;
    QString fileSystem;
    ESpaceUsage usage;
    EDataType type;
    QString mountPoint;
    QString label;          // 分区盘标
    QString path;           // 分区的路径
    QString device;
    EPartitionType partitionType;    // 分区的类型
    bool isMergeVG = false;
    bool isNeedFormat   = false;    // 标记该分区是否需要格式化
    bool isCanEncrypt;
    bool isEncrypt;
};

struct SHooksInfo {
    QString id;
    QStringList hooks;

    bool operator==(const QString &id) const {return this->id == id;}
};

struct PartMountPointInfo {
    int pathDeep;          // 挂载点路径深度
    QString partPath;      // 分区路径
    QString mountPoint;    // 挂载点
    QString mountOption; //挂载选项
};

struct GhostCheckInfo {
    QString mountPoint;
    QString path;
    quint64 size;
};

enum class EDevType {
    SystemDev,
    DataDev
};

enum class EOpeartType
{
    Edit,
    Delete,
    Add,
    Encrypt,
    Decrypt,
    Others
};

enum class OSType {
    Community,      //社区版
    Professional,   //专业版
    Server,         //服务器版
    Personal,       //个人版
    Device,         //专有设备
    Education,       //教育版
    Military        //军队版
};

enum EGhostCode
{
    CanInstall = 0,                 // 0---可以安装
    Carefull = 1,                   // 0---谨慎安装
    Forbid = 2,                     // 0---不能安装

    // Warning 级别错误码范围从 101 ~ 199
    DisplayCardWarning = 101,       // 显卡不一致, 谨慎安装
    CpuWarning = 102,               // CPU不匹配, 谨慎安装

    // Error 级别错误码范围从 201 ~ 299
    ReadError = 201,                // 文件读取错误，不能安装
    DiskNumInconsistentError = 202, // 磁盘数量不一致, 不能安装
    DiskSizeError = 203,            // 磁盘大小不满足, 不能安装
    BootLoaderModeError = 204,      // 引导模式不一致, 不能安装
    ArchError = 205,                // 架构不一致, 不能安装
    UimgReadError = 206,            // uimg 镜像读取失败, 不能安装
    InstallPositionError = 207,     // 安装设备选择了ghost镜像所在的磁盘
    UnkownError = 299               // 未知错误
};

enum class OSVersion:int {
    V20,
    V23
};

enum class KeepDataError:int {
    NO_ERROR,
    BOOT_ERROR,
    VERSION_MATCH_ERROR,
    OTHER_ERROR
};

typedef struct {
    QString appName;
    QString appPath;
    QString appIcon;
    bool installed = false;
}AppInfo ;

struct FSTabInfo {
    QString spec;                           //设备或者文件系统
    QString mountPoint;           //挂载点
    QString fstype;                       //文件系统类型
    QString mountOption;       //挂载选项
    int dump;                                //dump字段
    int fsck;                                    //fsck字段

    friend QDebug operator<<(QDebug debug, const FSTabInfo &info);
};

#endif // STRUCTS_H
