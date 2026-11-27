"""LUKS2 root installation with an anonymous passphrase descriptor.

The partition helper only creates the physical partition. No secret enters its
JSON plan, the installer configuration, argv, environment, logs or a target file.
"""

from __future__ import annotations

import fcntl
import json
import os
from pathlib import Path
import re
import shutil
import stat
import uuid

from fedora_backend import DEVICE_RE, ROOT_FILESYSTEMS, InstallError, require


SECRET_FD_ENV = "FEDORA_DDE_CRYPT_FD"
SECRET_MEMFD_NAME = "fedora-dde-crypt-passphrase"
MIN_SECRET_BYTES = 8
MAX_SECRET_BYTES = 1024
# Linux UAPI values are stable across supported architectures. Some minimal
# Python builds omit these exported names even when the kernel supports seals.
F_ADD_SEALS = getattr(fcntl, "F_ADD_SEALS", 1033)
F_GET_SEALS = getattr(fcntl, "F_GET_SEALS", 1034)
REQUIRED_SEALS = (getattr(fcntl, "F_SEAL_SEAL", 1) | getattr(fcntl, "F_SEAL_SHRINK", 2)
                  | getattr(fcntl, "F_SEAL_GROW", 4) | getattr(fcntl, "F_SEAL_WRITE", 8))
# Explicitly reserve 32 MiB, including alignment/keyslot headroom, rather than
# assuming cryptsetup's default (Blivet currently budgets 16 MiB). --offset uses
# 512-byte sectors even on disks with 4096-byte logical sectors.
LUKS_HEADER_BYTES = 32 * 1024 * 1024
LUKS_OFFSET_SECTORS = LUKS_HEADER_BYTES // 512
CRYPT_OPTIONS = "luks,x-initrd.attach"


def encrypted_root(plan: list[dict]) -> dict | None:
    selected = []
    active = []
    for item in plan:
        require(not item.get("cryptPassword"), "Passphrases must not appear in the partition plan.")
        if item.get("operate") in {"normal", "delete"}:
            continue
        require(type(item.get("isCrypt", False)) is bool, "Invalid partition encryption flag.")
        require(not item.get("isCryptHeader") and not item.get("isNeedCryptHeader"),
                "Detached and hidden encryption headers are not supported.")
        if item.get("type") == "partition":
            active.append(item)
        if item.get("isCrypt"):
            require(item.get("type") == "partition" and item.get("mountPoint") == "/"
                    and item.get("filesystem") in ROOT_FILESYSTEMS
                    and item.get("isneedformat") is True,
                    "Encryption requires a newly formatted Btrfs, ext4 or XFS root partition.")
            selected.append(item)
    require(len(selected) <= 1, "Select exactly one encrypted root partition.")
    if not selected:
        return None
    require(all(item.get("mountPoint", "") in {"", "/", "/boot", "/boot/efi"}
                and item.get("filesystem") not in {"swap", "linux-swap"} for item in active),
            "Encrypted installation cannot use separate plaintext home, data or disk-swap partitions. "
            "Keep those paths inside the encrypted root; Btrfs creates root and home subvolumes there.")
    return selected[0]


def read_passphrase(environ: dict | None = None) -> str:
    """Read a sealed anonymous memfd, without advancing its shared offset."""
    value = (os.environ if environ is None else environ).get(SECRET_FD_ENV, "")
    require(isinstance(value, str) and re.fullmatch(r"[0-9]{1,7}", value) and int(value) >= 3,
            "Enter and confirm an encryption passphrase in this Live session.")
    descriptor = int(value)
    try:
        metadata = os.fstat(descriptor)
        seals = fcntl.fcntl(descriptor, F_GET_SEALS)
        name = os.readlink(f"/proc/self/fd/{descriptor}")
        require(stat.S_ISREG(metadata.st_mode) and metadata.st_nlink == 0
                and metadata.st_uid == os.geteuid() and seals & REQUIRED_SEALS == REQUIRED_SEALS
                and name == f"/memfd:{SECRET_MEMFD_NAME} (deleted)",
                "The encryption passphrase must use the installer's sealed anonymous descriptor.")
        require(MIN_SECRET_BYTES <= metadata.st_size <= MAX_SECRET_BYTES,
                "Use an encryption passphrase of 8 to 1024 UTF-8 bytes.")
        data = os.pread(descriptor, metadata.st_size + 1, 0)
        require(len(data) == metadata.st_size and not any(byte in data for byte in (b"\0", b"\r", b"\n")),
                "The encryption passphrase must be a single line without NUL characters.")
        return data.decode("utf-8")
    except (OSError, UnicodeError) as error:
        raise InstallError("The private encryption passphrase channel is unavailable or invalid.") from error


def verify_requirements(backend, source: Path, plan: list[dict], source_state: dict) -> None:
    from fedora_profile import selinux_disabled
    from fedora_system import target_path, verify_selinux_config

    root = encrypted_root(plan)
    if root is None:
        return
    require(not selinux_disabled(source_state), "Encrypted installation requires Fedora SELinux Enforcing.")
    verify_selinux_config(source, "enforcing")
    for command in ("cryptsetup", "mkfs." + root["filesystem"]):
        require(shutil.which(command) is not None, f"The Live image is missing {command} for encrypted installation.")
    for command in ("cryptsetup", "lsinitrd", "systemd-ask-password", "systemd-tty-ask-password-agent"):
        require(any(target_path(source, directory + command).is_file() for directory in ("usr/bin/", "usr/sbin/")),
                f"The target payload must include {command} for encrypted boot.")
    # Dracut 111 renumbered these modules (70crypt, 71systemd-cryptsetup,
    # 11systemd-ask-password). Check the exact module suffix, not a release's
    # numeric ordering. crypt alone does not install systemd's unlock service.
    modules = target_path(source, "usr/lib/dracut/modules.d")
    for module in ("crypt", "systemd-cryptsetup", "systemd-ask-password"):
        require(any(target_path(source, str(path.relative_to(source))).is_file()
                    for path in modules.glob(f"[0-9][0-9]{module}/module-setup.sh")),
                f"The target payload is missing dracut's {module} module.")
    require(target_path(source, "usr/lib/systemd/systemd-cryptsetup").is_file()
            and target_path(source, "usr/lib/systemd/system-generators/systemd-cryptsetup-generator").is_file(),
            "The target payload must include systemd-cryptsetup for boot-time password entry.")


def _record_valid(record: dict) -> None:
    require(isinstance(record, dict), "Invalid encryption state.")
    identifier = record.get("uuid", "")
    try:
        canonical = str(uuid.UUID(identifier))
    except (ValueError, TypeError, AttributeError) as error:
        raise InstallError("Invalid LUKS UUID in installation state.") from error
    require(identifier == canonical and record.get("name") == "luks-" + identifier
            and record.get("mapper") == "/dev/mapper/luks-" + identifier
            and isinstance(record.get("device"), str) and DEVICE_RE.fullmatch(record["device"])
            and ".." not in Path(record["device"]).parts
            and not record["device"].startswith(("/dev/mapper/", "/dev/dm-"))
            and record.get("fstype") in ROOT_FILESYSTEMS, "Invalid encrypted root metadata.")


def root_record(state: dict) -> dict | None:
    record = state.get("encrypted_root")
    if record is None:
        return None
    _record_valid(record)
    root = next((item for item in state.get("mounts", []) if item["mountpoint"] == "/"), None)
    require(root is not None and root["device"] == record["mapper"],
            "The mounted root does not match this installation's encrypted device.")
    require(root.get("fstype") == record["fstype"], "The encrypted root filesystem type changed.")
    require(not record.get("filesystem_uuid") or record["filesystem_uuid"] == root.get("uuid"),
            "The encrypted root filesystem UUID changed.")
    require(all(item["mountpoint"] in {"/", "/boot", "/boot/efi"}
                or item["mountpoint"] == "/home" and item["device"] == record["mapper"]
                for item in state.get("mounts", [])), "An encrypted installation contains an unprotected data mount.")
    return record


def _verify_partition(backend, mapping: dict, planned: dict) -> None:
    """Match the actual physical partition to reviewed byte geometry, not just a disk."""
    from fedora_space import capacity

    device = mapping["device"]
    require(not device.startswith(("/dev/mapper/", "/dev/dm-")) and stat.S_ISBLK(os.stat(device).st_mode),
            "Encrypted root creation requires an ordinary physical partition.")
    parent = backend.run("lsblk", "-nro", "PKNAME", "--paths", device).stdout.strip()
    require(parent == planned["device"], "The encrypted root is outside its reviewed disk.")
    # Deleting an earlier MBR logical partition can legitimately renumber this
    # partition. Match its disk and exact reviewed byte geometry below instead
    # of requiring the pre-edit /dev path to survive unchanged.
    require(not backend.run("lsblk", "--noheadings", "--raw", "--output", "MOUNTPOINTS", device).stdout.strip(),
            "The encrypted root partition is already in use.")
    properties = backend.run("udevadm", "info", "--query=property", "--name", device).stdout
    number = next((line.partition("=")[2] for line in properties.splitlines()
                   if line.startswith("ID_PART_ENTRY_NUMBER=")), "")
    require(number.isdigit(), "Cannot identify the encrypted root partition number.")
    table = backend.run("parted", "--machine", "--script", parent, "unit", "B", "print").stdout
    geometry = [tuple(map(int, match.groups())) for line in table.splitlines()
                if (match := re.match(r"^([0-9]+):([0-9]+)B:([0-9]+)B:([0-9]+)B:", line))
                and match[1] == number]
    size = capacity(planned)
    require(len(geometry) == 1 and geometry[0][1:] ==
            (int(planned["startPoint"]) * int(planned["sectorSize"]),
             (int(planned["endPoint"]) + 1) * int(planned["sectorSize"]) - 1, size),
            "The helper's encrypted root partition does not match the confirmed geometry.")


def verify_mapping(backend, record: dict) -> bool:
    """Verify ownership before formatting, mounting or closing a mapper."""
    _record_valid(record)
    result = backend.run("cryptsetup", "status", record["name"], check=False)
    if result.returncode:
        require(not Path(record["mapper"]).exists() and result.returncode == 4,
                "Cannot verify the encrypted mapper; it has not been closed.")
        return False
    values = {line.partition(":")[0].strip(): line.partition(":")[2].strip()
              for line in result.stdout.splitlines() if ":" in line}
    require(values.get("type") == "LUKS2" and values.get("device")
            and Path(values["device"]).resolve() == Path(record["device"]).resolve(),
            "An encrypted mapper was replaced; refusing to use or close it.")
    actual = backend.run("cryptsetup", "luksUUID", record["device"]).stdout.strip()
    require(actual == record["uuid"], "The encrypted partition UUID changed.")
    return True


def create_root(backend, mappings: list[dict], plan: list[dict], secret: str) -> None:
    planned = encrypted_root(plan)
    if planned is None:
        return
    points = {item.get("mountPoint") for item in plan if item.get("type") == "partition"
              and item.get("operate") not in {"normal", "delete"} and item.get("mountPoint")}
    require({item["mountpoint"] for item in mappings} == points,
            "The helper returned a filesystem outside the reviewed encrypted layout.")
    mapping = next(item for item in mappings if item["mountpoint"] == "/")
    _verify_partition(backend, mapping, planned)
    require(not backend.state.get("encrypted_root") and not backend.state.get("owned_mappers"),
            "Encryption was already started; automatic reformatting is forbidden.")
    identifier = str(uuid.uuid4())
    record = {"device": mapping["device"], "uuid": identifier, "name": "luks-" + identifier,
              "mapper": "/dev/mapper/luks-" + identifier, "fstype": planned["filesystem"]}
    require(not Path(record["mapper"]).exists(), "The new encrypted mapper name is already occupied.")
    backend.state["encrypted_root"] = record
    backend.save()  # Record destructive intent, never the secret, before luksFormat.
    backend.run("cryptsetup", "luksFormat", "--batch-mode", "--type", "luks2", "--pbkdf", "argon2id",
                "--offset", str(LUKS_OFFSET_SECTORS), "--uuid", identifier, "--key-file", "-",
                record["device"], input_text=secret)
    metadata = json.loads(backend.run("cryptsetup", "luksDump", "--dump-json-metadata", record["device"]).stdout)
    segments = metadata.get("segments", {}) if isinstance(metadata, dict) else {}
    require(isinstance(segments, dict) and len(segments) == 1
            and isinstance(next(iter(segments.values())), dict)
            and next(iter(segments.values())).get("offset") == str(LUKS_HEADER_BYTES),
            "The LUKS2 data offset does not match the space reserved by this installation.")
    require(backend.run("cryptsetup", "luksUUID", record["device"]).stdout.strip() == identifier,
            "The newly formatted LUKS UUID does not match this installation.")
    backend.state.setdefault("owned_mappers", []).append(dict(record))
    backend.save()  # Track even an interrupted open attempt for safe cleanup.
    backend.run("cryptsetup", "open", "--type", "luks2", "--key-file", "-", record["device"],
                record["name"], input_text=secret)
    backend.run("udevadm", "settle")
    require(verify_mapping(backend, record), "The new encrypted root mapper did not open.")
    require(stat.S_ISBLK(os.stat(record["mapper"]).st_mode), "The encrypted mapper is not a block device.")
    force = "-F" if record["fstype"] == "ext4" else "-f"
    backend.run("mkfs." + record["fstype"], force, record["mapper"])
    backend.run("udevadm", "settle")
    mapping["device"] = record["mapper"]
    backend.save()


def close_owned_mappers(backend) -> None:
    for record in list(reversed(backend.state.get("owned_mappers", []))):
        if verify_mapping(backend, record):
            backend.run("cryptsetup", "close", record["name"])
        backend.state["owned_mappers"].remove(record)
        backend.save()


def cleanup_partition_failure(backend) -> None:
    """Release this attempt's mounts before its mappers; never force a busy close."""
    temporary = backend.state.get("temporary_mount")
    points = list(reversed(backend.state.get("owned_mounts", [])))
    if temporary:
        require(Path(temporary) == backend.state_dir / "btrfs-top-level", "Invalid temporary mount state.")
        points.insert(0, temporary)
    for point in points:
        require(".." not in Path(point).parts and (point == temporary or Path(point).is_relative_to(backend.target)),
                "Unsafe installation mount state.")
        if os.path.ismount(point):
            expected = (backend.state["encrypted_root"]["mapper"] if point == temporary else
                        next((item["device"] for item in backend.state.get("mounts", [])
                              if backend.target / item["mountpoint"].lstrip("/") == Path(point)), ""))
            actual = backend.run("findmnt", "--noheadings", "--raw", "--mountpoint", point,
                                 "--output", "SOURCE").stdout.strip().split("[", 1)[0]
            require(expected and actual and Path(expected).resolve() == Path(actual).resolve(),
                    "An installation mount was replaced; refusing to unmount it during encryption cleanup.")
            backend.run("umount", point)
        if point == temporary:
            backend.state.pop("temporary_mount", None)
        else:
            backend.state["owned_mounts"].remove(point)
        backend.save()
    close_owned_mappers(backend)


def crypttab(state: dict) -> str:
    record = root_record(state)
    if record is None:
        return "# No encrypted devices are configured by this installer.\n"
    return f"{record['name']} UUID={record['uuid']} none {CRYPT_OPTIONS}\n"


def verify_initramfs(backend, relative: str) -> None:
    record = root_record(backend.state)
    if record is None:
        return
    image = "/" + relative
    modules = set(backend.chroot("lsinitrd", "--mod", image).stdout.splitlines())
    for module in ("crypt", "systemd-cryptsetup", "systemd-ask-password"):
        require(module in {line.strip() for line in modules}, f"The target initramfs lacks the {module} module.")
    listing = backend.chroot("lsinitrd", image).stdout
    for path in ("usr/lib/systemd/systemd-cryptsetup",
                 "usr/lib/systemd/system-generators/systemd-cryptsetup-generator",
                 "usr/bin/systemd-ask-password", "usr/bin/systemd-tty-ask-password-agent",
                 "usr/lib/systemd/system/systemd-ask-password-console.service"):
        require(re.search(r"(?m)(?:^|\s)(?:\./)?" + re.escape(path) + r"(?:\s+->\s+[^\n]*)?$", listing),
                f"The target initramfs lacks the password unlock component {path}.")
    contents = backend.chroot("lsinitrd", "--file", "etc/crypttab", image).stdout
    rows = [line.split() for line in contents.splitlines() if line.strip() and not line.lstrip().startswith("#")]
    # Fedora's crypt module resolves UUID= to a /dev/disk/by-uuid path when
    # filtering crypttab for a hostonly image. Both forms identify the same LUKS.
    require(len(rows) == 1 and len(rows[0]) == 4 and rows[0][0] == record["name"]
            and rows[0][1] in {"UUID=" + record["uuid"], "/dev/disk/by-uuid/" + record["uuid"]}
            and rows[0][2] == "none" and set(rows[0][3].split(",")) == set(CRYPT_OPTIONS.split(",")),
            "The target initramfs lacks the password-only encrypted root configuration.")
