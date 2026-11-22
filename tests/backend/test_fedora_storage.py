"""Storage policy tests use temporary directories and never call disk utilities."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools/scripts"))
import fedora_backend as backend
import fedora_storage as storage


class Config:
    def get(self, key, default=""):
        return default


class StorageRunner:
    def __init__(self):
        self.calls = []
        self.filesystems = {"/dev/disk3": "btrfs", "/dev/disk2": "ext4", "/dev/disk1": "vfat",
                            "/dev/disk4": "xfs", "/dev/disk5": "swap"}
        self.fsroot = "/root"
        self.fail_home = False
        self.bios_area = {"path": "/dev/disk1", "type": "part", "parttype": storage.BIOS_BOOT_GUID,
                          "size": 1024 * 1024, "fstype": None, "mountpoints": [None]}

    def run(self, args, **kwargs):
        self.calls.append(args)
        output = ""
        if args[0] == "blkid":
            output = "aaaa-bbbb\n" if "UUID" in args else self.filesystems[args[-1]] + "\n"
        elif args[0] == "lsblk":
            output = json.dumps({"blockdevices": [{"path": "/dev/disk", "type": "disk", "children": [self.bios_area]}]})
        elif args[:3] == ["btrfs", "subvolume", "create"]:
            if args[-1].endswith("/home") and self.fail_home:
                raise backend.InstallError("Simulated subvolume failure")
            Path(args[-1]).mkdir()
        elif args[0] == "findmnt":
            output = f"/dev/disk3[{self.fsroot}] 8:3 btrfs aaaa-bbbb rw,compress=zstd:1 {self.fsroot}\n"
        return subprocess.CompletedProcess(args, 0, output, "")


class StorageTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.runner = StorageRunner()
        self.backend = backend.Backend(Config(), self.runner, self.root / "target", self.root / "state", "test-boot")
        self.backend.state = {"boot_id": "test-boot", "target": str(self.backend.target),
                              "phase": "partitioning", "owned_mounts": [], "devices": ["/dev/disk"],
                              "uefi": False, "partition_tables": {"/dev/disk": "gpt"}}

    def plan(self, sector_size=512):
        return [
            {"type": "disk", "operate": "edit", "device": "/dev/disk", "diskType": "gpt"},
            {"type": "partition", "operate": "new", "device": "/dev/disk", "filesystem": "btrfs", "mountPoint": "/", "isneedformat": True},
            {"type": "partition", "operate": "new", "device": "/dev/disk", "filesystem": "ext4", "mountPoint": "/boot", "isneedformat": True},
            {"type": "partition", "operate": "new", "device": "/dev/disk", "filesystem": "", "mountPoint": "",
             "label": "BIOS-BOOT", "isBiosBoot": True, "isneedformat": False,
             "startPoint": 1024 * 1024 // sector_size, "endPoint": 2 * 1024 * 1024 // sector_size - 1,
             "sectorSize": sector_size},
        ]

    def mappings(self):
        return backend.parse_mounts("/dev/disk3=/[compress=zstd:1];/dev/disk2=/boot;/dev/disk1=/boot/efi")

    def test_bios_gpt_requires_real_sized_unformatted_boot_area(self):
        for sector_size in (512, 4096):
            plan = self.plan(sector_size)
            self.assertEqual(backend.validate_plan(plan), ["/dev/disk"])
            self.assertEqual(storage.validate_layout(self.backend, plan, ["/dev/disk"], False), {"/dev/disk": "gpt"})
            plan[-1]["endPoint"] -= 1
            with self.assertRaises(backend.InstallError):
                storage.validate_layout(self.backend, plan, ["/dev/disk"], False)
        for change in ({"filesystem": "ext4"}, {"mountPoint": "/home"}, {"isneedformat": True}):
            plan = self.plan()
            plan[-1].update(change)
            with self.subTest(change=change), self.assertRaises(backend.InstallError):
                backend.validate_plan(plan)

    def test_existing_bios_area_only_survives_when_table_is_preserved(self):
        plan = self.plan()
        plan[-1]["operate"] = "normal"
        with self.assertRaises(backend.InstallError):
            storage.validate_layout(self.backend, plan, ["/dev/disk"], False)
        plan[0]["operate"] = "normal"
        self.assertEqual(storage.validate_layout(self.backend, plan, ["/dev/disk"], False), {"/dev/disk": "gpt"})

    def test_existing_mbr_does_not_require_a_bios_boot_partition(self):
        plan = self.plan()[:-1]
        plan[0].update(operate="normal", diskType="msdos")
        self.assertEqual(storage.validate_layout(self.backend, plan, ["/dev/disk"], False), {"/dev/disk": "msdos"})

    def test_uefi_preserves_untouched_bios_area_but_cannot_create_one(self):
        plan = self.plan()
        plan.append({"type": "partition", "operate": "edit", "device": "/dev/disk",
                     "filesystem": "vfat", "mountPoint": "/boot/efi"})
        with self.assertRaises(backend.InstallError):
            storage.validate_layout(self.backend, plan, ["/dev/disk"], True)
        plan[-2]["operate"] = "normal"
        self.assertEqual(storage.validate_layout(self.backend, plan, ["/dev/disk"], True), {"/dev/disk": "gpt"})

    def test_helper_result_must_have_actual_bios_guid_without_filesystem(self):
        storage.verify_bios_area(self.backend)
        for change in ({"parttype": "wrong-guid"}, {"size": 1}, {"fstype": "ext4"}, {"mountpoints": ["/unexpected"]}):
            original = dict(self.runner.bios_area)
            self.runner.bios_area.update(change)
            with self.subTest(change=change), self.assertRaises(backend.InstallError):
                storage.verify_bios_area(self.backend)
            self.runner.bios_area = original

    def test_btrfs_root_and_home_share_device_but_use_distinct_subvolumes(self):
        storage.mount_filesystems(self.backend, self.mappings())
        mounts = {item["mountpoint"]: item for item in self.backend.state["mounts"]}
        self.assertEqual(mounts["/"]["device"], mounts["/home"]["device"])
        self.assertEqual(mounts["/"]["options"], "subvol=root,compress=zstd:1")
        self.assertEqual(mounts["/home"]["options"], "subvol=home,compress=zstd:1")
        self.assertNotIn("swap", mounts)
        self.backend.write_fstab()
        fstab = (self.backend.target / "etc/fstab").read_text()
        self.assertIn("/\tbtrfs\tsubvol=root,compress=zstd:1\t0 0", fstab)
        self.assertIn("/home\tbtrfs\tsubvol=home,compress=zstd:1\t0 0", fstab)
        self.assertFalse(any(call[0] in {"swapon", "swapoff", "mkfs.btrfs"} for call in self.runner.calls))

    def test_preserved_separate_home_has_no_new_subvolume_or_formatting(self):
        mappings = self.mappings() + [{"device": "/dev/disk4", "mountpoint": "/home", "options": "defaults"}]
        storage.mount_filesystems(self.backend, mappings)
        subvolumes = [call[-1] for call in self.runner.calls if call[:3] == ["btrfs", "subvolume", "create"]]
        self.assertEqual(subvolumes, [str(self.backend.state_dir / "btrfs-top-level/root")])
        home = next(item for item in self.backend.state["mounts"] if item["mountpoint"] == "/home")
        self.assertEqual(home["device"], "/dev/disk4")
        self.assertEqual(home["options"], "defaults")

    def test_manual_ext4_and_explicit_swap_remain_conventional(self):
        self.runner.filesystems["/dev/disk3"] = "ext4"
        mappings = self.mappings()
        mappings[0]["options"] = "defaults"
        mappings.append({"device": "/dev/disk5", "mountpoint": "swap", "options": "defaults"})
        storage.mount_filesystems(self.backend, mappings)
        self.backend.write_fstab()
        fstab = (self.backend.target / "etc/fstab").read_text()
        self.assertIn("/\text4\tdefaults\t0 1", fstab)
        self.assertIn("none\tswap\tdefaults", fstab)
        self.assertFalse(any(call[0] in {"btrfs", "swapon", "swapoff"} for call in self.runner.calls))

    def test_subvolume_creation_failure_releases_preparation_mount_and_cannot_reformat(self):
        self.runner.fail_home = True
        with self.assertRaises(backend.InstallError):
            storage.mount_filesystems(self.backend, self.mappings())
        self.assertEqual(self.runner.calls[-1], ["umount", str(self.backend.state_dir / "btrfs-top-level")])
        self.assertNotIn("temporary_mount", self.backend.state)
        self.assertEqual(self.backend.state["owned_mounts"], [])
        with self.assertRaises(backend.InstallError):
            self.backend.partition()
        self.assertFalse(any(call[0] == backend.PARTITION_TOOL for call in self.runner.calls))

    def test_another_btrfs_subvolume_with_same_uuid_is_not_the_same_target(self):
        self.backend.state["mount_identity"] = self.backend.mount_identity()
        self.backend.state["mount_identities"] = {}
        self.runner.fsroot = "/home"
        with self.assertRaises(backend.InstallError):
            self.backend.verify_target(marker=False)

    def test_duplicate_partition_and_disk_delete_are_rejected(self):
        with self.assertRaises(backend.InstallError):
            backend.parse_mounts("/dev/disk3=/;/dev/disk3=/home")
        plan = self.plan()
        plan[0]["operate"] = "delete"
        with self.assertRaises(backend.InstallError):
            backend.validate_plan(plan)


if __name__ == "__main__":
    unittest.main()
