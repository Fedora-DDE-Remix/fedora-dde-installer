# Building Fedora DDE Remix Installer

The supported packaging targets are Fedora 45 on x86_64 and aarch64. Build
natively for each architecture. The graphical interface uses Qt 6 and DTK 6;
the Fedora DDE Remix COPR supplies the matching desktop libraries.

The installer is an application for a prepared Live image. Building its RPM
does not build an ISO or establish that a target disk can boot. Use the separate
`Fedora-DDE-Remix/ISO-Build` project for image integration.

## Safe tests on a development machine

These tests operate on a workflow state machine and mocked backend operations.
They do not format, mount, repartition or reboot the development machine.
Qt and DTK are not needed for this target.

```bash
cmake -S . -B build-tests \
  -DFEDORA_DDE_BUILD_GUI=OFF \
  -DBUILD_TESTING=ON
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

Python 3, a C++17 compiler and CMake 3.16 or newer are required. Configure with
`FEDORA_DDE_BUILD_GUI=ON` for the actual graphical application; this is the default.

## Install Fedora build dependencies

Run these commands in a Fedora 45 build VM or container:

```bash
sudo dnf install git rpm-build cmake gcc-c++ ninja-build dnf5-plugins
sudo dnf copr enable mozixun/Fedora-DDE-Remix
sudo dnf builddep packaging/fedora-dde-installer.spec
```

`builddep` reads the SPEC's `BuildRequires`, including Qt 6, DTK 6, libparted,
udisks2-qt6 and ICU. Do not skip unresolved dependencies: a
missing COPR build must be fixed before building the installer. The upstream
DDM/session-shell tray-module development dependency is not used by this port.

The dependency names have been checked against the desktop repository's RPM
definitions at commit `c2a29af363a2c573d47bdb7809e01ab454c2f5d9`:

| CMake or pkg-config dependency | Fedora DDE package | Definition checked |
|---|---|---|
| `Dtk6Core`, `Dtk6Gui`, `Dtk6Widget` | `dtk6core-devel`, `dtk6gui-devel`, `dtk6widget-devel` | DTK 6.7.54 |
| `Dtk6Log` | `dtk6log-devel` | Required by `Dtk6CoreConfig.cmake` |
| `udisks2-qt6` | `udisks2-qt6-devel` | udisks2-qt6 6.0.1 |

`Dtk6Log` is listed explicitly in the installer SPEC because it is needed when
loading the Core CMake package. The runtime also explicitly requires `udisks2`
for the D-Bus disk service, `systemd-udev` for partition identification and
`efibootmgr` for UEFI boot entries. Resolving these packages still requires a
successful build in the configured Fedora 45 COPR chroot.

## Build and stage the application

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

Staging does not install the application into your running system. Inspect the
files under `stage/`, then use the RPM for installation into the Live image.
Translation catalogs are generated without modifying the source `.ts` files.
The optional development terminal plugin is disabled; explicitly enable
`FEDORA_DDE_DEBUG_CONSOLE` only in a developer build with QTermWidget6 available.

## Build the RPM

The development SPEC uses the repository's `main` archive. The following commands
package the exact local commit, without fetching a potentially newer branch:

```bash
mkdir -p rpmbuild/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
git archive --format=tar --prefix=fedora-dde-installer-main/ HEAD |
  gzip -n > rpmbuild/SOURCES/fedora-dde-installer-main.tar.gz
rpmbuild --define "_topdir $PWD/rpmbuild" \
  -ba packaging/fedora-dde-installer.spec
```

Uncommitted edits are not included in `git archive`. To publish a release, update
the SPEC to use an immutable release tag and record the source commit. The
current `0.1.0-0.1` package is a development build, not a release qualification.

## Installed layout

| Path | Purpose |
|---|---|
| `/usr/bin/fedora-dde-installer-launch` | Fixed desktop entry; uses existing sudo authorization or asks through polkit |
| `/usr/bin/fedora-dde-installer` | Main graphical installer |
| `/usr/bin/fedora-dde-installer-config` | Configuration helper |
| `/usr/bin/fedora-dde-installer-parted*` | Partition helpers |
| `/usr/lib/fedora-dde-installer/modules/` | Private interface plugins, identical path on both architectures |
| `/usr/share/fedora-dde-installer/` | Hooks, backend, translations and configuration |
| `/usr/share/pixmaps/fedora-dde-installer.png` | Application icon |
| `/usr/lib/systemd/system/fedora-dde-installer.service` | Live-session preinitialization; no first-boot account wizard |

The common shared library uses the distribution's normal library directory
(`/usr/lib64` on these Fedora architectures). Internal plugin class names remain
compatible with the inherited plugin ABI; executable and installed application
names use the new Fedora DDE Remix identity.

The desktop launcher uses the XWayland/XCB display path so the authorized root
GUI can use the Live session's `DISPLAY` and `XAUTHORITY`. It does not create
passwordless sudo or polkit permissions. The image must provide XWayland and its
normal Live-user authorization. The installed target must remove the Live-only
installer/autostart configuration during finalization.

## Continuous integration and release verification

The workflow first runs the non-destructive tests without GUI dependencies. A
Fedora 45 container then installs the declared dependencies and builds source
and binary RPMs for x86_64. Failure to resolve the COPR packages is reported as a
build failure, rather than silently using substitute packages. aarch64 RPMs
must be built on an aarch64 builder; the x86_64 CI result does not validate them.

CI never runs the installer against a host disk. Before publishing an ISO,
verify it with disposable virtual disks and a complete installation cycle:

1. Boot the intended Live image and start the installer from its desktop entry.
2. Install to a disposable disk, then configure the account in the same session.
3. Test invalid account data, a configuration failure and retry. None should
   restart installation or report completion before finalization succeeds.
4. Restart only after the completion page, detach the ISO, and boot the target.
5. Check local login, writable root, network, DDE updates and SELinux labels.
6. Test x86_64 UEFI/BIOS and aarch64 UEFI separately where those image targets
   are advertised. Test Secure Boot separately before claiming it is supported.

## References

- [DNF5 builddep command](https://dnf5.readthedocs.io/en/stable/dnf5_plugins/builddep.8.html)
- [Fedora DDE RPM definitions](https://github.com/Fedora-DDE-Remix/Deepin-Desktop-Environment)
- [DTK Core RPM definition checked](https://github.com/Fedora-DDE-Remix/Deepin-Desktop-Environment/blob/c2a29af363a2c573d47bdb7809e01ab454c2f5d9/dtkcore/dtk6core.spec)
- [UDisks Qt 6 RPM definition checked](https://github.com/Fedora-DDE-Remix/Deepin-Desktop-Environment/blob/c2a29af363a2c573d47bdb7809e01ab454c2f5d9/udisks2-qt6/udisks2-qt6.spec)
- [Upstream DTK Widget CMake exports](https://github.com/linuxdeepin/dtkwidget/blob/6.7.54/misc/DtkWidgetConfig.cmake.in)
