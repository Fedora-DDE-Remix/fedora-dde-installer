"""Payload-based capacity checks before partitioning and before each copy.

Never count compression savings. Hard links share data only inside the same
destination filesystem and rsync batch. Retained filesystems are probed read-only
before any partition-table change; retries credit only files rsync can reuse.
"""

from __future__ import annotations

import os
from pathlib import Path
import stat
import tempfile

from fedora_backend import InstallError, require


MIB = 1024 ** 2
GIB = 1024 ** 3
ROOT_FLOOR = 12 * GIB
SKIP_TREES = {"dev", "proc", "sys", "run", "tmp", "mnt", "media", "target", "lost+found"}
COPY_BATCHES = ("boot/efi", "boot/grub2", "etc/sysconfig", "usr/lib/grub")


def planned_mounts(plan: list[dict]) -> dict[str, dict]:
    result = {}
    for item in plan:
        point = item.get("mountPoint")
        if (item.get("type") != "partition" or item.get("operate") in {"normal", "delete"}
                or not point or point == "swap"):
            continue
        require(point not in result, f"Duplicate planned filesystem: {point}")
        result[point] = item
    require("/" in result, "The space check requires a planned root filesystem.")
    return result


def capacity(item: dict) -> int:
    try:
        values = [item[name] for name in ("startPoint", "endPoint", "sectorSize")]
        require(all(not isinstance(value, bool) and int(value) == value for value in values),
                "Partition geometry must use integer sector values.")
        start, end, sector = map(int, values)
    except (KeyError, TypeError, ValueError, OverflowError) as error:
        raise InstallError("The partition plan is missing valid sector geometry.") from error
    require(0 <= start < end and sector in {512, 1024, 2048, 4096, 8192}, "Invalid partition sector geometry.")
    return (end - start + 1) * sector


def reserve(point: str, size: int) -> int:
    base = 2 * GIB if point == "/" else GIB if point in {"/boot", "/var"} else 64 * MIB if point == "/boot/efi" else 512 * MIB
    return max(base, (size + 9) // 10)


def reusable(source: Path, target: Path, relative: Path, metadata: os.stat_result) -> bool:
    current = target
    for component in relative.parts[:-1]:
        current /= component
        if current.is_symlink():
            return False  # Never inspect a retained-home link outside the target.
    destination = target / relative
    try:
        previous = destination.lstat()
        if stat.S_ISREG(metadata.st_mode):
            return (stat.S_ISREG(previous.st_mode) and previous.st_size == metadata.st_size
                    and previous.st_mtime_ns == metadata.st_mtime_ns)
        if stat.S_ISDIR(metadata.st_mode):
            return stat.S_ISDIR(previous.st_mode)
        if stat.S_ISLNK(metadata.st_mode):
            return stat.S_ISLNK(previous.st_mode) and os.readlink(destination) == os.readlink(source)
    except FileNotFoundError:
        return False
    return False


def measure(source: Path, points: set[str], live_user: str, target: Path | None = None) -> dict[str, dict]:
    """Scan metadata only; no source symlink or nested filesystem is followed."""
    require("/" in points and source.is_dir() and not source.is_symlink(), "Invalid payload space-check source.")
    totals = {point: {"payload_bytes": 0, "remaining_bytes": 0} for point in points}
    ordered = sorted(points, key=len, reverse=True)
    device = source.stat().st_dev
    pending = [source]
    seen = {}
    while pending:
        directory = pending.pop()
        try:
            with os.scandir(directory) as entries:
                for entry in entries:
                    path = Path(entry.path)
                    relative = path.relative_to(source)
                    name = relative.as_posix()
                    if (relative.parts[0] in SKIP_TREES or name == "etc/fedora-dde-installer"
                            or name == f"home/{live_user}"):
                        continue
                    metadata = entry.stat(follow_symlinks=False)
                    if metadata.st_dev != device:
                        continue
                    is_directory = stat.S_ISDIR(metadata.st_mode)
                    if is_directory:
                        pending.append(path)
                    if not (is_directory or stat.S_ISREG(metadata.st_mode) or stat.S_ISLNK(metadata.st_mode)):
                        continue
                    absolute = "/" + name
                    point = next(point for point in ordered if point == "/" or absolute == point or absolute.startswith(point + "/"))
                    # Round files and directories to allocation blocks and keep
                    # preallocated data. Compressed/sparse source size is not a promise.
                    size = max(4096, ((metadata.st_size + 4095) // 4096) * 4096, metadata.st_blocks * 512)
                    batch = next((prefix for prefix in COPY_BATCHES
                                  if name == prefix or name.startswith(prefix + "/")), "main")
                    # copy_payload uses separate rsync calls for these trees;
                    # -H cannot share data across calls and FAT uses only -rx.
                    key = ((point, batch, metadata.st_dev, metadata.st_ino)
                           if stat.S_ISREG(metadata.st_mode) and batch != "boot/efi" else (point, name))
                    matches = target is not None and reusable(path, target, relative, metadata)
                    if key not in seen:
                        totals[point]["payload_bytes"] += size
                        totals[point]["remaining_bytes"] += 0 if matches else size
                        seen[key] = (size, not matches)
                    elif not matches and not seen[key][1]:
                        # rsync chooses its own hard-link leader. Only credit a
                        # group when every encountered member already matches;
                        # a matching later alias cannot guarantee zero transfer.
                        totals[point]["remaining_bytes"] += seen[key][0]
                        seen[key] = (seen[key][0], True)
        except OSError as error:
            raise InstallError(f"Cannot measure the read-only payload: {error}") from error
    for point, values in totals.items():
        values["reserve_bytes"] = reserve(point, values["payload_bytes"])
        values["required_bytes"] = values["payload_bytes"] + values["reserve_bytes"]
    return totals


def validate_capacity(plan: list[dict], budgets: dict) -> None:
    mounts = planned_mounts(plan)
    require(set(mounts) == set(budgets), "The partition space budget does not match the confirmed plan.")
    for point, item in mounts.items():
        available = capacity(item)
        required = max(budgets[point]["required_bytes"], ROOT_FLOOR if point == "/" else 0)
        require(available >= required,
                f"The {point} partition needs at least {required / GIB:.2f} GiB for this image and free-space reserve; "
                f"the selected partition has {available / GIB:.2f} GiB.")


def retained_space(backend, plan: list[dict], budgets: dict) -> None:
    """Probe retained filesystems before any other partition can be formatted."""
    for point, item in planned_mounts(plan).items():
        if item.get("isneedformat") is True:
            continue
        device = item.get("path", "")
        require(device and stat.S_ISBLK(os.stat(device).st_mode), f"The retained {point} partition is not a block device.")
        parent = backend.run("lsblk", "-nro", "PKNAME", "--paths", device).stdout.strip()
        require(parent == item["device"], "A retained partition is outside its confirmed installation disk.")
        filesystem = backend.run("blkid", "-s", "TYPE", "-o", "value", device).stdout.strip()
        options = {"ext4": "ro,noload", "ext3": "ro,noload", "ext2": "ro",
                   "xfs": "ro,norecovery", "btrfs": "ro,nologreplay", "vfat": "ro"}
        require(filesystem in options, "The retained filesystem cannot be safely probed read-only.")
        backend.state_dir.mkdir(mode=0o700, parents=True, exist_ok=True)
        require(not backend.state_dir.is_symlink(), "The space-check runtime directory is unsafe.")
        probe = Path(tempfile.mkdtemp(prefix="space-probe-", dir=backend.state_dir))
        mounted = False
        try:
            backend.run("mount", "--types", filesystem, "--options", options[filesystem] + ",nosuid,nodev,noexec", device, str(probe))
            mounted = True
            actual = backend.run("findmnt", "--noheadings", "--mountpoint", str(probe), "--output", "OPTIONS").stdout.strip().split(",")
            require("ro" in actual, "The retained filesystem probe is not read-only.")
            information = os.statvfs(probe)
            available = information.f_bavail * information.f_frsize
            required = budgets[point]["required_bytes"]
            require(available >= required,
                    f"The retained {point} filesystem has only {available / GIB:.2f} GiB free; "
                    f"this image needs {required / GIB:.2f} GiB there. No partitions have been changed.")
        finally:
            if mounted or os.path.ismount(probe):
                try:
                    backend.run("umount", str(probe))
                except InstallError as error:
                    raise InstallError(f"Cannot release the read-only space probe at {probe}; "
                                       "unmount it before retrying. No partitions have been changed.") from error
            probe.rmdir()  # Never recursively delete a mount after unmount failure.


def prepare(backend, source: Path, plan: list[dict]) -> dict:
    budgets = measure(source, set(planned_mounts(plan)), backend.config.get("LIVE_USERNAME", "liveuser"))
    validate_capacity(plan, budgets)
    retained_space(backend, plan, budgets)
    return budgets


def before_copy(backend) -> None:
    budgets = backend.state.get("space_requirements")
    require(isinstance(budgets, dict) and "/" in budgets, "Repeat preflight to measure this image's required disk space.")
    measured = measure(Path(backend.state["source"]), set(budgets), backend.config.get("LIVE_USERNAME", "liveuser"), backend.target)
    groups = {}
    mappings = {item["mountpoint"]: item for item in backend.state["mounts"]}
    for point, values in measured.items():
        require(values["payload_bytes"] == budgets[point]["payload_bytes"], "The payload size changed after preflight.")
        require(point in mappings, f"The space-check filesystem {point} is not mounted.")
        path = backend.target if point == "/" else backend.target / point.lstrip("/")
        information = os.statvfs(path)
        # Different Btrfs subvolumes may have different st_dev values. Resolve
        # the underlying helper device and use its statvfs filesystem identity.
        key = (str(Path(mappings[point]["device"]).resolve()), information.f_fsid)
        group = groups.setdefault(key, {"available": information.f_bavail * information.f_frsize,
                                         "required": 0, "points": []})
        group["available"] = min(group["available"], information.f_bavail * information.f_frsize)
        group["required"] += values["remaining_bytes"] + values["reserve_bytes"]
        group["points"].append(point)
    for group in groups.values():
        require(group["available"] >= group["required"],
                f"The mounted {'/'.join(group['points'])} filesystem needs {group['required'] / GIB:.2f} GiB free "
                f"for the remaining copy and reserve, but has {group['available'] / GIB:.2f} GiB.")
