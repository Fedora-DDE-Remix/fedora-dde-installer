#!/usr/bin/python3
"""Install a conventional Fedora Live system, then configure it in the same session.

The GUI still drives ordered .job hooks.  Every hook runs in the Live environment;
only explicitly listed commands enter the target.  No command reboots the machine.
State lives in /run, is tied to this boot and to the target mount identity, and is
retained on failure so a failed account step never becomes a successful install.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import uuid


APP = "fedora-dde-installer"
TARGET = Path("/target")
STATE_DIR = Path("/run") / APP
CONFIG_TOOL = f"/usr/bin/{APP}-config"
PARTITION_TOOL = f"/usr/bin/{APP}-parted"
ALLOWED_MODES = {"0", "1", "6"}
ALLOWED_MOUNTS = {"/", "/boot", "/boot/efi", "/home", "/tmp", "/var", "/srv", "/opt", "/usr/local", "swap"}
ROOT_FILESYSTEMS = {"ext4", "xfs", "btrfs"}
DEVICE_RE = re.compile(r"/dev/[A-Za-z0-9_./:+-]+\Z")
SECRET_KEYS = ("DI_PASSWORD", "DI_ROOT_PASSWORD", "DI_CRYPT_PASSWORD")


class InstallError(RuntimeError):
    """A recoverable installer failure, displayed by the existing progress UI."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise InstallError(message)


def parse_mounts(value: str) -> list[dict[str, str]]:
    """Parse the existing partition helper's device=mount[options] protocol."""
    result = []
    seen = set()
    for entry in filter(None, value.split(";")):
        match = re.fullmatch(r"([^=]+)=([^\[]+)(?:\[([^\]]+)\])?", entry)
        require(match is not None, "The partition helper returned an invalid mount entry.")
        device, mountpoint, options = match.groups()
        require(bool(DEVICE_RE.fullmatch(device)) and ".." not in Path(device).parts,
                "A target device path is invalid.")
        require(mountpoint in ALLOWED_MOUNTS, f"Unsupported target mount point: {mountpoint}")
        require(mountpoint == "swap" or mountpoint not in seen,
                f"Duplicate target mount point: {mountpoint}")
        require(options in (None, "compress=zstd"), "Unsupported filesystem mount options.")
        seen.add(mountpoint)
        result.append({"device": device, "mountpoint": mountpoint, "options": options or "defaults"})
    require("/" in seen, "The partition plan does not contain a root filesystem.")
    return sorted(result, key=lambda item: (item["mountpoint"] != "/", item["mountpoint"].count("/"), item["mountpoint"]))


def validate_plan(plan: object) -> list[str]:
    require(isinstance(plan, list) and bool(plan), "The partition plan is empty or invalid.")
    devices, roots = set(), []
    for item in plan:
        require(isinstance(item, dict), "A partition plan entry is invalid.")
        require(item.get("operate") in {"new", "edit", "delete", "normal"}, "Invalid partition operation.")
        if item.get("operate") == "normal":
            continue  # Untouched devices are present in the GUI's complete plan.
        require(item.get("type") in {"disk", "partition"},
                "This release supports ordinary partitions; LVM is not enabled.")
        require(not any(item.get(key) for key in ("isCrypt", "isCryptHeader", "isNeedCryptHeader", "cryptPassword")),
                "Encrypted installation is not supported by this Fedora backend.")
        device = item.get("device", "")
        require(isinstance(device, str) and bool(DEVICE_RE.fullmatch(device)) and ".." not in Path(device).parts,
                "A partition plan device is invalid.")
        devices.add(device)
        if item.get("type") == "disk":
            require(item.get("diskType") in {"gpt", "msdos", "dos"}, "Unsupported partition table type.")
        else:
            path = item.get("path", "")
            require(not path or bool(DEVICE_RE.fullmatch(path)) and ".." not in Path(path).parts,
                    "A partition device path is invalid.")
            require(bool(re.fullmatch(r"[A-Za-z0-9_.-]{0,32}", item.get("label", ""))),
                    "Partition labels must use at most 32 letters, digits, dots, underscores or hyphens.")
            require(item.get("filesystem", "") in ROOT_FILESYSTEMS | {"", "vfat", "fat32", "efi", "ext2", "ext3", "swap", "linux-swap"},
                    "The partition filesystem is unsupported.")
        if item.get("type") != "partition" or item.get("operate") in {"delete", "normal"}:
            continue
        point = item.get("mountPoint", "")
        require(point == "" or point in ALLOWED_MOUNTS, f"Unsupported target mount point: {point}")
        require(item.get("filesystem", "") not in {"crypto_luks", "lvm2 pv"},
                "Encrypted and LVM partition payloads are not supported.")
        if point == "/":
            roots.append(item)
            require(item.get("filesystem") in ROOT_FILESYSTEMS,
                    "The root filesystem must be ext4, XFS or Btrfs.")
            require(item.get("operate") == "new" or item.get("isneedformat") is True,
                    "The target root must be newly created or explicitly selected for formatting.")
    require(len(roots) == 1, "Select exactly one target root filesystem.")
    return sorted(devices)


def normalized_locale(value: str) -> str:
    require(bool(re.fullmatch(r"[a-z]{2,3}_[A-Z]{2}(?:\.(?:UTF-8|utf8))?(?:@[A-Za-z0-9_-]+)?", value)),
            "Select a valid UTF-8 system locale.")
    base, separator, modifier = value.partition("@")
    base = base.split(".")[0] + ".UTF-8"
    return base + (separator + modifier if separator else "")


def validate_account(values: dict[str, str]) -> dict[str, str]:
    user = values.get("DI_USERNAME", "")
    require(bool(re.fullmatch(r"[a-z_][a-z0-9_-]{0,31}", user)), "The username is invalid.")
    require(user not in {"root", "dde", "nobody", "daemon", "bin", "systemd-network"},
            "Choose a regular user account name.")
    password = values.get("DI_PASSWORD", "")
    require(bool(password) and not any(char in password for char in "\r\n\0"),
            "A non-empty single-line password is required.")
    require(values.get("DI_PASSWORD_ENCRYPTION_CONFIG", "false").lower() != "true",
            "The obsolete password encoding format is not accepted.")
    hostname = values.get("DI_HOSTNAME", "") or f"{user}-pc"
    require(len(hostname) <= 253 and all(re.fullmatch(r"[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?", label)
                                       for label in hostname.split(".")), "The hostname is invalid.")
    timezone = values.get("DI_TIMEZONE", "UTC") or "UTC"
    if timezone in {"Asia/Beijing", "Asia/Chengdu", "Asia/Nanjing", "Asia/Wuhan", "Asia/Xian"}:
        timezone = "Asia/Shanghai"
    require(not timezone.startswith("/") and ".." not in Path(timezone).parts
            and bool(re.fullmatch(r"[A-Za-z0-9_+/-]+", timezone)), "The timezone is invalid.")
    layout = values.get("DI_LAYOUT", "us") or "us"
    variant = values.get("DI_LAYOUT_VARIANT", "")
    require(bool(re.fullmatch(r"[a-z0-9_-]+(?:,[a-z0-9_-]+)*", layout))
            and bool(re.fullmatch(r"[a-zA-Z0-9_,()-]*", variant)), "The keyboard layout is invalid.")
    return {"user": user, "password": password, "hostname": hostname,
            "locale": normalized_locale(values.get("DI_LOCALE", "en_US")),
            "region_locale": normalized_locale(values.get("DI_REGION_FORMAT_LOCALE_SET")
                                                or values.get("DI_REGION_FORMAT_LOCALE")
                                                or values.get("DI_LOCALE", "en_US")),
            "timezone": timezone, "layout": layout, "variant": variant}


class Runner:
    def run(self, argv: list[str], *, input_text: str | None = None, check: bool = True,
            capture: bool = True) -> subprocess.CompletedProcess:
        # Never log input_text: chpasswd receives the password only through stdin.
        env = dict(os.environ, LC_ALL="C", LANG="C", SYSTEMD_OFFLINE="1",
                   PATH="/usr/sbin:/usr/bin:/sbin:/bin")
        for key in ("LD_PRELOAD", "LD_LIBRARY_PATH", "PYTHONPATH", "PYTHONHOME", "BASH_ENV", "ENV"):
            env.pop(key, None)
        result = subprocess.run(argv, input=input_text, text=True, encoding="utf-8", env=env,
                                stdout=subprocess.PIPE if capture else None,
                                stderr=subprocess.PIPE if capture else None, check=False)
        if check and result.returncode:
            # A password helper's diagnostics are deliberately not relayed.
            detail = "" if input_text is not None else (result.stderr or "").strip()[-1500:]
            raise InstallError(f"{Path(argv[0]).name} failed (exit {result.returncode}). {detail}".strip())
        return result


class Config:
    def __init__(self, path: Path, runner: Runner):
        self.path, self.runner = path, runner

    def get(self, key: str, default: str = "") -> str:
        result = self.runner.run([CONFIG_TOOL, "get", str(self.path), "", key])
        return result.stdout.rstrip("\r\n") or default

    def set(self, key: str, value: str) -> None:
        require(key not in SECRET_KEYS or value == "", "Secrets must not be passed in command arguments.")
        self.runner.run([CONFIG_TOOL, "set", str(self.path), "", key, value])

    def require_private(self) -> None:
        require(self.path.is_file() and not self.path.is_symlink(), "The installer configuration is missing.")
        mode = self.path.stat()
        require(mode.st_uid == 0 and stat.S_IMODE(mode.st_mode) & 0o077 == 0,
                "The installer configuration must be owned by root with mode 0600.")
        parent = self.path.parent
        require(parent.is_dir() and not parent.is_symlink() and parent.stat().st_uid == 0
                and stat.S_IMODE(parent.stat().st_mode) & 0o077 == 0,
                "The installer runtime directory must be owned by root with mode 0700.")


class Backend:
    def __init__(self, config: Config, runner: Runner, target: Path = TARGET,
                 state_dir: Path = STATE_DIR, boot_id: str | None = None):
        self.config, self.runner = config, runner
        self.target, self.state_dir = target, state_dir
        self.state_file = state_dir / "state.json"
        self.boot_id = boot_id or Path("/proc/sys/kernel/random/boot_id").read_text().strip()
        self.state: dict = {}

    def run(self, *args: str, **kwargs) -> subprocess.CompletedProcess:
        return self.runner.run(list(args), **kwargs)

    def chroot(self, *args: str, **kwargs) -> subprocess.CompletedProcess:
        return self.run("chroot", str(self.target), *args, **kwargs)

    def load(self, *phases: str) -> None:
        require(self.state_file.is_file() and not self.state_file.is_symlink(),
                "No installation from this Live session is available.")
        self.state = json.loads(self.state_file.read_text())
        require(self.state.get("boot_id") == self.boot_id and self.state.get("target") == str(self.target),
                "The installation target belongs to a different session.")
        if phases:
            require(self.state.get("phase") in phases,
                    f"Cannot run this step while installation is {self.state.get('phase', 'unknown')}.")

    def save(self, phase: str | None = None) -> None:
        if phase:
            self.state["phase"] = phase
        self.state_dir.mkdir(mode=0o700, parents=True, exist_ok=True)
        require(not self.state_dir.is_symlink(), "The state directory must not be a symlink.")
        os.chmod(self.state_dir, 0o700)
        self.atomic(self.state_file, json.dumps(self.state, indent=2) + "\n", 0o600)

    @staticmethod
    def atomic(path: Path, text: str, mode: int | None = 0o644) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        fd, temporary = tempfile.mkstemp(prefix=".installer-", dir=path.parent)
        try:
            if mode is not None:
                os.fchmod(fd, mode)
            with os.fdopen(fd, "w") as stream:
                stream.write(text)
                stream.flush()
                os.fsync(stream.fileno())
            os.replace(temporary, path)
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)

    def path(self, relative: str) -> Path:
        rel = Path(relative)
        require(not rel.is_absolute() and ".." not in rel.parts, "Unsafe target path.")
        path = self.target / rel
        require(path.parent.resolve().is_relative_to(self.target.resolve()),
                "A target directory symlink escapes the installation root.")
        return path

    def write(self, relative: str, contents: str, mode: int = 0o644) -> None:
        # FAT does not implement arbitrary POSIX chmod modes; its mount mask is
        # already restrictive. Do not fail a valid ESP write by calling fchmod.
        self.atomic(self.path(relative), contents, None if relative.startswith("boot/efi/") else mode)

    def mount_identity(self, point: Path | None = None) -> dict[str, str]:
        require(not self.target.is_symlink() and self.target.resolve() != Path("/"), "Unsafe installation target.")
        point = point or self.target
        result = self.run("findmnt", "--noheadings", "--raw", "--mountpoint", str(point),
                          "--output", "SOURCE,MAJ:MIN,FSTYPE,UUID,OPTIONS")
        columns = result.stdout.strip().split()
        filesystems = ROOT_FILESYSTEMS if point == self.target else ROOT_FILESYSTEMS | {"ext2", "ext3", "vfat"}
        require(len(columns) == 5 and columns[0].startswith("/dev/") and columns[2] in filesystems
                and bool(re.fullmatch(r"[A-Fa-f0-9-]+", columns[3])) and "rw" in columns[4].split(","),
                "The target must be a mounted writable disk filesystem with a UUID.")
        return dict(zip(("source", "major_minor", "fstype", "uuid"), columns[:4]))

    def verify_target(self, marker: bool = True) -> None:
        require(self.mount_identity() == self.state.get("mount_identity"),
                "The installation target was unmounted or replaced. Refusing to write to it.")
        for point, identity in self.state.get("mount_identities", {}).items():
            if point in self.state["owned_mounts"]:
                require(self.mount_identity(Path(point)) == identity,
                        "An installation filesystem was unmounted or replaced.")
        if marker:
            target_marker = self.path("etc/fedora-dde-installer-target")
            require(target_marker.is_file() and not target_marker.is_symlink()
                    and target_marker.read_text().strip() == self.state["installation_id"],
                    "The target does not match this installation.")

    def progress(self, value: int) -> None:
        self.config.set("DI_INSTALLER_STATUS", str(value))

    def source_check(self, source: Path) -> None:
        require(source.is_absolute() and source.is_dir() and not source.is_symlink(), "Invalid Fedora source root.")
        require(not source.resolve().is_relative_to(self.target.resolve()), "The source is inside the target.")
        os_release = source / "etc/os-release"
        require(os_release.is_file(), "The source has no operating system metadata.")
        values = {}
        for line in os_release.read_text().splitlines():
            if "=" in line:
                key, value = line.split("=", 1)
                values[key] = value.strip('"\'')
        require(values.get("ID") == "fedora", "The source must be a Fedora Live root filesystem.")
        require(not (source / "run/ostree-booted").exists()
                and not (source / "ostree/repo").is_dir(), "Image-based deployment roots are not supported.")
        require(any((source / "usr/lib/modules").glob("*/vmlinuz"))
                or any((source / "boot").glob("vmlinuz-*")), "The source does not contain an installed kernel.")
        if source == Path("/"):
            commandline = Path("/proc/cmdline").read_text().split()
            require(any(item == "rd.live.image" or item.startswith("root=live:") for item in commandline),
                    "Copying / is only allowed while booted from a Fedora Live image.")
            mounted = self.run("findmnt", "--noheadings", "--raw", "--mountpoint", "/",
                               "--output", "SOURCE,FSTYPE").stdout.strip().split()
            require(len(mounted) == 2 and (mounted[1] in {"overlay", "squashfs"}
                    or mounted[0].startswith(("/dev/mapper/live-", "/dev/loop"))),
                    "The running root does not have a Fedora Live filesystem layout.")
        else:
            self.run("findmnt", "--noheadings", "--mountpoint", str(source))
        for path in ("usr/bin/rpm", "usr/bin/dnf", "usr/bin/systemctl", "usr/bin/dracut",
                     "usr/sbin/chpasswd", "usr/sbin/useradd", "usr/bin/grub2-mkconfig", "usr/bin/grub2-editenv"):
            # Fedora uses /usr/sbin for some utilities on older images.
            require((source / path).exists() or (source / "usr/sbin" / Path(path).name).exists(),
                    f"The Live image is missing {Path(path).name}; include it when composing the ISO.")

    def preflight(self) -> None:
        if self.state_file.exists():
            self.load()
            require(self.state.get("phase") in {"preflight", "finalized"}, "An installation is already active in this Live session.")
        source = Path(self.config.get("DI_SOURCE_ROOT", "/"))
        self.source_check(source)
        architecture = platform.machine()
        require(architecture in {"x86_64", "aarch64"}, "Supported targets are x86_64 and aarch64.")
        uefi = Path("/sys/firmware/efi").is_dir()
        require(uefi or architecture == "x86_64", "aarch64 requires booting the Live image in UEFI mode.")
        for command in ("rsync", "findmnt", "lsblk", "blkid", "mount", "umount", "chroot", "setfiles", "parted",
                        "systemctl", "rpm", "sync", "udevadm") + (("efibootmgr",) if uefi else ()):
            require(shutil.which(command) is not None, f"The Live image is missing the required utility {command}.")
        require(Path(PARTITION_TOOL).is_file(), "The Fedora partition helper is not installed.")
        require((source / "usr/lib/systemd/system/ddm.service").is_file(), "The Live image must include DDM.")
        require((source / "etc/selinux/targeted/contexts/files/file_contexts").is_file(),
                "The Live image must include selinux-policy-targeted.")
        require((source / "etc/pam.d/password-auth").exists() and (source / "etc/pam.d/postlogin").exists(),
                "The Live image must include Fedora's authselect PAM configuration.")
        if uefi:
            suffix = "x64" if architecture == "x86_64" else "aa64"
            self.efi_file(f"shim-{suffix}", f"shim{suffix}.efi", root=source)
            self.efi_file(f"grub2-efi-{suffix}", f"grub{suffix}.efi", root=source)
        else:
            require((source / "usr/lib/grub/i386-pc").is_dir(),
                    "BIOS installation requires grub2-pc and grub2-pc-modules in the Live image.")
        require(self.config.get("DI_PARTITION_TYPE", "0") in ALLOWED_MODES,
                "Choose a normal full-disk, manual or free-space installation.")
        require(not self.config.get("DI_CRYPT_PASSWORD") and self.config.get("DI_IS_GHOST_MODE", "false") != "true",
                "Encrypted and disk-clone installation modes are not supported.")
        require(not self.target.is_symlink() and not os.path.ismount(self.target),
                "The target is already mounted. Finish or explicitly release the existing installation first.")
        plan_path = Path(self.config.get("DI_PARTITION_CONFIG"))
        require(plan_path.is_file() and not plan_path.is_symlink(), "The confirmed partition plan is missing.")
        plan_bytes = plan_path.read_bytes()
        devices = validate_plan(json.loads(plan_bytes))
        requested_mounts = {item.get("mountPoint") for item in json.loads(plan_bytes)
                            if item.get("type") == "partition" and item.get("operate") not in {"delete", "normal"}}
        boot_parts = [item for item in json.loads(plan_bytes) if item.get("type") == "partition"
                      and item.get("operate") not in {"delete", "normal"} and item.get("mountPoint") == "/boot"]
        require(len(boot_parts) == 1 and boot_parts[0].get("filesystem") == "ext4",
                "This release requires a separate ext4 /boot partition.")
        require(boot_parts[0].get("operate") == "new" or boot_parts[0].get("isneedformat") is True,
                "The /boot partition must be new or explicitly selected for formatting.")
        require(not uefi or "/boot/efi" in requested_mounts,
                "UEFI installation requires an EFI System Partition at /boot/efi.")
        if uefi:
            esp_parts = [item for item in json.loads(plan_bytes) if item.get("mountPoint") == "/boot/efi"
                         and item.get("operate") not in {"delete", "normal"}]
            require(len(esp_parts) == 1 and esp_parts[0].get("filesystem") in {"vfat", "fat32", "efi"},
                    "The EFI System Partition must use FAT32.")
        else:
            require(len(devices) == 1, "This release supports BIOS installation to one disk at a time.")
            disk_operations = [item for item in json.loads(plan_bytes) if item.get("type") == "disk"]
            label = next((item.get("diskType", "") for item in disk_operations
                          if item.get("device") == devices[0]), "")
            if not label:
                label = self.run("lsblk", "--nodeps", "--noheadings", "--output", "PTTYPE", devices[0]).stdout.strip()
            require(label in {"dos", "msdos"}, "BIOS installation requires an MBR/MS-DOS disk in this release.")
        for device in devices:
            require(stat.S_ISBLK(os.stat(device).st_mode), f"Not a block device: {device}")
            mounted = self.run("lsblk", "--noheadings", "--raw", "--output", "MOUNTPOINTS", device).stdout.strip()
            require(not mounted, f"{device} has mounted filesystems; unmount them before installing.")
        self.state = {"boot_id": self.boot_id, "target": str(self.target), "source": str(source),
                      "architecture": architecture, "uefi": uefi, "devices": devices,
                      "plan_path": str(plan_path), "plan_sha256": hashlib.sha256(plan_bytes).hexdigest(),
                      "installation_id": str(uuid.uuid4()), "owned_mounts": [], "mounts": []}
        self.save("preflight")
        self.config.set("DI_BOOTLOADER_IS_EFI", "true" if uefi else "false")
        self.progress(1)

    def partition(self) -> None:
        self.load("preflight")
        plan_bytes = Path(self.state["plan_path"]).read_bytes()
        require(hashlib.sha256(plan_bytes).hexdigest() == self.state["plan_sha256"],
                "The partition plan changed after validation. Review it again.")
        # Mark before the destructive helper: retries must never silently format again.
        self.save("partitioning")
        self.config.set("DI_MOUNTS_POINTS", "")
        self.run(PARTITION_TOOL, "-m", self.config.get("DI_PARTITION_TYPE", "0"),
                 "-c", self.state["plan_path"], capture=False)
        self.run("udevadm", "settle")
        mappings = parse_mounts(self.config.get("DI_MOUNTS_POINTS"))
        require(any(item["mountpoint"] == "/boot" for item in mappings),
                "The partition helper did not return the required separate /boot partition.")
        require(not self.state["uefi"] or any(item["mountpoint"] == "/boot/efi" for item in mappings),
                "UEFI installation requires an EFI System Partition mounted at /boot/efi.")
        self.state["mounts"] = mappings
        for mapping in mappings:
            disk = self.run("lsblk", "-nro", "PKNAME", "--paths", mapping["device"]).stdout.strip()
            require(disk in self.state["devices"], "The partition helper returned a device outside the confirmed disks.")
            if mapping["mountpoint"] == "/boot/efi":
                properties = self.run("udevadm", "info", "--query=property", "--name", mapping["device"]).stdout
                number = next((line.split("=", 1)[1] for line in properties.splitlines()
                               if line.startswith("ID_PART_ENTRY_NUMBER=")), "")
                require(number.isdigit(), "Cannot identify the EFI partition number.")
                self.run("parted", "--script", disk, "set", number, "esp", "on")
        self.target.mkdir(mode=0o755, exist_ok=True)
        for mapping in mappings:
            if mapping["mountpoint"] == "swap":
                continue  # Do not activate or disable swap in the Live host.
            point = self.target / mapping["mountpoint"].lstrip("/")
            point.mkdir(parents=True, exist_ok=True)
            self.run("mount", "--options", mapping["options"], mapping["device"], str(point))
            self.state["owned_mounts"].append(str(point))
            self.save()
        self.state["mount_identity"] = self.mount_identity()
        self.state["mount_identities"] = {point: self.mount_identity(Path(point))
                                          for point in self.state["owned_mounts"]}
        self.save("partitioned")
        self.progress(10)

    def copy(self) -> None:
        self.load("partitioned", "copying")
        self.verify_target(marker=False)
        source = Path(self.state["source"])
        self.source_check(source)
        self.save("copying")
        exclusions = ["/dev/***", "/proc/***", "/sys/***", "/run/***", "/tmp/***", "/mnt/***", "/media/***",
                      "/target/***", "/lost+found", "/etc/machine-id", "/var/lib/dbus/machine-id",
                      "/var/lib/systemd/random-seed", "/etc/ssh/ssh_host_*", "/var/log/***", "/var/tmp/***",
                      "/var/cache/***", "/root/***",
                      "/etc/fedora-dde-installer/***", "/home/*/.cache/***"]
        live_user = self.config.get("LIVE_USERNAME", "liveuser")
        require(bool(re.fullmatch(r"[a-z_][a-z0-9_-]{0,31}", live_user)), "Invalid Live username.")
        exclusions.append(f"/home/{live_user}/***")
        # rsync preserves owners, capabilities, ACLs, hardlinks and xattrs.  Runtime
        # mounts and machine identity are excluded; no --delete can erase /home.
        command = ["rsync", "-aHAX", "--numeric-ids", "--one-file-system", "--info=progress2"]
        command += [f"--exclude={item}" for item in exclusions]
        command += [str(source).rstrip("/") + "/", str(self.target) + "/"]
        self.runner.run(command, capture=False)
        for directory, mode in (("dev", 0o755), ("proc", 0o755), ("sys", 0o755), ("run", 0o755),
                                ("tmp", 0o1777), ("var/tmp", 0o1777), ("var/log", 0o755), ("var/cache", 0o755),
                                ("root", 0o700)):
            path = self.path(directory)
            require(not path.is_symlink(), f"The payload contains an unsafe {directory} symlink.")
            path.mkdir(parents=True, exist_ok=True)
            path.chmod(mode)
        self.write("etc/fedora-dde-installer-target", self.state["installation_id"] + "\n", 0o600)
        self.save("copied")
        self.progress(60)

    def bind_runtime(self) -> None:
        for relative in ("dev", "sys"):
            destination = str(self.path(relative))
            if destination not in self.state["owned_mounts"]:
                self.run("mount", "--rbind", "/" + relative, destination)
                self.run("mount", "--make-rslave", destination)
                self.state["owned_mounts"].append(destination)
                self.save()
        for relative, filesystem in (("proc", "proc"), ("run", "tmpfs")):
            destination = str(self.path(relative))
            if destination not in self.state["owned_mounts"]:
                self.run("mount", "--types", filesystem, "--options", "nosuid,nodev,noexec", filesystem, destination)
                self.state["owned_mounts"].append(destination)
                self.save()

    def write_fstab(self) -> None:
        lines = ["# Generated by Fedora DDE Installer.\n"]
        for mapping in self.state["mounts"]:
            device, point = mapping["device"], mapping["mountpoint"]
            identifier = self.run("blkid", "-s", "UUID", "-o", "value", device).stdout.strip()
            filesystem = self.run("blkid", "-s", "TYPE", "-o", "value", device).stdout.strip()
            require(bool(re.fullmatch(r"[A-Fa-f0-9-]+", identifier)), f"Missing filesystem UUID for {device}.")
            require(filesystem in ROOT_FILESYSTEMS | {"vfat", "ext2", "ext3", "swap"}, "Unsupported target filesystem.")
            require(point != "/boot/efi" or filesystem == "vfat", "The EFI System Partition must use FAT.")
            mapping["uuid"], mapping["fstype"] = identifier, filesystem
            if point == "swap":
                lines.append(f"UUID={identifier}\tnone\tswap\tdefaults\t0 0\n")
            else:
                options = "umask=0077,shortname=winnt" if point == "/boot/efi" else mapping["options"]
                check = ("1" if point == "/" else "2") if filesystem in {"ext2", "ext3", "ext4"} else "0"
                lines.append(f"UUID={identifier}\t{point}\t{filesystem}\t{options}\t0 {check}\n")
        self.write("etc/fstab", "".join(lines))
        self.write("etc/crypttab", "# No encrypted devices are configured by this installer.\n", 0o600)
        self.save()

    def clean_live_settings(self) -> None:
        live_user = self.config.get("LIVE_USERNAME", "liveuser")
        require(bool(re.fullmatch(r"[a-z_][a-z0-9_-]{0,31}", live_user)), "Invalid Live username.")
        self.state["live_user"] = live_user
        # The image's live-only service must never take over the installed login.
        for unit in ("livesys.service", "livesys-late.service", "fedora-dde-installer.service"):
            self.run("systemctl", "--root", str(self.target), "disable", unit, check=False)
        for pattern in ("etc/sudoers.d/*", "etc/systemd/system/getty@*.service.d/*.conf",
                        "etc/systemd/system/serial-getty@*.service.d/*.conf", "etc/sddm.conf.d/*",
                        "etc/lightdm/lightdm.conf.d/*", "etc/dconf/db/local.d/*"):
            for path in self.target.glob(pattern):
                if path.is_file() and not path.is_symlink():
                    contents = path.read_text(errors="replace")
                    if live_user in contents and ("autologin" in contents.lower() or "NOPASSWD" in contents or "liveuser" in contents):
                        path.unlink()
        # Live images may grant their live user or the whole wheel group
        # passwordless sudo. The new wheel user must not inherit either rule.
        sudoers_paths = [self.path("etc/sudoers"), *self.target.glob("etc/sudoers.d/*")]
        for sudoers in sudoers_paths:
            if sudoers.is_file() and not sudoers.is_symlink():
                contents = sudoers.read_text()
                cleaned = "".join(line for line in contents.splitlines(keepends=True)
                                  if not re.match(rf"\s*(?:{re.escape(live_user)}|%wheel)\s+.*NOPASSWD", line))
                if cleaned != contents:
                    self.atomic(sudoers, cleaned, 0o440)
        # DConfig defaults are also reset, so a live-image default cannot retain
        # a passwordless account on the installed system.  Runtime caches were
        # excluded from the payload copy.
        schema = self.path("usr/share/dsg/configs/org.deepin.dde.ddm/org.deepin.dde.ddm.json")
        if schema.is_file():
            data = json.loads(schema.read_text())
            for key in ("autologinUser", "autologinSession", "lastUser", "lastSession"):
                if key in data.get("contents", {}):
                    data["contents"][key]["value"] = ""
            self.atomic(schema, json.dumps(data, indent=2) + "\n")
        for parent in ("etc/dsg/configs/overrides", "usr/share/dsg/configs/overrides"):
            for path in self.target.glob(parent + "/**/*.json"):
                if path.is_file() and not path.is_symlink():
                    text = path.read_text(errors="replace")
                    if "autologinUser" in text or "autologinSession" in text:
                        try:
                            data = json.loads(text)
                        except ValueError:
                            continue
                        for key in ("autologinUser", "autologinSession"):
                            setting = data.get("contents", {}).get(key)
                            if isinstance(setting, dict):
                                setting["value"] = ""
                        self.atomic(path, json.dumps(data, indent=2) + "\n")
        self.save()

    def configure_fedora_pam(self) -> None:
        # DDM's upstream generic files use Arch/Debian include names. Fedora's
        # authselect owns password-auth/postlogin; preserve that generated stack
        # and the SELinux/loginuid ordering used by Fedora display managers.
        self.write("etc/pam.d/ddm", """#%PAM-1.0
auth       substack     password-auth
auth       include      postlogin
account    required     pam_nologin.so
account    include      password-auth
password   include      password-auth
session    required     pam_selinux.so close
session    required     pam_loginuid.so
session    required     pam_selinux.so open
session    required     pam_namespace.so
session    optional     pam_keyinit.so force revoke
session    include      password-auth
session    include      postlogin
""")
        # Installed systems deliberately have no passwordless DDM login. A
        # separate Live image may ship its own live-only autologin PAM policy.
        self.write("etc/pam.d/ddm-autologin", """#%PAM-1.0
auth       requisite    pam_deny.so
account    include      password-auth
password   required     pam_deny.so
session    include      password-auth
session    include      postlogin
""")
        self.write("etc/selinux/config", "SELINUX=enforcing\nSELINUXTYPE=targeted\n")

    def install_kernels(self) -> None:
        root = next(item for item in self.state["mounts"] if item["mountpoint"] == "/")
        options = f"root=UUID={root['uuid']} ro rhgb quiet"
        self.write("etc/kernel/cmdline", options + "\n")
        self.write("etc/default/grub", 'GRUB_TIMEOUT=5\nGRUB_DISTRIBUTOR="Fedora DDE Remix"\nGRUB_DEFAULT=saved\n'
                   'GRUB_DISABLE_SUBMENU=true\nGRUB_TERMINAL_OUTPUT=console\n'
                   'GRUB_ENABLE_BLSCFG=true\nGRUB_CMDLINE_LINUX="rhgb quiet"\n')
        self.write("etc/dracut.conf.d/90-fedora-dde-installer.conf",
                   'hostonly="no"\nomit_dracutmodules+=" dmsquash-live livenet "\n')
        machine_id = uuid.uuid4().hex
        self.write("etc/machine-id", machine_id + "\n", 0o444)
        self.write("etc/kernel/entry-token", machine_id + "\n")
        dbus_id = self.path("var/lib/dbus/machine-id")
        dbus_id.parent.mkdir(parents=True, exist_ok=True)
        if dbus_id.exists() or dbus_id.is_symlink():
            dbus_id.unlink()
        dbus_id.symlink_to("/etc/machine-id")
        entries = self.path("boot/loader/entries")
        entries.mkdir(parents=True, exist_ok=True)
        for path in entries.glob("*.conf"):
            path.unlink()  # All copied entries refer to the Live source, never this root.
        versions = sorted(path.name for path in self.path("usr/lib/modules").iterdir() if path.is_dir())
        installed = []
        for version in versions:
            require(bool(re.fullmatch(r"[A-Za-z0-9._+-]+", version)), "Invalid installed kernel version.")
            source = self.path(f"usr/lib/modules/{version}/vmlinuz")
            destination = self.path(f"boot/vmlinuz-{version}")
            if source.is_file():
                require(source.resolve().is_relative_to(self.target.resolve()), "The kernel source escapes the target.")
                if destination.is_symlink():
                    destination.unlink()
                shutil.copy2(source, destination)
            require(destination.is_file(), f"Kernel image {version} is missing from the payload.")
            self.chroot("dracut", "--force", "--no-hostonly", "--omit", "dmsquash-live livenet",
                        f"/boot/initramfs-{version}.img", version,
                        capture=False)
            require(self.path(f"boot/initramfs-{version}.img").is_file(), "dracut did not create the target initramfs.")
            self.write(f"boot/loader/entries/{machine_id}-{version}.conf",
                       f"title Fedora DDE Remix ({version})\nversion {version}\n"
                       f"linux /vmlinuz-{version}\ninitrd /initramfs-{version}.img\noptions {options}\n"
                       "grub_users $grub_users\ngrub_arg --unrestricted\ngrub_class fedora\n")
            installed.append(version)
        require(bool(installed), "No installed kernel was found.")
        self.state["kernels"] = installed
        self.state["kernel_options"] = options
        self.save()

    def efi_file(self, package: str, name: str, root: Path | None = None) -> Path:
        root = root or self.target
        paths = self.run("rpm", "--root", str(root), "-ql", package).stdout.splitlines()
        matches = [root / path.lstrip("/") for path in paths
                   if path.endswith("/" + name) and (root / path.lstrip("/")).is_file()
                   and (root / path.lstrip("/")).resolve().is_relative_to(root.resolve())]
        require(bool(matches), f"The Live image must include {package} with {name}.")
        # Fedora 44+ keeps the signed payload under /usr/lib/efi; older images
        # carry it directly on /boot/efi. Prefer the package-owned source copy.
        return sorted(matches, key=lambda path: ("/usr/lib/efi/" not in str(path), str(path)))[0]

    def install_bootloader(self) -> None:
        if self.state["uefi"]:
            suffix = "x64" if self.state["architecture"] == "x86_64" else "aa64"
            shim = self.efi_file(f"shim-{suffix}", f"shim{suffix}.efi")
            grub = self.efi_file(f"grub2-efi-{suffix}", f"grub{suffix}.efi")
            vendor = self.path("boot/efi/EFI/fedora")
            fallback = self.path("boot/efi/EFI/BOOT")
            vendor.mkdir(parents=True, exist_ok=True)
            fallback.mkdir(parents=True, exist_ok=True)
            fallback_loader = fallback / f"BOOT{suffix.upper()}.EFI"
            install_fallback = not fallback_loader.exists()
            copies = [(shim, vendor / f"shim{suffix}.efi"), (grub, vendor / f"grub{suffix}.efi")]
            if install_fallback:
                copies += [(shim, fallback_loader), (grub, fallback / f"grub{suffix}.efi")]
            for source, destination in copies:
                if source.resolve() != destination.resolve():
                    if destination.is_symlink():
                        destination.unlink()
                    shutil.copyfile(source, destination)
            try:
                manager = self.efi_file(f"shim-{suffix}", f"mm{suffix}.efi")
                for directory in ((vendor, fallback) if install_fallback else (vendor,)):
                    if manager.resolve() != (directory / manager.name).resolve():
                        shutil.copyfile(manager, directory / manager.name)
            except InstallError:
                pass  # MokManager is not required to boot the Fedora-signed chain.
            boot = next((item for item in self.state["mounts"] if item["mountpoint"] == "/boot"),
                        next(item for item in self.state["mounts"] if item["mountpoint"] == "/"))
            prefix = "/grub2" if boot["mountpoint"] == "/boot" else "/boot/grub2"
            stub = f"search --no-floppy --fs-uuid --set=dev {boot['uuid']}\nset prefix=($dev){prefix}\nexport prefix\nconfigfile $prefix/grub.cfg\n"
            self.write("boot/efi/EFI/fedora/grub.cfg", stub)
            if install_fallback:
                self.write("boot/efi/EFI/BOOT/grub.cfg", stub)
                self.state["fallback_efi_sha256"] = hashlib.sha256(shim.read_bytes()).hexdigest()
                self.save()
            fallback_owned = (self.state.get("fallback_efi_sha256") == hashlib.sha256(fallback_loader.read_bytes()).hexdigest()
                              and (fallback / "grub.cfg").is_file()
                              and (fallback / "grub.cfg").read_text() == stub)
            esp = next(item for item in self.state["mounts"] if item["mountpoint"] == "/boot/efi")
            disk = self.run("lsblk", "-nro", "PKNAME", "--paths", esp["device"]).stdout.strip()
            properties = self.run("udevadm", "info", "--query=property", "--name", esp["device"]).stdout
            number = next((line.split("=", 1)[1] for line in properties.splitlines()
                           if line.startswith("ID_PART_ENTRY_NUMBER=")), "")
            require(bool(DEVICE_RE.fullmatch(disk)) and number.isdigit(), "Cannot identify the EFI partition.")
            if not self.state.get("firmware_entry_created"):
                result = self.run("efibootmgr", "--create", "--disk", disk, "--part", number,
                                  "--label", "Fedora DDE Remix", "--loader", f"\\EFI\\fedora\\shim{suffix}.efi", check=False)
                if result.returncode:
                    require(fallback_owned, "Firmware entry creation failed and another system owns the UEFI fallback path.")
                    print("Firmware entry could not be written; the standard UEFI fallback loader is installed.", flush=True)
                    self.state["firmware_fallback"] = True
                else:
                    self.state["firmware_entry_created"] = True
                self.save()
        else:
            boot_disk = self.config.get("DI_BOOT_DISK")
            if not boot_disk:
                require(len(self.state["devices"]) == 1, "Select DI_BOOT_DISK explicitly for a multi-disk BIOS installation.")
                boot_disk = self.state["devices"][0]
            require(boot_disk in self.state["devices"], "The BIOS boot disk is not part of the confirmed installation plan.")
            self.chroot("grub2-install", "--target=i386-pc", "--recheck", boot_disk, capture=False)
        self.path("boot/grub2").mkdir(parents=True, exist_ok=True)
        self.chroot("grub2-editenv", "/boot/grub2/grubenv", "create")
        self.chroot("grub2-editenv", "/boot/grub2/grubenv", "set", "blsdir=/loader/entries",
                    "kernelopts=" + self.state.get("kernel_options", ""))
        self.chroot("grub2-mkconfig", "--output=/boot/grub2/grub.cfg", capture=False)
        require(self.path("boot/grub2/grub.cfg").is_file(), "GRUB configuration was not created.")
        self.save()

    def system(self) -> None:
        self.load("copied", "system-configuring")
        self.verify_target()
        self.save("system-configuring")
        self.write_fstab()
        self.bind_runtime()
        self.clean_live_settings()
        self.configure_fedora_pam()
        self.install_kernels()
        self.install_bootloader()
        require(self.path("usr/lib/systemd/system/ddm.service").is_file(), "The Fedora Live image must include the DDM desktop.")
        for service in ("gdm.service", "sddm.service", "lightdm.service"):
            self.run("systemctl", "--root", str(self.target), "disable", service, check=False)
        self.run("systemctl", "--root", str(self.target), "enable", "--force", "ddm.service")
        self.run("systemctl", "--root", str(self.target), "set-default", "graphical.target")
        self.save("system-configured")
        self.progress(85)

    def ready(self) -> None:
        self.load("system-configured", "awaiting-user")
        self.verify_target()
        self.save("awaiting-user")
        self.config.set("DI_TARGET_STAGE", "awaiting-user")
        self.progress(90)
        print("System files are installed. The target remains mounted for account configuration.", flush=True)

    def configure(self) -> None:
        self.load("awaiting-user", "configuring-user", "user-configured", "finalized")
        if self.state["phase"] == "finalized":
            return
        self.verify_target()
        if self.state["phase"] == "user-configured":
            for key in SECRET_KEYS:
                self.config.set(key, "")
            return
        keys = ("DI_USERNAME", "DI_PASSWORD", "DI_HOSTNAME", "DI_LOCALE", "DI_TIMEZONE", "DI_LAYOUT",
                "DI_LAYOUT_VARIANT", "DI_PASSWORD_ENCRYPTION_CONFIG", "DI_REGION_FORMAT_LOCALE",
                "DI_REGION_FORMAT_LOCALE_SET")
        account = validate_account({key: self.config.get(key) for key in keys})
        user = account["user"]
        require(not self.state.get("user") or self.state["user"] == user,
                "An account was already created; retry configuration with the same username.")
        zone = self.path("usr/share/zoneinfo/" + account["timezone"])
        require(zone.is_file(), "The requested timezone is not available in the target.")
        available = self.chroot("locale", "-a").stdout.lower().replace("utf-8", "utf8")
        require(all(account[key].lower().replace("utf-8", "utf8") in available.splitlines()
                    for key in ("locale", "region_locale")),
                "The Live image lacks the selected language pack. Include the matching glibc-langpack in the ISO.")
        self.save("configuring-user")
        existing = self.chroot("getent", "passwd", user, check=False)
        require(existing.returncode != 0 or self.state.get("user") == user or user == self.state.get("live_user"),
                "That account already exists in the image; choose another username.")
        self.state["user"] = user
        self.save()
        if existing.returncode:
            self.chroot("useradd", "--create-home", "--user-group", "--shell", "/bin/bash", "--groups", "wheel", user)
        else:
            self.chroot("usermod", "--append", "--groups", "wheel", "--shell", "/bin/bash", user)
        self.chroot("install", "-d", "-m", "0700", "-o", user, "-g", user, f"/home/{user}")
        self.chroot("chpasswd", input_text=f"{user}:{account['password']}\n")
        self.chroot("passwd", "--lock", "root")
        live_user = self.state.get("live_user", "liveuser")
        if live_user != user and self.chroot("getent", "passwd", live_user, check=False).returncode == 0:
            self.chroot("userdel", "--remove", live_user)
        self.write("etc/hostname", account["hostname"] + "\n")
        self.write("etc/hosts", "127.0.0.1 localhost localhost.localdomain\n::1 localhost localhost.localdomain\n"
                   f"127.0.1.1 {account['hostname']}\n")
        locale_text = f"LANG={account['locale']}\n" + "".join(
            f"{key}={account['region_locale']}\n"
            for key in ("LC_TIME", "LC_NUMERIC", "LC_MONETARY", "LC_MEASUREMENT", "LC_PAPER"))
        self.write("etc/locale.conf", locale_text)
        localtime = self.path("etc/localtime")
        if localtime.exists() or localtime.is_symlink():
            localtime.unlink()
        localtime.symlink_to("../usr/share/zoneinfo/" + account["timezone"])
        self.write("etc/adjtime", "0.0 0 0.0\n0\nUTC\n")
        self.write("etc/vconsole.conf", "KEYMAP=us\n")
        self.write("etc/X11/xorg.conf.d/00-keyboard.conf", 'Section "InputClass"\n Identifier "system-keyboard"\n'
                   ' MatchIsKeyboard "on"\n' + f' Option "XkbLayout" "{account["layout"]}"\n'
                   + f' Option "XkbVariant" "{account["variant"]}"\nEndSection\n')
        self.write("etc/sudoers.d/90-fedora-dde-admin", "%wheel ALL=(ALL) ALL\n", 0o440)
        self.chroot("visudo", "--check")
        session = self.config.get("DI_DDE_SESSION", "treeland")
        require(bool(re.fullmatch(r"[A-Za-z0-9_-]+", session))
                and self.path(f"usr/share/wayland-sessions/{session}.desktop").is_file(),
                "The configured DDE Wayland session is missing.")
        settings = ("[User]\nSystemAccount=false\n" + f"Language={account['locale']}\n"
                    + f"XSession={session}\nSession={session}\nSessionType=wayland\n")
        self.write(f"var/lib/AccountsService/users/{user}", settings, 0o600)
        self.write(f"var/lib/AccountsService/deepin/users/{user}", settings + f"Locale={account['locale']}\n"
                   + f"Layout={account['layout']};{account['variant']}\n", 0o600)
        self.write(f"home/{user}/.config/locale.conf", locale_text, 0o600)
        self.chroot("chown", f"{user}:{user}", f"/home/{user}/.config", f"/home/{user}/.config/locale.conf")
        if self.config.get("DI_IS_ENABLE_NTP", "true") == "true" and self.path("usr/lib/systemd/system/chronyd.service").is_file():
            self.run("systemctl", "--root", str(self.target), "enable", "chronyd.service")
        self.remove_installer_from_target()
        self.relabel()
        self.save("user-configured")
        for key in SECRET_KEYS:
            self.config.set(key, "")
        self.config.set("DI_TARGET_STAGE", "user-configured")
        self.progress(95)

    def remove_installer_from_target(self) -> None:
        # The process and hooks run from the Live source, so removing the copied
        # RPM here cannot interrupt configuration. Normal RPM dependency checks
        # remain enabled and no repository/network access is needed.
        if self.chroot("rpm", "--query", APP, check=False).returncode == 0:
            self.chroot("rpm", "--erase", APP)
        for relative in ("usr/bin/fedora-dde-installer-launch",
                         "usr/share/applications/fedora-dde-installer.desktop",
                         "usr/share/polkit-1/actions/org.fedoradde.installer.policy",
                         "usr/lib/systemd/system/fedora-dde-installer.service"):
            path = self.path(relative)
            if path.is_file() or path.is_symlink():
                path.unlink()

    def relabel(self) -> None:
        contexts = self.path("etc/selinux/targeted/contexts/files/file_contexts")
        require(contexts.is_file(), "The Fedora payload must include selinux-policy-targeted.")
        command = ["setfiles", "-F", "-r", str(self.target)]
        for relative in ("dev", "proc", "sys", "run", "boot/efi"):
            command += ["-e", str(self.path(relative))]
        command += [str(contexts), str(self.target)]
        self.runner.run(command, capture=False)

    def finalize(self) -> None:
        self.load("user-configured", "finalized")
        if self.state["phase"] == "finalized":
            self.config.set("DI_TARGET_STAGE", "complete")
            self.progress(100)
            return
        self.verify_target()
        require(bool(self.state.get("user")), "Account setup has not completed.")
        for key in SECRET_KEYS:
            self.config.set(key, "")
        if not self.state.get("summary_written"):
            self.write("var/log/fedora-dde-installer/install-summary.json", json.dumps({
                "installation_id": self.state["installation_id"], "architecture": self.state["architecture"],
                "uefi": self.state["uefi"], "kernels": self.state.get("kernels", []),
                "firmware_fallback": self.state.get("firmware_fallback", False), "status": "configured"}, indent=2) + "\n", 0o600)
            # A retry may already have unmounted a separate /var. Do not write
            # through its former mount point into the filesystem underneath.
            self.state["summary_written"] = True
            self.save()
        self.run("sync")
        # Only mounts made by this backend are released. Never use lazy/forced
        # unmount, swapoff -a, or kill processes belonging to the Live desktop.
        for mountpoint in list(reversed(self.state["owned_mounts"])):
            if os.path.ismount(mountpoint):
                self.run("umount", "--recursive", mountpoint)
            self.state["owned_mounts"].remove(mountpoint)
            self.save()
        self.save("finalized")
        self.config.set("DI_TARGET_STAGE", "complete")
        self.progress(100)
        print("Installation and account configuration are complete. Restart only when you choose to.", flush=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("stage", choices=("preflight", "partition", "copy", "system", "ready", "configure", "finalize"))
    args = parser.parse_args()
    os.umask(0o077)
    try:
        require(os.geteuid() == 0, "The installation backend must run as root.")
        runner = Runner()
        config = Config(Path("/run/fedora-dde-installer/installer.conf"), runner)
        config.require_private()
        backend = Backend(config, runner)
        getattr(backend, args.stage)()
    except (InstallError, OSError, ValueError, KeyError) as error:
        print(f"Fedora DDE Installer: {error}", file=sys.stderr, flush=True)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
