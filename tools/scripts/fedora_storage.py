"""Fedora partition policy and Btrfs subvolume mounting.

The C++ helper owns partition-table changes and formatting. These functions
validate its confirmed plan and mount only filesystems on those same disks.
"""

from __future__ import annotations

import json
import os
from pathlib import Path

from fedora_backend import ROOT_FILESYSTEMS, require


BIOS_BOOT_GUID = "21686148-6449-6e6f-744e-656564454649"


def validate_layout(backend, plan: list[dict], devices: list[str], uefi: bool) -> dict[str, str]:
    active = [item for item in plan if item.get("type") == "partition"
              and item.get("operate") not in {"normal", "delete"}]
    boots = [item for item in active if item.get("mountPoint") == "/boot"]
    require(len(boots) == 1 and boots[0].get("filesystem") == "ext4",
            "This release requires a separate ext4 /boot partition.")
    require(boots[0].get("isneedformat") is True,
            "The /boot partition must be new or explicitly selected for formatting.")
    if uefi:
        esps = [item for item in active if item.get("mountPoint") == "/boot/efi"]
        require(len(esps) == 1 and esps[0].get("filesystem") in {"vfat", "fat32", "efi"},
                "UEFI installation requires one FAT32 EFI System Partition at /boot/efi.")
        require(not any(item.get("isBiosBoot") for item in active),
                "Do not create or change a BIOS boot partition in a UEFI installation.")
    else:
        require(len(devices) == 1, "This release supports BIOS installation to one disk at a time.")
    tables = {}
    for device in devices:
        disk_entries = [item for item in plan if item.get("type") == "disk" and item.get("device") == device]
        require(len(disk_entries) <= 1, "The partition plan contains conflicting disk operations.")
        label = disk_entries[0].get("diskType", "") if disk_entries else ""
        if not label:
            label = backend.run("lsblk", "--nodeps", "--noheadings", "--output", "PTTYPE", device).stdout.strip()
        require(label in {"gpt", "dos", "msdos"}, "The installation disk needs a GPT or MBR partition table.")
        tables[device] = "msdos" if label == "dos" else label
        if uefi or label != "gpt":
            continue
        # Untouched BIOS boot areas may be reused on an existing GPT disk. A
        # recreated table needs a newly-created area in its own reviewed plan.
        recreated = bool(disk_entries and disk_entries[0].get("operate") != "normal")
        areas = [item for item in plan if item.get("type") == "partition"
                 and item.get("device") == device and item.get("isBiosBoot") is True
                 and item.get("operate") != "delete"
                 and (not recreated or item.get("operate") == "new")]
        require(bool(areas), "BIOS installation on GPT requires an unformatted BIOS boot partition of at least 1 MiB.")
        for area in areas:
            require(not area.get("mountPoint") and not area.get("filesystem") and not area.get("isneedformat")
                    and not area.get("isCrypt"), "The BIOS boot area must not be formatted, mounted or encrypted.")
            try:
                size = (int(area["endPoint"]) - int(area["startPoint"]) + 1) * int(area["sectorSize"])
            except (KeyError, TypeError, ValueError):
                size = 0
            require(size >= 1024 * 1024, "The BIOS boot partition must be at least 1 MiB.")
    return tables


def verify_bios_area(backend) -> None:
    if backend.state["uefi"]:
        return
    device = backend.state["devices"][0]
    if backend.state["partition_tables"][device] != "gpt":
        return
    result = backend.run("lsblk", "--json", "--bytes", "--paths", "--output",
                         "PATH,TYPE,PARTTYPE,FSTYPE,SIZE,MOUNTPOINTS", device)
    entries = json.loads(result.stdout).get("blockdevices", [])

    def descendants(items):
        for item in items:
            yield item
            yield from descendants(item.get("children", []))

    areas = [item for item in descendants(entries) if item.get("type") == "part"
             and str(item.get("parttype", "")).lower() == BIOS_BOOT_GUID]
    require(any(int(item.get("size") or 0) >= 1024 * 1024 and not item.get("fstype")
                and not any(item.get("mountpoints") or []) for item in areas),
            "The partition helper did not create a usable GPT BIOS boot area. Installation stopped before bootloader writes.")


def mount_filesystems(backend, mappings: list[dict]) -> None:
    root = next(item for item in mappings if item["mountpoint"] == "/")
    for mapping in mappings:
        filesystem = backend.run("blkid", "-s", "TYPE", "-o", "value", mapping["device"]).stdout.strip()
        point = mapping["mountpoint"]
        allowed = ({"swap"} if point == "swap" else {"vfat"} if point == "/boot/efi"
                   else {"ext4"} if point == "/boot" else ROOT_FILESYSTEMS)
        require(filesystem in allowed, f"The partition helper returned an invalid filesystem for {point}.")
        require(mapping["options"] == "defaults" or filesystem == "btrfs",
                "Compression mount options require a Btrfs filesystem.")
        mapping["fstype"] = filesystem
    root_type = root["fstype"]
    if root_type == "btrfs":
        temporary = backend.state_dir / "btrfs-top-level"
        require(not temporary.is_symlink() and not os.path.ismount(temporary),
                "The Btrfs preparation mount is already occupied.")
        temporary.mkdir(mode=0o700, parents=True, exist_ok=True)
        backend.run("mount", "--options", "subvolid=5,compress=zstd:1", root["device"], str(temporary))
        backend.state["temporary_mount"] = str(temporary)
        backend.save()
        make_home = not any(item["mountpoint"] == "/home" for item in mappings)
        try:
            backend.run("btrfs", "subvolume", "create", str(temporary / "root"))
            if make_home:
                backend.run("btrfs", "subvolume", "create", str(temporary / "home"))
        finally:
            backend.run("umount", str(temporary))
            backend.state.pop("temporary_mount", None)
            backend.save()
        root["options"] = "subvol=root,compress=zstd:1"
        if make_home:
            mappings.append({"device": root["device"], "mountpoint": "/home",
                             "options": "subvol=home,compress=zstd:1"})
    mappings.sort(key=lambda item: (item["mountpoint"] != "/", item["mountpoint"].count("/"), item["mountpoint"]))
    backend.state["mounts"] = mappings
    backend.target.mkdir(mode=0o755, exist_ok=True)
    for mapping in mappings:
        if mapping["mountpoint"] == "swap":
            continue  # Never change the running Live environment's swap state.
        point = backend.target if mapping["mountpoint"] == "/" else backend.path(mapping["mountpoint"].lstrip("/"))
        require(not point.is_symlink() and not os.path.ismount(point), "A target mount point is already occupied.")
        point.mkdir(mode=0o755, parents=True, exist_ok=True)
        backend.run("mount", "--options", mapping["options"], mapping["device"], str(point))
        backend.state["owned_mounts"].append(str(point))
        backend.save()
