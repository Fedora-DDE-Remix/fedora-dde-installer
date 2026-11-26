# Fedora DDE Remix Installer

面向 **Fedora 45、x86_64 / aarch64** 的 Fedora DDE Remix 图形安装器，基于
[deepin-installer-reborn](https://github.com/guanzi008/deepin-installer-reborn)
改造，使用 Qt 6 和 DTK 6。软件包与可执行程序统一命名为
`fedora-dde-installer`，界面、图标和安装幻灯片使用 Fedora DDE Remix 品牌。

本项目安装普通、可写的 Fedora 系统，使用 RPM/DNF 管理软件。原有 OSTree、
不可变根文件系统和首次开机账户配置服务不再作为安装路径。

## 安装流程

1. 在 Fedora DDE Remix Live 桌面启动安装器，选择语言和目标分区，确认安装。
2. 校验并挂载原始只读 Live 镜像，按已确认的计划分区、写入系统。
3. **保持当前 Live 会话，无需重启**，直接配置用户名、密码、主机名、键盘、
   时区和区域格式。账户和设置写入已安装的目标系统。
4. 使用 Fedora 自带的 `kernel-install`、`dracut` 和 GRUB 工具生成普通及救援
   启动项，使 initramfs 包含刚选择的键盘和区域设置。完成标记和分区卸载后，
   显示完成页。用户可以选择“立即重启”，
   也可以继续使用 Live 桌面；安装器不会自动重启。

账户配置失败时，可以修改设置或重试配置；该操作不会重新分区或重新安装系统。
进入安装阶段后，界面会阻止返回分区页面。后端同时校验安装会话及目标挂载身份，
避免把配置写入 Live 系统。

## 当前支持范围

| 项目 | 当前设计 |
|---|---|
| 目标发行版 | Fedora 45，普通 RPM 系统 |
| 架构 | x86_64、aarch64；需分别构建和验证 |
| 桌面 | DDE，DDM 登录管理器，Treeland 会话 |
| 根文件系统 | 默认 Btrfs，`root`、`home` 子卷共享空间，`compress=zstd:1`；手动支持 ext4、XFS、Btrfs |
| 交换空间 | 默认使用 Fedora 的 zram，不自动划分磁盘 swap；支持手动指定 swap |
| 分区模式 | 全盘安装、普通手工分区、空闲空间安装 |
| `/boot` | **必须有独立的 ext4 分区，且新建或显式选择格式化**；默认大小 2 GiB |
| UEFI | `/boot/efi` 使用 FAT32 EFI 系统分区，可复用现有 ESP；使用 Fedora 软件包中的 shim/GRUB 文件 |
| BIOS | 仅 x86_64、单磁盘；新建 GPT + 1 MiB `bios_grub`，也可安装到现有 MBR 布局 |
| 最小安装容量 | 默认整盘初筛 14 GiB、根分区 12 GiB；还按本次 payload 的实际占用和各分区余量检查，小盘是否可安装以实际测量为准 |
| 当前不支持 | 加密安装、LVM 安装、镜像克隆、不可变系统部署、首次开机再创建账户 |

手工分区时，根分区和 `/boot` 都必须新建或明确选择格式化。其他分区能否保留，
以界面显示的操作计划为准。此版本要求独立 `/boot`，不接受把它放在根文件系统内。
新建普通文件系统分区也必须明确标记格式化，不能依赖新分区范围内遗留的旧签名。
手动指定独立 `/home` 时保留该选择，不在 Btrfs 根内另建 `home` 子卷。

0.2.2 下调了原来的 50 GiB 整盘与 32 GiB 根分区门槛。安装器会在分区操作前
统计只读 payload 的未压缩占用，按目标挂载点分配空间要求；根分区至少保留
2 GiB 余量，`/boot` 为生成内核及救援镜像保留额外空间。挂载目标后、复制前还会
核对实际可用空间，不依赖 Btrfs 压缩率来保证能够安装。16 GB（十进制）是本次
ISO 虚拟机验证的目标容量，并不是对所有未来镜像大小的固定承诺。

## 与 Fedora 官方安装结果对齐

0.2.0 按 Fedora 45 Anaconda 45.30 的源码与 Fedora 包策略改进现有后端：
只复制原始只读 payload，保留软件包目录和元数据；使用 Fedora 内核安装钩子、
救援启动项、目标机 dracut 默认、固件匹配的 shim/GRUB 和官方 EFI stub 工具；
保留 authselect、DDM RPM 的 PAM 与 SELinux 配置；配置正式账户后再生成最终引导文件。

这些改进仍使用本项目的分区和阶段后端。**没有接入完整 Anaconda/Blivet 引擎，
也没有完成与官方安装结果的虚拟机逐项验收。** 当前不迁移 Live 会话中新连接的
Wi-Fi/VPN，不执行 Anaconda/Kickstart 的任意安装后脚本。差异、ISO 前提和验收项见
[docs/FEDORA_ALIGNMENT.md](docs/FEDORA_ALIGNMENT.md)。

## 开发状态与验证边界

当前源码版本为 0.2.2。发布验证由 [ISO-Build 的构建与安装测试](https://github.com/Fedora-DDE-Remix/ISO-Build/actions)
记录；更新版本号或通过单元测试不等于镜像已经通过实际安装。

2026-11-23（北京时间）的 [GitHub Actions 构建](https://github.com/Fedora-DDE-Remix/fedora-dde-installer/actions/runs/45055520645)
已完成 **0.2.0 的完整 Qt 6 / DTK 6 GUI 编译、Fedora 45 x86_64 RPM/SRPM 打包**。
验证提交为 `320197121fb194a327e7fcd60f5c552c901add4d`，直接使用正式 COPR
开发包，未启用临时 SDK 补建。三项 CTest 均实际执行并通过：C++ 工作流、
包含 80 项 Python 用例的后端测试，以及真实 C++ 分区规划器/协议回归。
[安装器构建产物](https://github.com/Fedora-DDE-Remix/fedora-dde-installer/actions/runs/45055520645/artifacts/14767373498)
包含主 RPM、SRPM、debuginfo 和 debugsource。

0.2.1 曾为 2026-11-23 的 Fedora 45 x86_64 测试 ISO 增加显式配置：保留维护者的
DDM `0.5.0-5`，由 ISO 自己的配置 RPM 提供 Fedora PAM 桥接，并明确禁用 SELinux。
该旧配置保留兼容。本次 0.2.2 镜像恢复默认的 `enforcing / fedora` profile，使用
维护者已发布的 DDM `0.5.0-8` 原生 Fedora PAM 和 SELinux 策略包，移除 ISO PAM 桥。
本仓库不修改或重建维护者的桌面仓库。上面的 GUI/RPM 构建记录属于 0.2.0，
不能当作新版本或新 ISO 的运行验证。

当前发布门禁包含 UEFI/Btrfs、BIOS/GPT/ext4、BIOS/MBR/ext4、XFS+独立 home+swap、
保留分区及十进制 16 GB 小盘六种实际安装，再移除 ISO 启动并验证 Enforcing 和
DDM 桌面登录。各项是否通过以对应 Actions 报告为准；aarch64 与 Secure Boot
需要独立验证。下方的本地工作流及后端测试只使用模拟命令，不操作真实磁盘。

CI 默认直接使用 COPR 发布的开发包。手动触发工作流时，可显式选择
`bootstrap_widget`，在临时容器内用固定配方补建 Widget SDK；此选项默认关闭。
依赖缺失或编译失败会使任务失败，详细输入记录见构建文档。

发布可供安装的 ISO 前，需要使用可丢弃的虚拟磁盘完成安装、同会话账户配置、
移除 ISO 后启动、DDM 登录、DNF 更新和 SELinux 检查；UEFI、BIOS、aarch64
和 Secure Boot 应分别验证。

## 运行安全测试

需要 CMake 3.16 以上、C++17 编译器和 Python 3。此模式不需要 Qt/DTK，测试使用
临时目录和模拟命令，不会执行真实分区、挂载或重启操作。

```bash
cmake -S . -B build-tests \
  -DFEDORA_DDE_BUILD_GUI=OFF \
  -DBUILD_TESTING=ON
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

## 在 Fedora 45 构建

在对应架构的 Fedora 45 构建虚拟机或容器中安装依赖：

```bash
sudo dnf install git rpm-build cmake gcc-c++ ninja-build dnf5-plugins
sudo dnf copr enable mozixun/Fedora-DDE-Remix
sudo dnf builddep packaging/fedora-dde-installer.spec
```

DTK 6 和 udisks2-qt6 开发包来自项目使用的 Fedora DDE Remix COPR。
SPEC 中的依赖名称已与组织现有 RPM 定义核对；实际包是否可在目标 chroot 中解析，
仍由构建结果确认。

编译并暂存安装文件：

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DFEDORA_DDE_BUILD_GUI=ON \
  -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
DESTDIR="$PWD/stage" cmake --install build
```

`stage/` 是暂存目录，这一步不会安装到当前系统。开发调试终端插件默认关闭。

### 构建 RPM

以下命令打包当前 Git 提交，生成源码 RPM 和二进制 RPM：

```bash
mkdir -p rpmbuild/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
git archive --format=tar --prefix=fedora-dde-installer-main/ HEAD |
  gzip -n > rpmbuild/SOURCES/fedora-dde-installer-main.tar.gz
rpmbuild --define "_topdir $PWD/rpmbuild" \
  -ba packaging/fedora-dde-installer.spec
```

`git archive` 仅包含已经提交的文件。开发 SPEC 使用 `main` 快照，正式发布时应改为
固定标签并记录源提交。详细依赖、安装路径和验证步骤见
[docs/BUILDING.md](docs/BUILDING.md)。

## 集成到 Live ISO

本仓库提供安装器源码和 RPM；ISO 构建由
[Fedora-DDE-Remix/ISO-Build](https://github.com/Fedora-DDE-Remix/ISO-Build)
负责。

将构建出的 RPM 放入镜像构建器可访问的软件源，在 Live 镜像的软件包列表中加入
`fedora-dde-installer`。基础镜像需要是普通 Fedora RPM 根文件系统，并提供可写的
Live 运行环境。镜像应包含 DDE/DDM/Treeland、目标内核、对应架构的引导软件包、
SELinux 策略及文件标签、所需 `glibc-langpack-*`、kbd 键盘/字体数据和 SPEC 声明的运行依赖。
特别需要与所选 profile 匹配的 DDM PAM 栈、`zram-generator-defaults`、
`dracut-config-rescue` 和有效的 Fedora kernel-install 钩子。安装器依赖维护者提供的
`ddm >= 0.5.0-5`；缺少可用 PAM 栈时仍会在分区前停止。

Live 镜像需通过 Fedora dracut Live 启动路径启动，内核参数包含 `rd.live.image`
或 `root=live:`。桌面中的“安装 Fedora DDE Remix”入口执行
`/usr/bin/fedora-dde-installer-launch`；启动器会准备本会话配置，然后打开安装界面。
它使用 Live 镜像现有的 sudo 授权，或通过 polkit 请求认证，不额外创建免密规则。
图形授权路径需要 Live 桌面提供 XWayland、`DISPLAY` 和对应显示认证信息。

`DI_SOURCE_ROOT=auto` 自动从 Fedora Live 基础设备独立挂载只读源，支持带正确标签的
EROFS/SquashFS 或普通 Fedora Live 根镜像；不会把正在运行的 `/` 当作源。
调试时显式指定的源也必须是独立只读挂载。默认 profile 要求 Live 内核启用
SELinux，允许 permissive 模式，并要求已标记且配置为 enforcing 的 payload。

### 本次测试 ISO：禁用 SELinux、保留 DDM 5

ISO-Build 在原始只读 payload 中安装 root 拥有、其他用户不可写的
`/etc/fedora-dde-installer/install-profile.json`：

```json
{"schema":1,"selinux":"disabled","pam":"ddm-system-login"}
```

同时将 payload 的 `/etc/selinux/config` 设为 `SELINUX=disabled`，Live 引导使用
`selinux=0`。安装器把这个选择绑定到安装状态，保留 ACL 和文件 capabilities，
跳过 SELinux 标签复制及 relabel，并为安装后的普通和救援启动项写入 `selinux=0`。
未提供这个文件的镜像仍使用默认 enforcing profile；运行环境变量不能启用例外。
复制时仍排除安装器运行配置和密码，只将已验证的 profile 规范化写入目标，保留
ISO 配置 RPM 的配置文件；后续策略继续使用安装状态中保存的选择。

ISO 自己的配置 RPM 仅新增 `/etc/pam.d/system-login`，接入 Fedora 的
`password-auth`、`postlogin` 和完整 session 栈，不覆盖 DDM 拥有的 PAM 文件。
本版 Live 通过 DDM **手动登录 `liveuser` / `liveuser`**；不使用 DDM 5 中不兼容的
`ddm-autologin`。正式安装账户必须设置自己的密码，且不能属于 `nopasswdlogin` 组。
完整 profile 契约与 PAM 桥内容见 [构建文档](docs/BUILDING.md#explicit-test-iso-profile)。

目标系统完成账户配置后，会移除安装器和 Live 专用启动配置，保留正常 DDM 登录。
初次集成应先生成测试 ISO，在虚拟机中验证完整流程，再提供安装镜像下载。

## 目录与上游归属

- `src/`：Qt/DTK 界面、工作流和分区助手。
- `tools/`：Fedora 安装后端、阶段钩子和 Live 预初始化。
- `configs/`：默认配置、普通分区策略、新品牌图标和幻灯片。
- `packaging/`：Fedora RPM SPEC、桌面入口、启动器和 polkit action。
- `tests/backend/`：不执行真实系统操作的 Python 回归测试。
- `docs/BUILDING.md`：完整构建和验证说明。

上游代码版权声明与 Git 历史保留。来源提交、改造范围和品牌说明见
[UPSTREAM.md](UPSTREAM.md)，GPL 第 3 版许可文本见 [COPYING](COPYING)。
`debian/copyright` 仅保留上游归属记录，旧 Debian 打包与维护脚本已移除。
这是 Fedora DDE Remix 社区项目。
