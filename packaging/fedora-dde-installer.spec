Name:           fedora-dde-installer
Version:        0.3.0
Release:        0.2%{?dist}
Summary:        Fedora DDE Remix graphical Live installer
License:        GPL-3.0-or-later
URL:            https://github.com/Fedora-DDE-Remix/fedora-dde-installer
# Development snapshot. For a release, use an immutable tag archive instead.
Source0:        %{url}/archive/refs/heads/main/%{name}-main.tar.gz
ExclusiveArch:  x86_64 aarch64

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  ninja-build
BuildRequires:  cmake(Qt6Core)
BuildRequires:  cmake(Qt6Gui)
BuildRequires:  cmake(Qt6Widgets)
BuildRequires:  cmake(Qt6Xml)
BuildRequires:  cmake(Qt6DBus)
BuildRequires:  cmake(Qt6Concurrent)
BuildRequires:  cmake(Qt6LinguistTools)
BuildRequires:  cmake(Dtk6Core)
BuildRequires:  cmake(Dtk6Log)
BuildRequires:  cmake(Dtk6Gui)
BuildRequires:  cmake(Dtk6Widget)
BuildRequires:  pkgconfig(udisks2-qt6)
BuildRequires:  pkgconfig(libparted)
BuildRequires:  libicu-devel
BuildRequires:  python3-devel
BuildRequires:  desktop-file-utils
BuildRequires:  systemd-rpm-macros

Requires:       qt6-qtsvg
Requires:       python3
Requires:       python3-langtable
Requires:       bash
Requires:       rsync
Requires:       util-linux
Requires:       udisks2
Requires:       shadow-utils
Requires:       authselect
Requires:       sudo
Requires:       polkit
Requires:       dracut
Requires:       dracut-config-rescue
Requires:       cryptsetup
# Fedora 45 ships this executable in systemd-udev, not a separate subpackage.
Requires:       /usr/bin/systemd-cryptsetup
Requires:       grub2-tools
Requires:       grub2-tools-extra
Requires:       grubby
Requires:       kmod
Requires:       efibootmgr
Requires:       policycoreutils
Requires:       selinux-policy-targeted
Requires:       dosfstools
Requires:       e2fsprogs
Requires:       parted
Requires:       xfsprogs
Requires:       btrfs-progs
Requires:       squashfs-tools
Requires:       tzdata
Requires:       kbd
Requires:       xkeyboard-config
Requires:       xorg-x11-server-Xwayland
Requires:       systemd
Requires:       systemd-udev
Requires:       zram-generator-defaults
Requires:       ddm >= 0.5.0-5
%ifarch x86_64
Requires:       grub2-efi-x64
Requires:       shim-x64
Requires:       grub2-pc
Requires:       grub2-pc-modules
%endif
%ifarch aarch64
Requires:       grub2-efi-aa64
Requires:       shim-aa64
%endif

%description
A Qt 6 and DTK 6 installer for Fedora DDE Remix Live images. Installs a
regular writable Fedora system and configures the user account, hostname,
timezone and keyboard in the same Live session before a user-initiated reboot.
This package does not install a first-boot account wizard or an immutable
system deployment backend.

%prep
%autosetup -n %{name}-main

%build
%cmake -GNinja -DFEDORA_DDE_BUILD_GUI=ON -DBUILD_TESTING=ON
%cmake_build

%install
%cmake_install
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop

%check
%ctest

%post
%systemd_post %{name}.service

%preun
%systemd_preun %{name}.service

%postun
%systemd_postun %{name}.service

%files
%license COPYING debian/copyright
%doc README.md docs/BUILDING.md docs/FEDORA_ALIGNMENT.md UPSTREAM.md
%{_bindir}/%{name}
%{_bindir}/%{name}-launch
%{_bindir}/%{name}-config
%{_bindir}/%{name}-parted
%{_bindir}/%{name}-parted-util
%{_libdir}/lib%{name}-common.so.*
%{_prefix}/lib/%{name}/
%{_datadir}/%{name}/
%{_datadir}/dsg/configs/org.fedoradde.installer/
%{_datadir}/applications/%{name}.desktop
%{_datadir}/pixmaps/%{name}.png
%{_datadir}/polkit-1/actions/org.fedoradde.installer.policy
%{_unitdir}/%{name}.service

%changelog
* Fri Nov 27 2026 Fedora DDE Remix contributors - 0.3.0-0.2
- Require Fedora's systemd-cryptsetup executable through its actual file provider.
- Keep the cryptsetup CLI and systemd-udev dependencies for encrypted installation.

* Fri Nov 27 2026 Fedora DDE Remix contributors - 0.3.0-0.1
- Add password-only LUKS2 installation for Btrfs, ext4 and XFS roots.
- Keep encryption secrets in sealed anonymous descriptors and reject plaintext data mounts.
- Generate and verify crypttab and initramfs, and close owned mappings on completion.
- Reserve encryption metadata while retaining payload-aware small-disk checks.

* Thu Nov 26 2026 Fedora DDE Remix contributors - 0.2.2-0.1
- Replace conservative disk limits with payload-aware checks and a 12 GiB root floor.
- Check retained filesystems before partitioning and actual free space before copying.

* Mon Nov 23 2026 Fedora DDE Remix contributors - 0.2.1-0.1
- Support an explicit read-only image profile for the SELinux-disabled test ISO.
- Validate the maintainer's DDM 5 password-login stack with an ISO-owned Fedora bridge.
- Keep enforcing SELinux and direct Fedora PAM checks as the default policy.

* Mon Nov 23 2026 Fedora DDE Remix contributors - 0.2.0-0.1
- Install a validated read-only Fedora Live base with Btrfs root/home and zram.
- Use Fedora kernel-install, rescue hooks, dracut and EFI configuration tools.
- Configure accounts and keyboards before final boot image generation.
- Require package-owned Fedora DDM PAM and preserve payload SELinux policy.
- Preserve existing homes, untouched disks and other systems' EFI fallback.

* Sun Nov 22 2026 Fedora DDE Remix contributors - 0.1.0-0.1
- Initial Fedora 45 Qt 6 / DTK 6 installer adaptation.
- Configure accounts in the installation session and use a writable root system.
