# Fedora 45 安装结果对齐记录

本项目的目标是安装普通、可写、可由 RPM/DNF 正常维护的 Fedora DDE Remix。
0.2.0 按下面固定版本的 Fedora 实现修正当前后端，同时保留 DDE 界面和
“写入系统后，在同一个 Live 会话中配置账户，再由用户决定何时重启”的流程。

这份记录区分源码已实现的行为与真实安装验证。通过编译、单元测试或 RPM 打包，
都不能代替移除 ISO 后的启动和登录测试。

## 比较基线

| 组件 | 固定参考 |
|---|---|
| Anaconda | [45.30，`9be834bf2500afddedee3f8f4b3496a474241c32`](https://github.com/rhinstaller/anaconda/tree/9be834bf2500afddedee3f8f4b3496a474241c32) |
| Blivet | [3.14.2，`ab7277b876a85173a2f144872fe91d9cbfd7d61e`](https://github.com/storaged-project/blivet/tree/ab7277b876a85173a2f144872fe91d9cbfd7d61e) |
| Fedora KIWI 描述 | [f45，`daf359394913068f4fc18a8b7aafc404caeb8257`](https://forge.fedoraproject.org/releng/kiwi-descriptions/src/commit/daf359394913068f4fc18a8b7aafc404caeb8257) |
| Fedora grub2 配方 | [f45，`3a43c62954ea4ca3c81483db7875502297ebd6fc`](https://src.fedoraproject.org/rpms/grub2/tree/3a43c62954ea4ca3c81483db7875502297ebd6fc) |
| Fedora dracut 配方 | [f45，`470f6b470191e3eb3d49ec5956ef1ea58e8f612f`](https://src.fedoraproject.org/rpms/dracut/tree/470f6b470191e3eb3d49ec5956ef1ea58e8f612f) |

最终比较应使用**同一个 Fedora 45 DDE Live payload、相同软件包集合、相同虚拟机
硬件和用户选择**，分别由 Anaconda 与本安装器落盘。直接拿 GNOME Workstation
与 DDE Remix 的包清单相比，会把桌面软件选择误当作安装器差异。

## 0.2.0 已落实的改动

| 项目 | 当前行为 | 代码位置 |
|---|---|---|
| 安装源 | 按 `live-base`、`live-osimg-min`、`/run/rootfsbase` 顺序定位原始镜像，独立只读挂载；拒绝运行中的 `/`、可写源和 overlay | `fedora_payload.py` |
| 源验证 | 要求 Fedora 45、必要工具、内核钩子、zram、Fedora DDM PAM 和 SELinux 标签；分区前重新检查源挂载身份 | `fedora_payload.py`、`fedora_system.py` |
| 系统复制 | 保留 RPM 目录、所有者、权限、硬链接、ACL 和 xattr；排除运行时挂载、机器身份、Live 家目录、旧 BLS/救援文件 | `fedora_payload.py` |
| FAT / KIWI 特殊目录 | ESP 单独复制且不复制 `EFI/BOOT`；`/boot/grub2`、`/etc/sysconfig`、`/usr/lib/grub` 单独保留 POSIX 元数据，避免 FAT/xattr 与严格 umask 冲突 | `fedora_payload.py` |
| 默认磁盘布局 | Btrfs 根，建立 `root` 和 `home` 子卷，`compress=zstd:1`，同一文件系统共享容量；独立 `/boot` 为 2 GiB ext4 | C++ 规划器、`fedora_storage.py` |
| 独立 home | 手动选择独立 `/home` 时使用该分区，不再创建同名自动子卷；不使用 rsync `--delete`，删除 Live 账户不递归删除家目录 | `fedora_storage.py`、`fedora_backend.py` |
| 交换空间 | 自动布局不建物理 swap；要求 `zram-generator-defaults`，手动 swap 仍可用；有磁盘 swap 时重建 resume UUID | 默认策略、`fedora_boot.py` |
| BIOS / GPT | x86_64 新建 GPT，预留至少 1 MiB 未格式化 `bios_grub`；保留现有 MBR 支持；helper 后检查实际 GPT 类型 GUID | C++ helper、`fedora_storage.py` |
| 内核安装 | 设置机器 ID、entry-token、sysconfig/kernel，使用 `depmod`、`kernel-install add` 与最终 `dracut`；核验普通及 rescue BLS/内核/initramfs | `fedora_boot.py` |
| initramfs | 在账户、键盘和区域配置之后生成；使用目标 Fedora 的 hostonly 默认配置，只排除 Live 专用 dracut 模块 | `fedora_boot.py`、`configure()` |
| 内核参数 | 重建根 UUID、Btrfs subvol、显式 swap resume；继承必要硬件/console 参数和模块黑名单，去除 Live 启动参数 | `fedora_boot.py` |
| UEFI | 从 Fedora RPM 拥有的路径取 shim/GRUB；使用 `gen_grub_cfgstub`；根据固件位数选择 x64/ia32/aa64 | `fedora_boot.py` |
| fallback | 保护其他系统的文件；新建 fallback 前记录归属与哈希，支持复制中断后重试；NVRAM 失败仅接受本次安装的完整 fallback | `fedora_boot.py` |
| 账户 | 先移除临时 Live 账户释放 UID，再创建 wheel 用户；清除密码变更日期、锁定 root；密码只通过 stdin 传递 | `configure()` |
| 键盘 / 字体 | 使用 payload 的 systemd 键盘映射表与 kbd 文件离线转换布局/变体，使用 langtable 选择字体 | `fedora_system.py` |
| Fedora 配置 | 保留 hosts、authselect、PAM、SELinux 包配置；尊重 NTP 开关，完成后禁用 initial-setup | `fedora_backend.py` |
| SELinux | 继承只读镜像标签，按目标策略定向 restorecon；保留 home 不递归重标记，不使用 `-F` 重置自定义用户/范围 | `verify_selinux_source()`、`relabel()` |
| 失败重试 | 按 Live boot ID、根及各独立分区 UUID/设备/FSROOT 验证目标；分区失败不能静默重复格式化，账户和卸载失败保留可重试状态 | `fedora_backend.py` |

分区 helper 还修复了三项原有问题：保留项 `normal` 不得重建磁盘分区表；全盘规划
不得把刚设置的 GPT 元数据恢复成旧值；普通分区实际路径必须在关闭加密头功能时
也原子回写。新建普通分区、根分区和 `/boot` 都必须明确设置格式化标志，避免误用
原扇区遗留的文件系统签名。

## ISO 必须满足的条件

安装器会在破坏性分区之前检查以下前提，发现缺失就停止：

- 普通 Fedora 45 Live 镜像，提供支持的只读基础设备，不是 OSTree 部署。
- 可用的目标内核、`kernel-install`、GRUB、dracut 和 rescue 钩子。UKI-only compose
  中用空文件屏蔽 `20-grub.install` 或 `50-dracut.install` 的产物不能直接用作此 payload。
- 默认 profile 要求直接接入 Fedora 的 DDM PAM 栈，不根据 RPM 版本号推断可登录。
- 默认 profile 要求已启用的 SELinux 内核接口；Live 可以 permissive，但不能以 `selinux=0` 关闭 LSM。
  payload 的关键文件必须有正确的 `security.selinux` 标签，目标配置为 enforcing。
  默认 profile 不支持没有 SELinux 标签的 EROFS/SquashFS 镜像。
- `zram-generator-defaults`、所需 `glibc-langpack-*`、kbd 键盘与字体数据、DDE/DDM/
  Treeland 和安装器 SPEC 列出的工具。选择某语言前仍会验证该语言和字体是否存在。

调试配置 `DI_SOURCE_ROOT` 默认为 `auto`。显式指定的路径也必须是独立只读挂载，
安装器不会卸载用户自己提供的源。自动挂载的源只在核验身份后释放。

0.2.1 曾为 2026-11-23 的 Fedora 45 x86_64 测试 ISO 增加明确例外：只读 payload 中的
root-owned `install-profile.json` 选择禁用 SELinux，并通过 ISO 自有 `system-login`
桥接使用维护者 DDM 5 的普通密码登录。默认 profile 不变，DDM 原 PAM 文件不改，
Live 使用 `liveuser` / `liveuser` 手动登录，不启用 DDM 5 自动登录。
这项明确选择不再声称与 Fedora 默认 enforcing 策略相同；配置契约、复制和引导行为见
[BUILDING.md](BUILDING.md#explicit-test-iso-profile)。

当前 0.2.2 ISO 配方已恢复默认 `enforcing / fedora` profile，使用维护者更新的
DDM 原生 Fedora PAM 和 SELinux 策略包。compose 检查策略注册及最终只读 payload
标签；运行验证要求 Live 和安装后的系统都保持 Enforcing。

0.2.2 同时将原来的高固定容量门槛改为较小的 UI 基础门槛和实际 payload 空间检查。
整盘初筛为 14 GiB，默认根分区最低 12 GiB；分区操作前按挂载点计算未压缩数据与
余量，复制前再次核对实际可用容量。具体镜像能否安装到十进制 16 GB 磁盘，必须
由该镜像的实际虚拟机安装证明，不能从静态阈值推断。

## 仍然存在的差异

1. **安装引擎**：当前仍为自己的阶段调度与 libparted helper，不是完整 Anaconda/
   Blivet。未支持 LUKS、LVM、软件 RAID、复杂多磁盘策略和 FIPS 安装。
2. **网络**：目标继承原始 payload 的网络默认值；尚未迁移 Live 会话后来建立的
   Wi-Fi、VPN 或带证书连接。未来需对明确选定的 NetworkManager profile 做有权限
   控制的迁移，不能通过复制整个运行中 `/etc` 实现。
3. **镜像安装后脚本**：不任意重放 Anaconda/Kickstart 脚本。镜像需要的额外变换应
   明确实现为已审查的后端操作或 RPM 配置。Live 基础镜像应包含可安装的正常默认值。
4. **保留 home 的复杂场景**：不递归改已有数据的所有者或标签；跨发行版 UID/GID
   映射、带旧用户的独立 home、加密 home 仍需专门设计与测试。
5. **硬件与启动实测**：aarch64、x86 混合位数 UEFI、Secure Boot、实际休眠恢复和
   固件 NVRAM 行为还需要分别验证。复制发行版签名文件不等于已经通过 Secure Boot 验收。
6. **安装产生的额外信息**：本项目自己的安装标记和日志、DDE 会话与品牌文件属于
   预期差异；磁盘 UUID、machine-id、随机种子、时间戳和密码哈希盐也不会逐字相同。

## 验证与发布门槛

本地安全测试覆盖 Python 后端及 C++ 工作流；它们模拟所有外部系统命令，不会操作
真实分区。GUI 的非 Debug 构建（包括未显式指定构建类型的 Fedora RPM）还包含直接调用真实规划器和 helper 解析代码的
`installer-partition-layout` 测试，覆盖 UEFI/BIOS、512/4096 字节扇区、手动布局、
helper 协议及分区路径回写。实际运行记录应关联到对应提交的 GitHub Actions。

完成“与官方安装结果几乎相同”的验收，还需在可丢弃的虚拟磁盘上执行：

| 验收项 | 应比较的结果 |
|---|---|
| 完整安装 | 同会话配置账户；配置失败后重试；完成前不自动重启；移除 ISO 后正常启动 |
| 磁盘 | GPT 类型、ESP/bios_grub、独立 boot、Btrfs root/home、fstab 与 zram；保留 home 的用户数据，不删除内容；允许写入本次选择的用户配置 |
| 软件包 | 对相同 payload 比较已安装 RPM 集合及 `rpm -Va`，逐条解释品牌、桌面和安装器包移除差异 |
| 启动 | 普通/救援 BLS、grubenv 默认项、initramfs 的磁盘与控制台模块；没有 `rd.live.*` 或 `root=live:` |
| 登录 | 首个用户 UID/GID、wheel/sudo、锁定 root、DDM/Treeland 登录、重试未重复创建用户 |
| 配置 | locale、region、键盘变体、vconsole 字体、时区、NTP、authselect 和服务启用状态 |
| SELinux | enforcing 启动，系统和新用户文件标签正确，登录无关键 AVC 拒绝，保留 home 标签未被强制覆盖 |
| 维护 | DNF 更新内核后仍可启动，新内核与 rescue 项正确，RPM 自有 PAM 配置可正常升级 |
| 共存 | 复用 ESP 不覆盖其他系统 fallback；保留分区数据；NVRAM 写入失败时能明确区分可恢复与失败 |

详细构建方法和已有运行记录见 [BUILDING.md](BUILDING.md)。RPM/Actions 产物不能
代替 COPR 仓库发布；COPR 使用手动仓库生成模式时还需要单独发布新元数据。

## 关键官方实现

- [原始 Live 基础设备的发现与只读挂载](https://github.com/rhinstaller/anaconda/blob/9be834bf2500afddedee3f8f4b3496a474241c32/pyanaconda/modules/payloads/source/live_os/initialization.py)
- [Live payload 的复制边界和 FAT/KIWI 目录处理](https://github.com/rhinstaller/anaconda/blob/9be834bf2500afddedee3f8f4b3496a474241c32/pyanaconda/modules/payloads/source/live_image/installation.py)
- [Fedora 配置基线](https://github.com/rhinstaller/anaconda/blob/9be834bf2500afddedee3f8f4b3496a474241c32/data/profile.d/fedora.conf)
- [内核部署工具](https://github.com/rhinstaller/anaconda/blob/9be834bf2500afddedee3f8f4b3496a474241c32/pyanaconda/modules/storage/bootloader/utils.py)
- [EFI 引导实现](https://github.com/rhinstaller/anaconda/blob/9be834bf2500afddedee3f8f4b3496a474241c32/pyanaconda/modules/storage/bootloader/efi.py)
- [Fedora SDDM PAM 基线](https://src.fedoraproject.org/rpms/sddm/raw/f45/f/sddm.pam)
- [restorecon 的 SELinux 内核启用检查](https://github.com/SELinuxProject/selinux/blob/main/policycoreutils/setfiles/setfiles.c)
