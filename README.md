# Fedora DDE Remix Installer

面向 **Fedora 45、x86_64 / aarch64** 的 Fedora DDE Remix 图形安装器，基于
[deepin-installer-reborn](https://github.com/guanzi008/deepin-installer-reborn)
改造，使用 Qt 6 和 DTK 6。软件包与可执行程序统一命名为
`fedora-dde-installer`，界面、图标和安装幻灯片使用 Fedora DDE Remix 品牌。

本项目安装普通、可写的 Fedora 系统，使用 RPM/DNF 管理软件。原有 OSTree、
不可变根文件系统和首次开机账户配置服务不再作为安装路径。

## 安装流程

1. 在 Fedora DDE Remix Live 桌面启动安装器，选择语言和目标分区，确认安装。
2. 写入系统，生成目标系统的 initramfs、BLS 启动项和 GRUB 配置。
3. **保持当前 Live 会话，无需重启**，直接配置用户名、密码、主机名、键盘、
   时区和区域格式。账户和设置写入已安装的目标系统。
4. 配置成功并完成目标分区卸载后，显示完成页。用户可以选择“立即重启”，
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
| 根文件系统 | ext4、XFS 或 Btrfs；默认自动分区使用 ext4 |
| 分区模式 | 全盘安装、普通手工分区、空闲空间安装 |
| `/boot` | **必须有独立的 ext4 分区，且新建或显式选择格式化**；默认大小 2 GiB |
| UEFI | `/boot/efi` 使用 FAT32 EFI 系统分区，可复用现有 ESP；使用 Fedora 软件包中的 shim/GRUB 文件 |
| BIOS | 仅 x86_64，当前限单磁盘 MBR/MS-DOS 布局 |
| 最小安装容量 | 默认安装磁盘容量门槛为 50 GB |
| 当前不支持 | 加密安装、LVM 安装、镜像克隆、不可变系统部署、首次开机再创建账户 |

手工分区时，根分区和 `/boot` 都必须新建或明确选择格式化。其他分区能否保留，
以界面显示的操作计划为准。此版本要求独立 `/boot`，不接受把它放在根文件系统内。

## 开发状态与验证边界

当前为开发版。已经通过不接触真实磁盘的纯 C++ 工作流回归测试和 Python 后端
模拟测试，以及启动脚本语法、资源路径和打包配置检查。

**完整 Qt 6 / DTK 6 GUI 编译、Fedora RPM 构建、Live ISO 虚拟机安装及首次启动
尚未完成验证。** 安全测试通过不代表 ISO 已能成功安装或启动。GitHub Actions
配置了实际的 Fedora 45 RPM 构建任务，依赖缺失或编译失败会使任务失败。

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
SELinux 策略、所需 `glibc-langpack-*` 和 SPEC 声明的运行依赖。

Live 镜像需通过 Fedora dracut Live 启动路径启动，内核参数包含 `rd.live.image`
或 `root=live:`。桌面中的“安装 Fedora DDE Remix”入口执行
`/usr/bin/fedora-dde-installer-launch`；启动器会准备本会话配置，然后打开安装界面。
它使用 Live 镜像现有的 sudo 授权，或通过 polkit 请求认证，不额外创建免密规则。
图形授权路径需要 Live 桌面提供 XWayland、`DISPLAY` 和对应显示认证信息。

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
