"""Read-only Fedora Live payload selection and installation.

Use the same base-device discovery and filesystem copy boundaries as Fedora
45 Anaconda. The running Live overlay is deliberately never an install source.
"""

from __future__ import annotations

from collections import deque
import json
import os
from pathlib import Path
import re
import stat

from fedora_backend import InstallError, require


SOURCE_FILESYSTEMS = {"squashfs", "erofs", "ext4", "ext3", "ext2", "xfs", "btrfs"}


def rooted_path(root: Path, relative: str) -> Path:
    """Resolve image symlinks inside the image, including absolute symlinks."""
    pending = deque(Path(relative.lstrip("/")).parts)
    resolved: list[str] = []
    links = 0
    while pending:
        part = pending.popleft()
        if part == ".":
            continue
        if part == "..":
            require(bool(resolved), "An image symlink escapes its root.")
            resolved.pop()
            continue
        current = root.joinpath(*resolved, part)
        if current.is_symlink():
            links += 1
            require(links <= 40, "Too many image symlinks.")
            target = Path(os.readlink(current))
            if target.is_absolute():
                resolved.clear()
            pending.extendleft(reversed(target.parts[1:] if target.is_absolute() else target.parts))
        else:
            resolved.append(part)
    return root.joinpath(*resolved)


def mount_info(backend, path: Path) -> dict:
    result = backend.run("findmnt", "--json", "--mountpoint", str(path),
                         "--output", "SOURCE,MAJ:MIN,FSTYPE,UUID,OPTIONS,FSROOT")
    entries = json.loads(result.stdout).get("filesystems", [])
    require(len(entries) == 1, "The Live base must be a distinct mounted filesystem.")
    entry = entries[0]
    require(entry.get("fstype") in SOURCE_FILESYSTEMS
            and "ro" in entry.get("options", "").split(","),
            "The installation source must be a read-only Fedora base image, not the running Live overlay.")
    return {key: entry.get(key) for key in ("source", "maj:min", "fstype", "uuid", "fsroot")}


def validate_root(backend, source: Path, *, expected_profile: dict | None = None) -> dict:
    from fedora_profile import read_profile

    require(source.is_absolute() and source.is_dir() and not source.is_symlink(), "Invalid Fedora source root.")
    require(source.resolve() != Path("/") and not source.resolve().is_relative_to(backend.target.resolve()),
            "The running system and the installation target cannot be used as an image source.")
    identity = mount_info(backend, source)
    profile = read_profile(source)
    if expected_profile is not None:
        require(profile == expected_profile, "The installation image profile changed after preflight.")
    metadata = rooted_path(source, "etc/os-release")
    require(metadata.is_file(), "The source has no operating system metadata.")
    values = {}
    for line in metadata.read_text().splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            values[key] = value.strip('\"\'')
    require(values.get("ID") == "fedora" and values.get("VERSION_ID") == "45",
            "This installer requires a Fedora 45 Live base image.")
    require(not rooted_path(source, "ostree/repo").is_dir(), "OSTree deployment roots are not supported.")
    require(any((source / "usr/lib/modules").glob("*/vmlinuz"))
            or any((source / "usr/lib/modules").glob("*/vmlinuz-dtbloader.efi")),
            "The image does not contain a supported installed kernel.")
    commands = ("rpm", "dnf", "systemctl", "dracut", "chpasswd", "useradd", "userdel", "usermod", "chage",
                "grub2-mkconfig", "grub2-editenv", "grubby", "kernel-install", "systemd-machine-id-setup", "depmod")
    if profile["selinux"] == "enforcing":
        commands += ("restorecon",)
    if profile["pam"] == "ddm-system-login":
        commands += ("id",)
    for name in commands:
        require(any(rooted_path(source, prefix + name).is_file() for prefix in ("usr/bin/", "usr/sbin/")),
                f"The Live base image is missing {name}; include it when composing the ISO.")
    require(rooted_path(source, "usr/lib/systemd/zram-generator.conf").is_file()
            or rooted_path(source, "etc/systemd/zram-generator.conf").is_file(),
            "Include zram-generator-defaults in the Live image for Fedora's default swap policy.")
    from fedora_system import verify_fedora_pam, verify_selinux_config, verify_selinux_source
    verify_fedora_pam(source, profile["pam"])
    if profile["selinux"] == "enforcing":
        verify_selinux_source(source)
    from fedora_boot import verify_kernel_hooks
    verify_kernel_hooks(source)
    verify_selinux_config(source, profile["selinux"])
    return identity


def detect_live_device(backend) -> str:
    cmdline = Path("/proc/cmdline").read_text().split()
    require(any(x == "rd.live.image" or x.startswith("root=live:") for x in cmdline),
            "Automatic image detection requires a Fedora Live boot.")
    device = None
    for candidate in (Path("/dev/mapper/live-base"), Path("/dev/mapper/live-osimg-min")):
        if candidate.exists() and stat.S_ISBLK(candidate.stat().st_mode):
            device = str(candidate)
            break
    if device is None:
        base = Path("/run/rootfsbase")
        require(base.is_dir(), "Cannot locate the original Fedora Live base image.")
        result = backend.run("findmnt", "--noheadings", "--raw", "--mountpoint", str(base), "--output", "SOURCE")
        device = result.stdout.strip().split("[", 1)[0]
        require(device.startswith("/dev/") and Path(device).exists() and stat.S_ISBLK(Path(device).stat().st_mode),
                "The Fedora Live base is not backed by a block device.")
    return device


def prepare_source(backend) -> dict:
    from fedora_profile import read_profile

    configured = backend.config.get("DI_SOURCE_ROOT", "auto")
    if configured not in ("", "auto"):
        source = Path(configured)
        return {"source": str(source), "source_identity": validate_root(backend, source),
                "install_profile": read_profile(source),
                "source_mount_owned": False, "source_mounted_this_call": False}
    device = detect_live_device(backend)
    backend.state_dir.mkdir(mode=0o700, parents=True, exist_ok=True)
    require(not backend.state_dir.is_symlink(), "The installer runtime directory is unsafe.")
    os.chmod(backend.state_dir, 0o700)
    source = backend.state_dir / "source"
    require(not source.is_symlink(), "The image mount point must not be a symlink.")
    source.mkdir(mode=0o700, exist_ok=True)
    created = not os.path.ismount(source)
    if created:
        backend.run("mount", "--read-only", "--options", "nosuid,nodev", device, str(source))
    try:
        identity = validate_root(backend, source)
        require(Path(identity["source"].split("[", 1)[0]).resolve() == Path(device).resolve(),
                "Another device occupies the image mount point.")
    except Exception:
        if created:
            backend.run("umount", str(source), check=False)
        raise
    return {"source": str(source), "source_identity": identity,
            "install_profile": read_profile(source),
            "source_mount_owned": True, "source_mounted_this_call": created}


def verify_source(backend) -> None:
    from fedora_profile import state_profile

    require(validate_root(backend, Path(backend.state["source"]), expected_profile=state_profile(backend.state))
            == backend.state.get("source_identity"),
            "The installation image was unmounted or replaced.")


def release_source(backend) -> None:
    if backend.state.get("source_mount_owned") and not backend.state.get("source_released"):
        verify_source(backend)
        backend.run("umount", backend.state["source"])
        backend.state["source_released"] = True
        backend.save()


def copy_payload(backend) -> None:
    from fedora_profile import PROFILE_PATH, selinux_disabled, state_profile

    verify_source(backend)
    source = Path(backend.state["source"])
    exclusions = ["/dev/***", "/proc/***", "/sys/***", "/run/***", "/tmp/***", "/mnt/***", "/media/***",
                  "/target/***", "/lost+found", "/etc/machine-id", "/etc/machine-info",
                  "/var/lib/dbus/machine-id", "/var/lib/systemd/random-seed", "/etc/ssh/ssh_host_*",
                  "/etc/fedora-dde-installer/***",
                  "/boot/*rescue*", "/boot/loader/***", "/boot/efi/***", "/boot/grub2/***",
                  "/etc/sysconfig/***", "/usr/lib/grub/***"]
    live_user = backend.config.get("LIVE_USERNAME", "liveuser")
    require(bool(re.fullmatch(r"[a-z_][a-z0-9_-]{0,31}", live_user)), "Invalid Live username.")
    exclusions.append(f"/home/{live_user}/***")
    command = ["rsync", "-aHAX", "--numeric-ids", "--one-file-system", "--info=progress2", "--no-inc-recursive"]
    if selinux_disabled(backend.state):
        # Custom xattr rules replace rsync's default namespace filtering.
        # Keep capabilities and ACLs; exclude only SELinux and the default
        # system namespace (ACLs are handled independently by -A).
        command += ["--filter=-x system.*", "--filter=-x security.selinux"]
    command += [f"--exclude={item}" for item in exclusions]
    command += [str(source) + "/", str(backend.target) + "/"]
    backend.runner.run(command, capture=False)
    # Fedora 45 Anaconda handles FAT and KIWI xattr-sensitive directories
    # separately. Do not strip valid RPM-owned /var or /root contents.
    for relative in ("boot/efi", "boot/grub2", "etc/sysconfig", "usr/lib/grub"):
        origin = rooted_path(source, relative)
        if not origin.is_dir():
            continue
        destination = backend.path(relative)
        require(not destination.is_symlink(), "The payload destination must not be a symlink.")
        destination.mkdir(parents=True, exist_ok=True)
        command = ["rsync", "--no-inc-recursive"]
        if relative == "boot/efi":
            # The boot module installs a new fallback only when nobody already
            # owns that path. Copying the Live fallback here would bypass that
            # ownership check on a reused ESP.
            command += ["-rx", "--exclude=/loader/***", "--exclude=/EFI/BOOT/***"]
        else:
            # KIWI's special directories must not inherit the backend's 0077
            # umask. Preserve their POSIX metadata while omitting xattrs/ACLs,
            # as required by this separate copy path. Restorecon handles labels.
            command += ["-aH", "--numeric-ids", "--one-file-system"]
        backend.runner.run(command + [str(origin) + "/", str(destination) + "/"], capture=False)
    for relative, mode in (("dev", 0o755), ("proc", 0o755), ("sys", 0o755), ("run", 0o755), ("tmp", 0o1777)):
        path = backend.path(relative)
        require(not path.is_symlink(), "The payload contains an unsafe runtime directory.")
        path.mkdir(mode=mode, parents=True, exist_ok=True)
        path.chmod(mode)
    if (source / PROFILE_PATH).is_file():
        # Keep the ISO RPM's declared profile, but never copy runtime settings
        # or secrets from this otherwise excluded directory. Policy continues
        # to come from the already validated state, not from this target copy.
        directory = backend.path("etc/fedora-dde-installer")
        require(not directory.is_symlink(), "The target profile directory must not be a symlink.")
        directory.mkdir(mode=0o755, parents=True, exist_ok=True)
        directory.chmod(0o755)
        backend.write(PROFILE_PATH, json.dumps(state_profile(backend.state), indent=2) + "\n", 0o644)
