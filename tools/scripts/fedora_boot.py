"""Deploy kernels and GRUB through the Fedora payload's own installation hooks.

The caller verifies the mounted installation before entering this module.  Keep
all commands inside that target except RPM file queries and firmware access.
The reference is Anaconda 45.30 and Fedora 45's grub2/dracut packaging.
"""

from __future__ import annotations

import hashlib
from pathlib import Path
import re
import shlex
import shutil
import stat

from fedora_backend import DEVICE_RE, InstallError, require


# data/anaconda.conf [Bootloader], with security policy handled separately below.
PRESERVED_ARGUMENTS = frozenset("""
cio_ignore zfcp.allow_lun_scan speakup_synth apic noapic apm ide noht acpi
video pci nodmraid nompath nomodeset noiswmd biosdevname ipv6.disable
net.ifnames net.ifnames.prefix nosmt vga rd.net.dns rd.net.dns-resolve-mode
rd.net.dns-backend console clk_ignore_unused pd_ignore_unused arm64.nopauth
systemd.tpm2_wait
""".split())
KERNEL_VERSION = re.compile(r"[A-Za-z0-9._+-]+\Z")
UUID = re.compile(r"[A-Fa-f0-9-]+\Z")
MACHINE_ID = re.compile(r"[a-f0-9]{32}\Z")
MODULE_NAME = re.compile(r"[A-Za-z0-9_-]+\Z")


def verify_kernel_hooks(root: Path) -> None:
    """Fail before partitioning if the payload cannot create Fedora GRUB entries.

    KIWI's UKI-only compose can mask GRUB/dracut hooks with empty files.  Such an
    image is not a conventional Live installation payload even when RPM lists
    the required packages. Runtime artifact checks still verify the resulting
    rescue entry after all target dracut configuration has been applied.
    """
    def checked_file(path: Path) -> Path:
        require(path.is_file() and path.resolve().is_relative_to(root.resolve()) and path.stat().st_size > 0,
                f"The Live payload is missing an active Fedora boot configuration: {path.relative_to(root)}.")
        return path

    for name in ("20-grub.install", "50-dracut.install", "51-dracut-rescue.install"):
        packaged = checked_file(root / "usr/lib/kernel/install.d" / name)
        override = root / "etc/kernel/install.d" / name
        effective = checked_file(override) if override.exists() or override.is_symlink() else packaged
        require(all(path.stat().st_mode & (stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH) for path in {packaged, effective}),
                f"The Fedora kernel installation hook {name} is not executable.")
    rescue = checked_file(root / "usr/lib/dracut/dracut.conf.d/02-rescue.conf")
    override = root / "etc/dracut.conf.d/02-rescue.conf"
    if override.exists() or override.is_symlink():
        rescue = checked_file(override)
    require(re.search(r'''(?m)^\s*dracut_rescue_image\s*=\s*(["']?)yes\1\s*(?:#.*)?$''', rescue.read_text()),
            "The Live payload must enable dracut-config-rescue's rescue image generation.")


def firmware_suffix(architecture: str, firmware_bits: str | None = None) -> str:
    """The EFI architecture follows the firmware, including mixed-mode x86."""
    require(architecture in {"x86_64", "aarch64"}, "Unsupported EFI architecture.")
    if firmware_bits is None:
        platform_size = Path("/sys/firmware/efi/fw_platform_size")
        firmware_bits = platform_size.read_text().strip() if platform_size.is_file() else "64"
    require(str(firmware_bits) in {"32", "64"}, "Cannot identify the UEFI firmware architecture.")
    require(architecture != "aarch64" or str(firmware_bits) == "64", "aarch64 requires 64-bit UEFI.")
    return "aa64" if architecture == "aarch64" else ("ia32" if str(firmware_bits) == "32" else "x64")


def _directory(backend, relative: str) -> Path:
    path = backend.path(relative)
    require(not path.is_symlink(), f"The target directory {relative} must not be a symbolic link.")
    path.mkdir(parents=True, exist_ok=True)
    require(path.resolve().is_relative_to(backend.target.resolve()), "A boot directory escapes the target.")
    return path


def _file(backend, relative: str) -> Path:
    path = backend.path(relative)
    require(path.is_file() and path.resolve().is_relative_to(backend.target.resolve())
            and path.stat().st_size > 0, f"The target boot file {relative} is missing or invalid.")
    return path


def _uuid(mapping: dict) -> str:
    identifier = mapping.get("uuid", "")
    require(isinstance(identifier, str) and UUID.fullmatch(identifier), "A boot filesystem UUID is invalid.")
    return identifier


def kernel_arguments(backend) -> list[str]:
    """Rebuild storage arguments; only explicit hardware options survive Live."""
    from fedora_profile import selinux_disabled
    from fedora_crypto import root_record

    if "live_kernel_cmdline" not in backend.state:
        backend.state["live_kernel_cmdline"] = Path("/proc/cmdline").read_text().strip()
        backend.save()
    commandline = backend.state["live_kernel_cmdline"]
    require(isinstance(commandline, str) and not any(c in commandline for c in "\0\r\n"),
            "The saved Live kernel command line is invalid.")
    try:
        source_arguments = shlex.split(commandline)
    except ValueError as error:
        raise InstallError("The Live kernel command line cannot be parsed.") from error
    require("fips=1" not in source_arguments,
            "FIPS installation is not implemented; boot this Live installer without fips=1.")
    root = next(item for item in backend.state["mounts"] if item["mountpoint"] == "/")
    arguments = [f"root=UUID={_uuid(root)}", "ro"]
    encrypted = root_record(backend.state)
    if encrypted:
        require(not selinux_disabled(backend.state), "Encrypted boot requires Fedora SELinux Enforcing.")
        arguments.append("rd.luks.uuid=luks-" + encrypted["uuid"])
    if root.get("fstype") == "btrfs":
        subvolumes = [option for option in root.get("options", "").split(",")
                      if option.startswith(("subvol=", "subvolid="))]
        require(len(subvolumes) == 1 and re.fullmatch(r"subvol=[A-Za-z0-9_./-]+|subvolid=[0-9]+", subvolumes[0])
                and ".." not in Path(subvolumes[0].split("=", 1)[1]).parts,
                "The Btrfs root subvolume is missing or invalid.")
        arguments.append("rootflags=" + subvolumes[0])
    swaps = [item for item in backend.state["mounts"] if item["mountpoint"] == "swap"]
    if swaps:
        # Anaconda chooses the largest explicit disk swap. Zram is not a resume device.
        if len(swaps) > 1:
            sizes = []
            for swap in swaps:
                size = backend.run("lsblk", "--bytes", "--nodeps", "--noheadings", "--output", "SIZE", swap["device"]).stdout.strip()
                require(size.isdecimal() and int(size) > 0, "Cannot determine the resume swap size.")
                sizes.append((int(size), swap))
            resume = max(sizes, key=lambda item: item[0])[1]
        else:
            resume = swaps[0]
        arguments.append(f"resume=UUID={_uuid(resume)}")
    blacklist = []
    serial_console = False
    for argument in source_arguments:
        key, _, value = argument.partition("=")
        if key == "modprobe.blacklist":
            for module in value.split(","):
                require(MODULE_NAME.fullmatch(module), "An inherited module blacklist is invalid.")
                if module not in blacklist:
                    blacklist.append(module)
        elif key in PRESERVED_ARGUMENTS:
            # These values also enter BLS files and shell-sourced GRUB defaults.
            # Hardware parameters need no whitespace or shell metacharacters.
            require(re.fullmatch(r"[A-Za-z0-9_./,:=@%+\[\]-]+", argument),
                    f"Unsupported characters in the inherited {key} kernel parameter.")
            if argument not in arguments:
                arguments.append(argument)
            if key == "console" and not re.fullmatch(r"tty[0-9]+(?:,.*)?", value):
                serial_console = True
    # Only the explicit, validated image profile can disable target SELinux.
    # Live-only selinux/enforcing arguments never select installation policy.
    if selinux_disabled(backend.state):
        arguments.append("selinux=0")
    if blacklist:
        arguments.append("modprobe.blacklist=" + ",".join(blacklist))
    backend.write("etc/modprobe.d/anaconda-denylist.conf", "".join(f"blacklist {module}\n" for module in blacklist))
    graphical = backend.run("rpm", "--root", str(backend.target), "-q", "--whatprovides", "plymouth", "rhgb", check=False)
    if not serial_console and any(not line.startswith("no package provides ") for line in graphical.stdout.splitlines()):
        arguments.extend(("rhgb", "quiet"))
    return arguments


def _machine_id(backend) -> str:
    machine_id_path = backend.path("etc/machine-id")
    dbus_id = backend.path("var/lib/dbus/machine-id")
    machine_id = backend.state.get("machine_id")
    if machine_id:
        require(isinstance(machine_id, str) and MACHINE_ID.fullmatch(machine_id), "The installation machine ID is invalid.")
        require(machine_id_path.is_file() and not machine_id_path.is_symlink()
                and machine_id_path.read_text().strip() == machine_id,
                "The target machine ID changed; refusing to replace an installation identity.")
    else:
        # Do not let systemd import the copied Live D-Bus or machine ID.
        for path in (machine_id_path, dbus_id):
            if path.exists() or path.is_symlink():
                require(not path.is_dir() or path.is_symlink(), "A machine ID path is a directory.")
                path.unlink()
        backend.chroot("systemd-machine-id-setup")
        machine_id = _file(backend, "etc/machine-id").read_text().strip()
        require(MACHINE_ID.fullmatch(machine_id) and machine_id != "0" * 32, "systemd did not create a valid machine ID.")
        backend.state["machine_id"] = machine_id
        backend.save()  # All later retries keep the ID used by the kernel hooks.
    backend.write("etc/kernel/entry-token", machine_id + "\n")
    dbus_id.parent.mkdir(parents=True, exist_ok=True)
    if dbus_id.exists() or dbus_id.is_symlink():
        dbus_id.unlink()
    dbus_id.symlink_to("/etc/machine-id")
    return machine_id


def _kernel_images(backend) -> list[tuple[str, Path]]:
    modules = _directory(backend, "usr/lib/modules")
    images = []
    for directory in modules.iterdir():
        if not directory.is_dir():
            continue
        version = directory.name
        require(KERNEL_VERSION.fullmatch(version), "Invalid installed kernel version.")
        found = [backend.path(f"usr/lib/modules/{version}/{name}")
                 for name in ("vmlinuz", "vmlinuz-dtbloader.efi")
                 if backend.path(f"usr/lib/modules/{version}/{name}").is_file()]
        # Older Fedora payloads may store the RPM-owned kernel only under /boot.
        if not found and backend.path(f"boot/vmlinuz-{version}").is_file():
            found = [backend.path(f"boot/vmlinuz-{version}")]
        require(found, f"Kernel image {version} is missing from the payload.")
        for source in found:
            _file(backend, str(source.relative_to(backend.target)))
            images.append((version, source))
    require(images, "No installed kernel was found.")
    ordered = backend.run("sort", "--version-sort", input_text="\n".join(sorted({version for version, _ in images})) + "\n").stdout.splitlines()
    require(len(ordered) == len(set(ordered)) and set(ordered) == {version for version, _ in images},
            "Cannot determine the installed kernel order.")
    return sorted(images, key=lambda item: ordered.index(item[0]), reverse=True)


def _verify_bls(backend, relative: str, expected_arguments: list[str]) -> None:
    entry = _file(backend, relative)
    fields = {}
    for line in entry.read_text().splitlines():
        parts = line.split(None, 1)
        if len(parts) == 2:
            fields[parts[0]] = parts[1]
    require("linux" in fields and "initrd" in fields and "options" in fields,
            "Fedora's kernel hooks did not create a complete BLS entry.")
    require(fields["options"].split() == expected_arguments,
            "The generated BLS entry does not contain the installed root and kernel parameters.")
    for key in ("linux", "initrd"):
        for name in fields[key].split():
            require(name.startswith("/") and ".." not in Path(name).parts, "A BLS boot path is invalid.")
            _file(backend, "boot/" + name.lstrip("/"))


def install_kernels(backend) -> None:
    from fedora_crypto import crypttab, root_record, verify_initramfs
    from fedora_profile import selinux_disabled
    from fedora_system import verify_selinux_config

    if selinux_disabled(backend.state):
        verify_selinux_config(backend.target, "disabled")
    encrypted = root_record(backend.state)
    if encrypted:
        verify_selinux_config(backend.target, "enforcing")
        require(backend.path("etc/crypttab").is_file()
                and backend.path("etc/crypttab").read_text() == crypttab(backend.state),
                "Write the password-only encrypted root crypttab before generating initramfs.")
    arguments = kernel_arguments(backend)
    options = " ".join(arguments)
    # Write the defaults before cmdline: Fedora's 20-grub.install compares their mtimes.
    backend.write("etc/default/grub", "GRUB_TIMEOUT=5\nGRUB_DISTRIBUTOR='Fedora DDE Remix'\n"
                  "GRUB_DEFAULT=saved\nGRUB_DISABLE_SUBMENU=true\nGRUB_TERMINAL_OUTPUT=console\n"
                  "GRUB_DISABLE_RECOVERY=true\nGRUB_ENABLE_BLSCFG=true\n"
                  + "GRUB_CMDLINE_LINUX=" + shlex.quote(options) + "\n")
    backend.write("etc/kernel/cmdline", options + "\n")
    # Use the target Fedora hostonly defaults. Only omit modules used by the Live medium.
    backend.write("etc/dracut.conf.d/90-fedora-dde-installer.conf",
                  'omit_dracutmodules+=" dmsquash-live livenet "\n'
                  + ('add_dracutmodules+=" crypt systemd-cryptsetup "\ninstall_items+=" /etc/crypttab "\n' if encrypted else ""))
    machine_id = _machine_id(backend)
    images = _kernel_images(backend)
    default_version, default_image = next((item for item in images if "+debug" not in item[0]), images[0])
    owner = backend.run("rpm", "--root", str(backend.target), "-qf", "--qf", "%{NAME}\n",
                        "/" + str(default_image.relative_to(backend.target))).stdout.strip()
    require(re.fullmatch(r"kernel[A-Za-z0-9_.+-]*", owner), "Cannot identify the RPM providing the default kernel.")
    backend.write("etc/sysconfig/kernel", f"UPDATEDEFAULT=yes\nDEFAULTKERNEL={owner}\n")
    entries = _directory(backend, "boot/loader/entries")
    for path in entries.glob("*.conf"):
        path.unlink()  # /boot is a newly formatted, verified installation filesystem.
    for pattern in ("vmlinuz-0-rescue-*", "initramfs-0-rescue-*.img"):
        for path in backend.path("boot").glob(pattern):
            path.unlink()
    _directory(backend, "boot/grub2")
    backend.chroot("grub2-editenv", "/boot/grub2/grubenv", "create")
    backend.chroot("grub2-editenv", "/boot/grub2/grubenv", "set", "blsdir=/loader/entries", "kernelopts=" + options)
    installed = []
    for version, source in images:
        backend.chroot("depmod", "-a", version)
        if source.name == "vmlinuz-dtbloader.efi":
            # A signed dtbloader can be detected as an EFI image by the dracut
            # hook. Provide its external initrd before Fedora's 95-set-boot-entry check.
            backend.chroot("dracut", "--force", f"/boot/initramfs-{version}.img", version, capture=False)
        backend.chroot("kernel-install", "add", version, "/" + str(source.relative_to(backend.target)), capture=False)
        # Match Anaconda's final recreation after configuration. This also avoids
        # keeping a pre-generated module-directory initrd copied by the hook.
        backend.chroot("dracut", "--force", f"/boot/initramfs-{version}.img", version, capture=False)
        _file(backend, f"boot/vmlinuz-{version}")
        _file(backend, f"boot/initramfs-{version}.img")
        _verify_bls(backend, f"boot/loader/entries/{machine_id}-{version}.conf", arguments)
        verify_initramfs(backend, f"boot/initramfs-{version}.img")
        if version not in installed:
            installed.append(version)
    # dracut-config-rescue's 51 hook owns rescue creation and its BLS entry.
    # A zero exit code without those files must never count as an installed system.
    _verify_bls(backend, f"boot/loader/entries/{machine_id}-0-rescue.conf", arguments)
    verify_initramfs(backend, f"boot/initramfs-0-rescue-{machine_id}.img")
    backend.chroot("grubby", "--set-default", f"/boot/vmlinuz-{default_version}")
    backend.state.update(kernels=installed, kernel_options=options, default_kernel=default_version)
    backend.save()


def efi_file(backend, package: str, name: str, root: Path | None = None) -> Path:
    root = root or backend.target
    require(re.fullmatch(r"[A-Za-z0-9_.+-]+", package) and re.fullmatch(r"[A-Za-z0-9_.-]+", name),
            "Invalid EFI package or file name.")
    paths = backend.run("rpm", "--root", str(root), "-ql", package).stdout.splitlines()
    matches = []
    for name_in_package in paths:
        path = root / name_in_package.lstrip("/")
        if (name_in_package.startswith("/") and name_in_package.endswith("/" + name)
                and ".." not in Path(name_in_package).parts and path.is_file()
                and path.resolve().is_relative_to(root.resolve()) and path.stat().st_size > 0):
            matches.append(path)
    require(matches, f"The Live image must include {package} with {name}.")
    # Prefer the package-owned signed source over an existing ESP copy.
    return sorted(matches, key=lambda path: ("/usr/lib/efi/" not in str(path), str(path)))[0]


def _copy_efi(source: Path, destination: Path) -> None:
    if source.resolve() == destination.resolve():
        return
    if destination.is_symlink():
        destination.unlink()
    require(not destination.exists() or destination.is_file(), "An EFI destination is not a regular file.")
    shutil.copyfile(source, destination)


def _digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _fallback(backend, directory: Path, files: dict[str, Path]) -> bool:
    expected = {name: _digest(source) for name, source in files.items()}
    recorded = backend.state.get("fallback_efi_hashes", {})
    pending = backend.state.get("fallback_efi_installing", False)
    if not recorded:
        # Even an incomplete other OS fallback is its property; do not overwrite
        # its grub.cfg or companion binaries merely because BOOT*.EFI is absent.
        if any((directory / name).exists() or (directory / name).is_symlink() for name in files):
            return False
        backend.state["fallback_efi_hashes"] = expected
        backend.state["fallback_efi_installing"] = True
        backend.save()  # A retry can safely complete a partially copied fallback.
        recorded, pending = expected, True
    if recorded != expected:
        return False
    for name, digest in recorded.items():
        path = directory / name
        if path.is_symlink() or (path.exists() and (not path.is_file() or _digest(path) != digest)):
            return False
        if not path.exists() and not pending:
            return False
    if pending:
        for name, source in files.items():
            _copy_efi(source, directory / name)
        backend.state["fallback_efi_installing"] = False
        backend.save()
    return all((directory / name).is_file() and _digest(directory / name) == digest for name, digest in expected.items())


def install_bootloader(backend) -> None:
    require(backend.state.get("kernels") and backend.state.get("kernel_options"),
            "Install and verify the target kernels before writing the bootloader.")
    _directory(backend, "boot/grub2")
    if not backend.path("boot/grub2/grubenv").is_file():
        backend.chroot("grub2-editenv", "/boot/grub2/grubenv", "create")
    # Do not recreate grubenv here: the Fedora hooks have selected saved_entry.
    backend.chroot("grub2-editenv", "/boot/grub2/grubenv", "set", "blsdir=/loader/entries",
                   "kernelopts=" + backend.state["kernel_options"])
    backend.chroot("grub2-mkconfig", "--output=/boot/grub2/grub.cfg", capture=False)
    _file(backend, "boot/grub2/grub.cfg")
    if backend.state["uefi"]:
        suffix = backend.state.get("efi_suffix") or firmware_suffix(backend.state["architecture"])
        require(suffix in ({"x64", "ia32"} if backend.state["architecture"] == "x86_64" else {"aa64"}),
                "The saved firmware architecture does not match the installation.")
        shim = efi_file(backend, f"shim-{suffix}", f"shim{suffix}.efi")
        grub = efi_file(backend, f"grub2-efi-{suffix}", f"grub{suffix}.efi")
        vendor = _directory(backend, "boot/efi/EFI/fedora")
        fallback = _directory(backend, "boot/efi/EFI/BOOT")
        _copy_efi(shim, vendor / f"shim{suffix}.efi")
        _copy_efi(grub, vendor / f"grub{suffix}.efi")
        files = {f"BOOT{suffix.upper()}.EFI": shim, f"grub{suffix}.efi": grub}
        try:
            manager = efi_file(backend, f"shim-{suffix}", f"mm{suffix}.efi")
        except InstallError:
            manager = None  # MokManager is optional in the signed Fedora boot chain.
        if manager is not None:
            _copy_efi(manager, vendor / manager.name)
            files[manager.name] = manager
        stub = vendor / "grub.cfg"
        if stub.is_symlink():
            stub.unlink()
        backend.chroot("gen_grub_cfgstub", "/boot/grub2", "/boot/efi/EFI/fedora")
        files["grub.cfg"] = _file(backend, "boot/efi/EFI/fedora/grub.cfg")
        fallback_owned = _fallback(backend, fallback, files)
        esp = next(item for item in backend.state["mounts"] if item["mountpoint"] == "/boot/efi")
        disk = backend.run("lsblk", "-nro", "PKNAME", "--paths", esp["device"]).stdout.strip()
        properties = backend.run("udevadm", "info", "--query=property", "--name", esp["device"]).stdout
        number = next((line.split("=", 1)[1] for line in properties.splitlines() if line.startswith("ID_PART_ENTRY_NUMBER=")), "")
        require(DEVICE_RE.fullmatch(disk) and ".." not in Path(disk).parts and number.isdecimal() and int(number) > 0,
                "Cannot identify the EFI partition.")
        if not backend.state.get("firmware_entry_created"):
            result = backend.run("efibootmgr", "--create", "--disk", disk, "--part", number,
                                 "--label", "Fedora DDE Remix", "--loader", f"\\EFI\\fedora\\shim{suffix}.efi", check=False)
            if result.returncode:
                require(fallback_owned, "Firmware entry creation failed and another system owns or changed the UEFI fallback path.")
                print("Firmware entry could not be written; the verified UEFI fallback loader is installed.", flush=True)
                backend.state["firmware_fallback"] = True
            else:
                backend.state["firmware_entry_created"] = True
                backend.state.pop("firmware_fallback", None)
    else:
        boot_disk = backend.config.get("DI_BOOT_DISK")
        if not boot_disk:
            require(len(backend.state["devices"]) == 1, "Select DI_BOOT_DISK explicitly for a multi-disk BIOS installation.")
            boot_disk = backend.state["devices"][0]
        require(boot_disk in backend.state["devices"] and DEVICE_RE.fullmatch(boot_disk) and ".." not in Path(boot_disk).parts,
                "The BIOS boot disk is not part of the confirmed installation plan.")
        backend.chroot("grub2-install", "--target=i386-pc", "--recheck", boot_disk, capture=False)
    backend.save()
