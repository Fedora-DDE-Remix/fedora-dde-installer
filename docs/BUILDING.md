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
current 0.3.0 package version alone is not a release qualification. The ISO
recipe pins an immutable installer commit and records its RPM hashes; release
qualification additionally requires the real installation jobs to pass.

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
Fedora 45 container then installs the declared dependencies from COPR and builds
source and binary RPMs for x86_64. Required development packages must be built
and published in the normal repository before this workflow can use them.
Failure to resolve a required COPR package is a build failure. aarch64 RPMs must
be built on an aarch64 builder; the x86_64 CI result does not validate them.

On 2026-11-23 (Asia/Shanghai), workflow run
[`45055520645`](https://github.com/Fedora-DDE-Remix/fedora-dde-installer/actions/runs/45055520645)
successfully built commit `320197121fb194a327e7fcd60f5c552c901add4d` using the
published COPR packages, with the optional SDK bootstrap disabled. Full Qt 6 /
DTK 6 GUI compilation, Fedora 45 x86_64 RPM/SRPM packaging and artifact upload
passed. All three CTest entries actually ran: `installer-workflow`,
`installer-backend` (80 Python cases) and `installer-partition-layout`. The last
test exercises the real C++ planners and helper protocol in temporary files,
including both firmware modes and 512/4096-byte sectors, without disk operations.

The [installer artifact](https://github.com/Fedora-DDE-Remix/fedora-dde-installer/actions/runs/45055520645/artifacts/14767373498)
contains `fedora-dde-installer-0.2.0-0.1.fc45.x86_64.rpm`, its SRPM, debuginfo and
debugsource. The archive SHA-256 is
`41163a0c10d26d678ae8b23ebce63199699e4e561a5bb9826665437b5aaa0a2d`.

Version 0.2.1 added the legacy test-image profile described below. It used the
maintainer's DDM 0.5.0-5 with an ISO-owned PAM bridge; the earlier DDM 6 recipe
is not an input to this image. Do not modify or rebuild the separately maintained
desktop repository. The 0.2.0 build above does not validate newer installers or ISOs.
The 0.2.2 image uses the normal enforcing/Fedora profile with the maintainer's
updated DDM 0.5.0-8 and SELinux packages; it does not install the old PAM bridge.

These Actions workflows do not submit COPR builds or publish repository
metadata. `dnf builddep` checks build dependencies; image integration must also
install the output installer RPM and resolve its runtime dependencies. A runtime
upgrade inside a running Live session does not change the original read-only
payload; compose the required packages and profile into the image itself.
See [FEDORA_ALIGNMENT.md](FEDORA_ALIGNMENT.md) for the default payload
prerequisites and remaining Anaconda comparison items.

The package jobs above do not themselves validate Live-image installation,
DDM login or target-system boot. The ISO-Build workflow runs actual disposable
VM installations for UEFI/Btrfs, BIOS/GPT/ext4, BIOS/MBR/ext4, XFS with separate
home and swap, retained partitions, and a decimal 16 GB disk. Inspect each
completed report for its result. Secure Boot and aarch64 require separate tests.

## Capacity checks in 0.2.2

The default UI thresholds are 14 GiB for initial disk filtering and 12 GiB for
the root partition, replacing the earlier 50 GiB disk and 32 GiB root limits.
These thresholds do not guarantee that a particular image fits. Before
partitioning, the backend measures the uncompressed read-only payload for the
chosen mount layout and reserves working space. It checks actual available
space again after mounting and before copying. Btrfs compression is not counted
as required free space, and preserved filesystems are not assumed empty.

## LUKS2 installation in 0.3.0

The Live payload must include `cryptsetup`, `systemd-cryptsetup`, dracut's crypt
module and the selected filesystem utilities. The installer RPM declares these
dependencies. Supported new encrypted roots are Btrfs, ext4 and XFS. The ESP
and separate ext4 `/boot` remain unencrypted; all user data stays within the
single encrypted root. Separate home, var, data and disk swap mounts are rejected
in encrypted mode before partitioning. Btrfs root/home subvolumes share the same
LUKS2 container. Existing LUKS reuse, detached headers and LVM are not implemented.

The GUI passes a confirmed 8–1024-byte UTF-8 passphrase through a sealed memfd
named `fedora-dde-crypt-passphrase`. Only preflight and partition hooks inherit
it; `FEDORA_DDE_CRYPT_FD` contains only the descriptor number. Plans, INI files,
argv and target key files do not contain the passphrase. The helper creates the
physical partition without formatting the raw root as a plaintext filesystem.
Python validates its geometry, creates LUKS2 with Argon2id and a fixed 32 MiB
data offset, then formats the opened mapper. Capacity checks subtract that
overhead before applying the existing inner-root minimum and payload allowance.

Boot generation uses a UUID-based, password-only crypttab and `rd.luks.uuid`.
The resulting initramfs must contain the crypt module and matching no-key-file
crypttab. Finalization unmounts the target before closing only the mapper opened
by this installation; cleanup can resume after a close failure. The ISO VM
matrix includes UEFI/Btrfs on decimal 16 GB, BIOS/ext4 and UEFI/XFS encrypted
installations. Inspect each new report: adding scenarios does not mean they
have passed, and earlier unencrypted reports do not validate encryption.

## Fedora Enforcing release profile

The current release ISO places this explicit profile in its read-only payload:

```json
{"schema":1,"selinux":"enforcing","pam":"fedora"}
```

The ISO build verifies the maintainer's native Fedora DDM PAM stack, installed
SELinux modules and actual filesystem labels. VM jobs require Enforcing during
Live boot and after disk-only boot and authenticate through the real DDM.

## Explicit test ISO profile

The earlier 2026-11-23 Fedora 45 x86_64 test ISO explicitly disabled SELinux and
retained the maintainer's DDM 0.5.0-5. This legacy exception is not used by the
current release recipe. Its configuration RPM installed this file in the original
read-only payload, not only in the running Live overlay:

`/etc/fedora-dde-installer/install-profile.json`

```json
{"schema":1,"selinux":"disabled","pam":"ddm-system-login"}
```

The file and its parent directories must be root-owned, not writable by group
or other users, and not symlinks; mode 0644 is suitable for the file. Unknown
keys, schema versions, policy combinations and duplicate JSON keys are rejected.
An absent file selects `enforcing` with the direct Fedora DDM PAM stack.
Environment variables and Live kernel arguments cannot select the exception.
The selected profile is saved with the source identity and rechecked before
partitioning/copying. Later stages use that saved selection after source unmount.
Copying excludes all installer runtime settings, then writes only the verified
profile as normalized JSON with mode 0644 into the target. This preserves the
ISO RPM's configuration file without importing passwords or trusting a target
file to select policy. Keep that configuration RPM installed: it also owns the
PAM bridge required for normal DDM login.

The payload must have `SELINUX=disabled` in `/etc/selinux/config`; the ISO's Live
kernel command line must include `selinux=0`. The installer preserves ACLs and
file capabilities while excluding the `security.selinux` xattr, skips both
restorecon passes, and writes `selinux=0` to the installed normal and rescue BLS
entries and `/etc/kernel/cmdline`. SELinux utilities/policy RPMs remain installed
so the default enforcing profile retains its dependencies.

The ISO configuration RPM owns only the additional `/etc/pam.d/system-login`
bridge below. DDM's `/etc/pam.d/ddm` and `/etc/pam.d/ddm-autologin` stay unchanged.
The installer checks the exact effective rows, permitting comments/whitespace:

```pam
auth substack password-auth
auth include postlogin
account required pam_nologin.so
account include password-auth
password include password-auth
session required pam_selinux.so close
session required pam_loginuid.so
session required pam_selinux.so open
session optional pam_keyinit.so force revoke
session required pam_namespace.so
session include password-auth
session include postlogin
```

Fedora's `password-auth` and `postlogin` must resolve within the payload. Validate
the generated authselect configuration and real password authentication when
composing the image; file-presence tests alone are not a login test. The Live
session uses a manual DDM login with `liveuser` / `liveuser`. Do not configure
DDM 5 autologin: its `@include` syntax is incompatible with Fedora PAM.
Installed users receive their chosen nonempty password and `wheel` membership;
the backend rejects membership in DDM's `nopasswdlogin` group. Finalization
disables the ISO's `fedora-dde-live.service` and removes Live account settings.

Test both profiles: the default must still reject an inactive SELinux LSM or
unlabelled payload; the opt-in must reject malformed/untrusted profiles, changed
source policy, incomplete/altered PAM bridges and mismatched target SELinux
configuration. Verify the ISO and installed system with disposable VM disks;
the mock tests do not prove DDM login or target boot.

Earlier dependency failures were caused by unpublished repository metadata:
the COPR project used `devel_mode`, and its Gui/Widget builds completed after
the previous production publication. The successful production repository
[regeneration](https://copr.fedorainfracloud.org/coprs/mozixun/Fedora-DDE-Remix/action/2939948/)
at 15:38 UTC made those SDKs available to ordinary DNF clients. When COPR's
[manual repository mode](https://docs.copr.fedorainfracloud.org/user_documentation.html#creating-repositories-manually)
is enabled, use **Regenerate Repositories** after the desired package set is
ready; a successful package build alone does not publish it to that repository.

For developer testing, the optional CI helper
`.github/scripts/bootstrap-dtk-widget.sh` can build the Widget SDK in the
temporary container. It runs only when a workflow is manually dispatched with
`bootstrap_widget=true`; the default is false, and normal pushes never build
this substitute SDK. Its inputs are fixed:

| Input | Pinned value |
|---|---|
| Desktop recipe repository commit | `7b32215a3ada9664e52382247be9a5823bcc8617` |
| Recipe | `dtkwidget/dtk6widget.spec`, unchanged |
| Recipe SHA-256 | `a6fbab31190916d15293dbe728590a876afc88a320706edac1ff41ebc2f0c0c4` |
| Upstream archive | `linuxdeepin/dtkwidget`, tag `6.7.54` |
| Archive SHA-256 | `299c64f46c20f078a055f4b69242a9750c84be4558e1dce3af2aa53a66fcaead` |

The helper refreshes repository metadata and requires the existing Gui and Log
development RPMs before building Widget. It uses normal `dnf builddep`,
`rpmbuild` and RPM dependency checks. The runtime/development SDK RPMs, SRPM,
recipe, input checksums and build log are retained in a separate Actions
artifact. Nothing is uploaded to COPR or another package repository. For an
ISO test, include the matching runtime SDK RPMs along with the installer RPM;
an Actions artifact is not itself an enabled software repository.

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
