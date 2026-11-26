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

# Hooks run with Python's isolated (-I) mode. Only permit imports from this
# installed, root-owned script directory; never consult the caller's cwd or
# PYTHONPATH. Keep one exception/state type when this file is the entry point.
sys.path.insert(0, str(Path(__file__).resolve().parent))
if __name__ == "__main__":
    sys.modules["fedora_backend"] = sys.modules[__name__]


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
    devices = set()
    for entry in filter(None, value.split(";")):
        match = re.fullmatch(r"([^=]+)=([^\[]+)(?:\[([^\]]+)\])?", entry)
        require(match is not None, "The partition helper returned an invalid mount entry.")
        device, mountpoint, options = match.groups()
        require(bool(DEVICE_RE.fullmatch(device)) and ".." not in Path(device).parts,
                "A target device path is invalid.")
        require(mountpoint in ALLOWED_MOUNTS, f"Unsupported target mount point: {mountpoint}")
        require(mountpoint == "swap" or mountpoint not in seen,
                f"Duplicate target mount point: {mountpoint}")
        require(device not in devices, "The partition helper returned the same partition more than once.")
        require(options in (None, "compress=zstd", "compress=zstd:1"), "Unsupported filesystem mount options.")
        seen.add(mountpoint)
        devices.add(device)
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
            require(item.get("operate") in {"new", "edit"}, "Unsupported disk operation.")
            require(item.get("diskType") in {"gpt", "msdos", "dos"}, "Unsupported partition table type.")
        else:
            path = item.get("path", "")
            require(not path or bool(DEVICE_RE.fullmatch(path)) and ".." not in Path(path).parts,
                    "A partition device path is invalid.")
            require(bool(re.fullmatch(r"[A-Za-z0-9_.-]{0,32}", item.get("label", ""))),
                    "Partition labels must use at most 32 letters, digits, dots, underscores or hyphens.")
            require(item.get("filesystem", "") in ROOT_FILESYSTEMS | {"", "vfat", "fat32", "efi", "ext2", "ext3", "swap", "linux-swap"},
                    "The partition filesystem is unsupported.")
            require(isinstance(item.get("isBiosBoot", False), bool), "Invalid BIOS boot partition flag.")
            if item.get("isBiosBoot"):
                require(not item.get("mountPoint") and not item.get("filesystem")
                        and not item.get("isneedformat"),
                        "A BIOS boot partition must be unformatted and have no mount point.")
            if item.get("operate") == "new" and item.get("filesystem"):
                require(item.get("isneedformat") is True,
                        "A newly created filesystem partition must be explicitly formatted.")
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
            require(item.get("isneedformat") is True,
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
        from fedora_profile import state_profile

        require(self.state_file.is_file() and not self.state_file.is_symlink(),
                "No installation from this Live session is available.")
        self.state = json.loads(self.state_file.read_text())
        require(self.state.get("boot_id") == self.boot_id and self.state.get("target") == str(self.target),
                "The installation target belongs to a different session.")
        state_profile(self.state)
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
                          "--output", "SOURCE,MAJ:MIN,FSTYPE,UUID,OPTIONS,FSROOT")
        columns = result.stdout.strip().split()
        filesystems = ROOT_FILESYSTEMS if point == self.target else ROOT_FILESYSTEMS | {"ext2", "ext3", "vfat"}
        require(len(columns) == 6 and columns[0].startswith("/dev/") and columns[2] in filesystems
                and bool(re.fullmatch(r"[A-Fa-f0-9-]+", columns[3])) and "rw" in columns[4].split(",")
                and columns[5].startswith("/"),
                "The target must be a mounted writable disk filesystem with a UUID.")
        identity = dict(zip(("source", "major_minor", "fstype", "uuid"), columns[:4]))
        identity["source"] = identity["source"].split("[", 1)[0]
        identity["fsroot"] = columns[5]  # Distinguish Btrfs subvolumes sharing a device and UUID.
        return identity

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
        from fedora_payload import validate_root
        validate_root(self, source)

    def preflight(self) -> None:
        from fedora_space import prepare as prepare_space
        from fedora_boot import firmware_suffix
        from fedora_payload import prepare_source, release_source, rooted_path
        from fedora_profile import selinux_disabled
        from fedora_storage import validate_layout
        if self.state_file.exists():
            self.load()
            require(self.state.get("phase") in {"preflight", "source-validation-failed", "finalized"},
                    "An installation is already active in this Live session.")
            if self.state.get("phase") in {"preflight", "source-validation-failed"}:
                release_source(self)
        commandline = Path("/proc/cmdline").read_text().strip()
        require("fips=1" not in commandline.split(),
                "FIPS installation is not implemented; boot this Live installer without fips=1.")
        architecture = platform.machine()
        require(architecture in {"x86_64", "aarch64"}, "Supported targets are x86_64 and aarch64.")
        uefi = Path("/sys/firmware/efi").is_dir()
        require(uefi or architecture == "x86_64", "aarch64 requires booting the Live image in UEFI mode.")
        for command in ("rsync", "findmnt", "lsblk", "blkid", "mount", "umount", "chroot", "parted", "btrfs",
                        "systemctl", "rpm", "sort", "sync", "udevadm") + (("efibootmgr",) if uefi else ()):
            require(shutil.which(command) is not None, f"The Live image is missing the required utility {command}.")
        require(Path(PARTITION_TOOL).is_file(), "The Fedora partition helper is not installed.")
        require(self.config.get("DI_PARTITION_TYPE", "0") in ALLOWED_MODES,
                "Choose a normal full-disk, manual or free-space installation.")
        require(not self.config.get("DI_CRYPT_PASSWORD") and self.config.get("DI_IS_GHOST_MODE", "false") != "true",
                "Encrypted and disk-clone installation modes are not supported.")
        require(not self.target.is_symlink() and not os.path.ismount(self.target),
                "The target is already mounted. Finish or explicitly release the existing installation first.")
        plan_path = Path(self.config.get("DI_PARTITION_CONFIG"))
        require(plan_path.is_file() and not plan_path.is_symlink(), "The confirmed partition plan is missing.")
        plan_bytes = plan_path.read_bytes()
        plan = json.loads(plan_bytes)
        devices = validate_plan(plan)
        tables = validate_layout(self, plan, devices, uefi)
        for device in devices:
            require(stat.S_ISBLK(os.stat(device).st_mode), f"Not a block device: {device}")
            mounted = self.run("lsblk", "--noheadings", "--raw", "--output", "MOUNTPOINTS", device).stdout.strip()
            require(not mounted, f"{device} has mounted filesystems; unmount them before installing.")
        source_state = prepare_source(self)
        source = Path(source_state["source"])
        try:
            space_requirements = prepare_space(self, source, plan)
            require(rooted_path(source, "usr/lib/systemd/system/ddm.service").is_file(), "The Live image must include DDM.")
            if not selinux_disabled(source_state):
                require(rooted_path(source, "etc/selinux/targeted/contexts/files/file_contexts").is_file(),
                        "The Live image must include selinux-policy-targeted.")
            suffix = firmware_suffix(architecture) if uefi else ""
            if uefi:
                self.efi_file(f"shim-{suffix}", f"shim{suffix}.efi", root=source)
                self.efi_file(f"grub2-efi-{suffix}", f"grub{suffix}.efi", root=source)
                require(any(rooted_path(source, prefix + "gen_grub_cfgstub").is_file()
                            for prefix in ("usr/bin/", "usr/sbin/")),
                        "The Live base must include Fedora's gen_grub_cfgstub utility.")
            else:
                require(rooted_path(source, "usr/lib/grub/i386-pc").is_dir()
                        and any(rooted_path(source, prefix + "grub2-install").is_file()
                                for prefix in ("usr/bin/", "usr/sbin/")),
                        "BIOS installation requires grub2-pc and grub2-pc-modules in the Live image.")
        except Exception:
            if source_state.get("source_mounted_this_call"):
                result = self.run("umount", str(source), check=False)
                if result.returncode:
                    # Keep ownership recorded if cleanup is temporarily busy;
                    # this state can only retry preflight, never partition.
                    self.state = {"boot_id": self.boot_id, "target": str(self.target), **source_state}
                    self.save("source-validation-failed")
            raise
        self.state = {"boot_id": self.boot_id, "target": str(self.target), **source_state,
                      "architecture": architecture, "uefi": uefi, "efi_suffix": suffix, "devices": devices,
                      "partition_tables": tables, "live_kernel_cmdline": commandline,
                      "space_requirements": space_requirements,
                      "plan_path": str(plan_path), "plan_sha256": hashlib.sha256(plan_bytes).hexdigest(),
                      "installation_id": str(uuid.uuid4()), "owned_mounts": [], "mounts": []}
        self.save("preflight")
        self.config.set("DI_BOOTLOADER_IS_EFI", "true" if uefi else "false")
        self.progress(1)

    def partition(self) -> None:
        from fedora_payload import verify_source
        from fedora_space import validate_capacity
        from fedora_storage import mount_filesystems, verify_bios_area
        self.load("preflight")
        plan_bytes = Path(self.state["plan_path"]).read_bytes()
        require(hashlib.sha256(plan_bytes).hexdigest() == self.state["plan_sha256"],
                "The partition plan changed after validation. Review it again.")
        verify_source(self)  # Check again before the first destructive operation.
        validate_capacity(json.loads(plan_bytes), self.state.get("space_requirements", {}))
        # Mark before the destructive helper: retries must never silently format again.
        self.save("partitioning")
        self.config.set("DI_MOUNTS_POINTS", "")
        self.run(PARTITION_TOOL, "-m", self.config.get("DI_PARTITION_TYPE", "0"),
                 "-c", self.state["plan_path"], capture=False)
        self.run("udevadm", "settle")
        verify_bios_area(self)
        mappings = parse_mounts(self.config.get("DI_MOUNTS_POINTS"))
        require(any(item["mountpoint"] == "/boot" for item in mappings),
                "The partition helper did not return the required separate /boot partition.")
        require(not self.state["uefi"] or any(item["mountpoint"] == "/boot/efi" for item in mappings),
                "UEFI installation requires an EFI System Partition mounted at /boot/efi.")
        for mapping in mappings:
            disk = self.run("lsblk", "-nro", "PKNAME", "--paths", mapping["device"]).stdout.strip()
            require(disk in self.state["devices"], "The partition helper returned a device outside the confirmed disks.")
            if mapping["mountpoint"] == "/boot/efi":
                properties = self.run("udevadm", "info", "--query=property", "--name", mapping["device"]).stdout
                number = next((line.split("=", 1)[1] for line in properties.splitlines()
                               if line.startswith("ID_PART_ENTRY_NUMBER=")), "")
                require(number.isdigit(), "Cannot identify the EFI partition number.")
                self.run("parted", "--script", disk, "set", number, "esp", "on")
        mount_filesystems(self, mappings)
        self.state["mount_identity"] = self.mount_identity()
        self.state["mount_identities"] = {point: self.mount_identity(Path(point))
                                          for point in self.state["owned_mounts"]}
        self.save("partitioned")
        self.progress(10)

    def copy(self) -> None:
        from fedora_payload import copy_payload, release_source
        from fedora_space import before_copy
        self.load("partitioned", "copying", "copied")
        self.verify_target(marker=self.state["phase"] == "copied")
        if self.state["phase"] != "copied":
            before_copy(self)
            self.save("copying")
            copy_payload(self)
            self.write("etc/fedora-dde-installer-target", self.state["installation_id"] + "\n", 0o600)
            self.save("copied")
        release_source(self)
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
        from fedora_system import clean_live_autologin

        live_user = self.config.get("LIVE_USERNAME", "liveuser")
        require(bool(re.fullmatch(r"[a-z_][a-z0-9_-]{0,31}", live_user)), "Invalid Live username.")
        self.state["live_user"] = live_user
        # The image's live-only service must never take over the installed login.
        for unit in ("livesys.service", "livesys-late.service", "fedora-dde-live.service", "fedora-dde-installer.service"):
            self.run("systemctl", "--root", str(self.target), "disable", unit, check=False)
        for pattern in ("etc/sddm.conf", "etc/sddm.conf.d/*.conf", "etc/gdm/custom.conf",
                        "etc/lightdm/lightdm.conf", "etc/lightdm/lightdm.conf.d/*.conf",
                        "etc/lxdm/lxdm.conf", "etc/dconf/db/local.d/*"):
            for path in self.target.glob(pattern):
                if path.is_file() and not path.is_symlink():
                    path = self.path(str(path.relative_to(self.target)))
                    contents = path.read_text(errors="replace")
                    cleaned = clean_live_autologin(contents, live_user)
                    if cleaned != contents:
                        self.atomic(path, cleaned, stat.S_IMODE(path.stat().st_mode))
        for pattern in ("etc/systemd/system/getty@*.service.d/*.conf",
                        "etc/systemd/system/serial-getty@*.service.d/*.conf"):
            for path in self.target.glob(pattern):
                if path.is_file() and not path.is_symlink():
                    path = self.path(str(path.relative_to(self.target)))
                    contents = path.read_text()
                    if re.search(rf"(?m)^\s*ExecStart\s*=.*(?:--autologin|-a)\s+{re.escape(live_user)}(?:\s|$)", contents):
                        # Drop both the Live ExecStart and its reset; preserve
                        # unrelated serial console and unit settings.
                        cleaned = "".join(line for line in contents.splitlines(keepends=True)
                                          if not re.match(r"\s*ExecStart\s*=", line))
                        self.atomic(path, cleaned, stat.S_IMODE(path.stat().st_mode))
        # Live images may grant their live user or the whole wheel group
        # passwordless sudo. The new wheel user must not inherit either rule.
        sudoers_paths = [self.path("etc/sudoers"), *self.target.glob("etc/sudoers.d/*")]
        for sudoers in sudoers_paths:
            if sudoers.is_file() and not sudoers.is_symlink():
                sudoers = self.path(str(sudoers.relative_to(self.target)))
                contents = sudoers.read_text()
                cleaned = "".join(line for line in contents.splitlines(keepends=True)
                                  if not re.match(rf"\s*(?:{re.escape(live_user)}|%wheel)\s+.*NOPASSWD", line))
                if cleaned != contents:
                    self.atomic(sudoers, cleaned, 0o440)
        # The DDM RPM owns its /usr/share schema/defaults. Only remove a Live
        # account's /etc override; runtime DConfig caches are excluded on copy.
        for path in self.target.glob("etc/dsg/configs/overrides/**/*.json"):
            if path.is_file() and not path.is_symlink():
                path = self.path(str(path.relative_to(self.target)))
                try:
                    data = json.loads(path.read_text())
                except ValueError:
                    continue
                if not isinstance(data, dict):
                    continue
                settings = data.get("contents", {})
                if not isinstance(settings, dict):
                    continue
                changed = False
                for name, session in (("autologinUser", "autologinSession"), ("lastUser", "lastSession")):
                    setting = settings.get(name)
                    if isinstance(setting, dict) and setting.get("value") == live_user:
                        setting["value"] = ""
                        if isinstance(settings.get(session), dict):
                            settings[session]["value"] = ""
                        changed = True
                if changed:
                    self.atomic(path, json.dumps(data, indent=2) + "\n", stat.S_IMODE(path.stat().st_mode))
        for relative in ("etc/machine-info", ".liveimg-configured"):
            path = self.path(relative)
            if path.is_file() or path.is_symlink():
                path.unlink()
        self.save()

    def configure_fedora_pam(self) -> None:
        from fedora_profile import state_profile
        from fedora_system import verify_fedora_pam

        # Keep the DDM RPM's files and the ISO RPM's optional Fedora bridge
        # package-managed. The explicit DDM 5 profile uses password login only.
        verify_fedora_pam(self.target, state_profile(self.state)["pam"])

    def install_kernels(self) -> None:
        from fedora_boot import install_kernels
        install_kernels(self)

    def efi_file(self, package: str, name: str, root: Path | None = None) -> Path:
        from fedora_boot import efi_file
        return efi_file(self, package, name, root)

    def install_bootloader(self) -> None:
        from fedora_boot import install_bootloader
        install_bootloader(self)

    def system(self) -> None:
        self.load("copied", "system-configuring")
        self.verify_target()
        self.save("system-configuring")
        self.write_fstab()
        self.bind_runtime()
        self.clean_live_settings()
        self.configure_fedora_pam()
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
        from fedora_profile import state_profile
        from fedora_system import console_font, console_keymap

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
        legacy_pam = state_profile(self.state)["pam"] == "ddm-system-login"
        require(not legacy_pam or user != "nopasswdlogin", "Choose a username outside DDM's passwordless-login group.")
        require(not self.state.get("user") or self.state["user"] == user,
                "An account was already created; retry configuration with the same username.")
        zone = self.path("usr/share/zoneinfo/" + account["timezone"])
        require(zone.is_file(), "The requested timezone is not available in the target.")
        available = self.chroot("locale", "-a").stdout.lower().replace("utf-8", "utf8")
        require(all(account[key].lower().replace("utf-8", "utf8") in available.splitlines()
                    for key in ("locale", "region_locale")),
                "The Live image lacks the selected language pack. Include the matching glibc-langpack in the ISO.")
        keymap = console_keymap(self.target, account["layout"], account["variant"])
        font = console_font(self.target, account["locale"])
        ntp = self.config.get("DI_IS_ENABLE_NTP", "true").lower()
        require(ntp in {"true", "false"}, "Choose whether network time synchronization should be enabled.")
        session = self.config.get("DI_DDE_SESSION", "treeland")
        require(bool(re.fullmatch(r"[A-Za-z0-9_-]+", session))
                and self.path(f"usr/share/wayland-sessions/{session}.desktop").is_file(),
                "The configured DDE Wayland session is missing.")
        home = self.path(f"home/{user}")
        require(not home.is_symlink(), "The selected user's home directory must not be a symbolic link.")
        self.save("configuring-user")
        existing = self.chroot("getent", "passwd", user, check=False)
        require(existing.returncode != 0 or self.state.get("user") == user or user == self.state.get("live_user"),
                "That account already exists in the image; choose another username.")
        # Free the Live account's UID before useradd, so an otherwise ordinary
        # image creates the first installed user at Fedora's UID_MIN. Do not use
        # userdel --remove: a retained /home may contain unrelated existing data.
        live_user = self.state.get("live_user", "liveuser")
        if live_user != user or self.state.get("user") != user:
            if self.chroot("getent", "passwd", live_user, check=False).returncode == 0:
                self.chroot("userdel", live_user)
            for relative in (f"var/lib/AccountsService/users/{live_user}",
                             f"var/lib/AccountsService/deepin/users/{live_user}"):
                path = self.path(relative)
                if path.is_file() or path.is_symlink():
                    path.unlink()
            if live_user == user:
                # Reusing the Live username still creates a regular account,
                # without its temporary group memberships or Live GECOS text.
                existing = self.chroot("getent", "passwd", user, check=False)
        self.state["user"] = user
        self.state.setdefault("user_home_created", not home.exists())
        self.state["keyboard"] = {"layout": account["layout"], "variant": account["variant"],
                                  "keymap": keymap, "font": font}
        self.save()
        if existing.returncode:
            self.chroot("useradd", "--create-home", "--user-group", "--shell", "/bin/bash", "--groups", "wheel", user)
        else:
            self.chroot("usermod", "--append", "--groups", "wheel", "--shell", "/bin/bash", user)
        if legacy_pam:
            groups = self.chroot("id", "--name", "--groups", user).stdout.split()
            require(bool(groups) and "nopasswdlogin" not in groups,
                    "The installed user must not belong to DDM's passwordless-login group.")
        self.chroot("install", "-d", "-m", "0700", "-o", user, "-g", user, f"/home/{user}")
        self.chroot("chpasswd", input_text=f"{user}:{account['password']}\n")
        # As in Anaconda, an unset password-change date avoids a broken RTC
        # turning a new account into a mandatory password reset at first login.
        self.chroot("chage", "-d", "", user)
        self.chroot("passwd", "--lock", "root")
        self.chroot("chage", "-d", "", "root")
        self.write("etc/hostname", account["hostname"] + "\n")
        # Fedora's nss-myhostname resolves the configured hostname. Preserve
        # packaged/custom aliases rather than replacing hosts with a new table.
        if not self.path("etc/hosts").exists():
            self.write("etc/hosts", "127.0.0.1 localhost localhost.localdomain localhost4 localhost4.localdomain4\n"
                       "::1 localhost localhost.localdomain localhost6 localhost6.localdomain6\n")
        locale_text = f"LANG={account['locale']}\n" + "".join(
            f"{key}={account['region_locale']}\n"
            for key in ("LC_TIME", "LC_NUMERIC", "LC_MONETARY", "LC_MEASUREMENT", "LC_PAPER"))
        self.write("etc/locale.conf", locale_text)
        localtime = self.path("etc/localtime")
        if localtime.exists() or localtime.is_symlink():
            localtime.unlink()
        localtime.symlink_to("../usr/share/zoneinfo/" + account["timezone"])
        adjtime = self.path("etc/adjtime")
        clock_lines = adjtime.read_text().splitlines() if adjtime.is_file() and not adjtime.is_symlink() else []
        clock_lines = clock_lines[:2] if len(clock_lines) >= 2 else ["0.0 0 0.0", "0"]
        self.write("etc/adjtime", "\n".join(clock_lines + ["UTC"]) + "\n")
        self.write("etc/vconsole.conf", f'KEYMAP="{keymap}"\nFONT="{font}"\n')
        self.write("etc/X11/xorg.conf.d/00-keyboard.conf", 'Section "InputClass"\n Identifier "system-keyboard"\n'
                   ' MatchIsKeyboard "on"\n' + f' Option "XkbLayout" "{account["layout"]}"\n'
                   + f' Option "XkbVariant" "{account["variant"]}"\n'
                   + (' Option "XkbOptions" "grp:alt_shift_toggle"\n' if "," in account["layout"] else "")
                   + 'EndSection\n')
        self.write("etc/sudoers.d/90-fedora-dde-admin", "%wheel ALL=(ALL) ALL\n", 0o440)
        self.chroot("visudo", "--check")
        settings = ("[User]\nSystemAccount=false\n" + f"Language={account['locale']}\n"
                    + f"XSession={session}\nSession={session}\nSessionType=wayland\n")
        self.write(f"var/lib/AccountsService/users/{user}", settings, 0o600)
        self.write(f"var/lib/AccountsService/deepin/users/{user}", settings + f"Locale={account['locale']}\n"
                   + f"Layout={account['layout']};{account['variant']}\n", 0o600)
        self.write(f"home/{user}/.config/locale.conf", locale_text, 0o600)
        self.chroot("chown", f"{user}:{user}", f"/home/{user}/.config", f"/home/{user}/.config/locale.conf")
        if self.path("usr/lib/systemd/system/chronyd.service").is_file():
            self.run("systemctl", "--root", str(self.target), "enable" if ntp == "true" else "disable", "chronyd.service")
        if self.path("usr/lib/systemd/system/initial-setup.service").is_file():
            self.run("systemctl", "--root", str(self.target), "disable", "initial-setup.service")
        reconfigure = self.path("etc/reconfigSys")
        if reconfigure.is_file() or reconfigure.is_symlink():
            reconfigure.unlink()
        # The final initramfs must include the chosen console layout and locale.
        # These steps are retryable and leave the Live session and mounts intact.
        self.install_kernels()
        self.install_bootloader()
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
        from fedora_profile import selinux_disabled
        from fedora_system import target_path, verify_selinux_config

        if selinux_disabled(self.state):
            verify_selinux_config(self.target, "disabled")
            return

        config = self.path("etc/selinux/config")
        policy = re.search(r"(?m)^\s*SELINUXTYPE\s*=\s*([A-Za-z0-9_-]+)\s*(?:#.*)?$",
                           config.read_text()) if config.is_file() and not config.is_symlink() else None
        policy_type = policy[1] if policy else "targeted"
        contexts = self.path(f"etc/selinux/{policy_type}/contexts/files/file_contexts")
        require(contexts.is_file(), "The Fedora payload must include selinux-policy-targeted.")
        # Follow Anaconda's targeted restorecon pass without -F: keep custom
        # SELinux users/ranges, never walk the Live /dev or /sys binds, and do not
        # recursively relabel a retained /home. Payload labels came via rsync -X.
        candidates = ("/boot", "/etc", "/root", "/usr/lib", "/usr/lib64",
                      "/var/cache/yum", "/var/lib", "/var/log", "/var/opt", "/var/spool", "/var/srv")
        paths = [path for path in candidates if target_path(self.target, path.lstrip("/")).exists()]
        self.chroot("restorecon", "-ir", "-e", "/boot/efi", "-e", "/dev", "-e", "/proc",
                    "-e", "/sys", "-e", "/run", *paths, capture=False)
        user = self.state.get("user")
        if user:
            home = f"/home/{user}"
            if self.state.get("user_home_created"):
                self.chroot("restorecon", "-ir", home, capture=False)
            else:
                # Only the entries this installer creates in an existing home.
                self.chroot("restorecon", "-i", home, home + "/.config",
                            home + "/.config/locale.conf", capture=False)

    def finalize(self) -> None:
        from fedora_profile import selinux_disabled, state_profile
        from fedora_system import verify_selinux_config

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
            if selinux_disabled(self.state):
                verify_selinux_config(self.target, "disabled")
            self.write("var/log/fedora-dde-installer/install-summary.json", json.dumps({
                "installation_id": self.state["installation_id"], "architecture": self.state["architecture"],
                "uefi": self.state["uefi"], "kernels": self.state.get("kernels", []),
                "firmware_fallback": self.state.get("firmware_fallback", False),
                "install_profile": state_profile(self.state),
                "keyboard": self.state.get("keyboard", {}), "status": "configured"}, indent=2) + "\n", 0o600)
            if not selinux_disabled(self.state):
                self.chroot("restorecon", "-ir", "/var/log/fedora-dde-installer", capture=False)
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
