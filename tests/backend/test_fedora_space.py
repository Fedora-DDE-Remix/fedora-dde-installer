"""Capacity tests use temporary files and fake mounts; no disk is modified."""

import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools/scripts"))
import fedora_backend as backend
import fedora_payload as payload
import fedora_space as space


def partition(point, size, *, formatted=True, filesystem="ext4"):
    return {"type": "partition", "operate": "new" if formatted else "edit",
            "device": "/dev/testdisk", "path": "/dev/testpart", "mountPoint": point,
            "filesystem": filesystem, "isneedformat": formatted,
            "startPoint": 2048, "endPoint": 2048 + size // 512 - 1, "sectorSize": 512}


def budget(size, point):
    reserve = space.reserve(point, size)
    return {"payload_bytes": size, "remaining_bytes": size,
            "reserve_bytes": reserve, "required_bytes": size + reserve}


class SpaceTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source, self.target = self.root / "source", self.root / "target"
        self.source.mkdir()
        self.target.mkdir()
        self.calls = []
        self.filesystem = "ext4"
        self.fail_unmount = False
        config = SimpleNamespace(get=lambda key, default="": default)
        self.backend = backend.Backend(config, SimpleNamespace(run=self.run_command), self.target,
                                       self.root / "state", "space-test")
        self.backend.state = {"source": str(self.source), "mounts": [
            {"mountpoint": "/", "device": "/dev/testroot"}]}

    def run_command(self, arguments, **kwargs):
        self.calls.append(arguments)
        output = ""
        if arguments[0] == "lsblk":
            output = "/dev/testdisk\n"
        elif arguments[0] == "blkid":
            output = self.filesystem + "\n"
        elif arguments[0] == "findmnt":
            output = "ro,nosuid,nodev,noexec\n"
        elif arguments[0] == "umount" and self.fail_unmount:
            raise backend.InstallError("Simulated unmount failure")
        return subprocess.CompletedProcess(arguments, 0, output, "")

    def put(self, relative, size=8192):
        path = self.source / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"x" * size)
        return path

    def measure(self, points=None, target=None):
        return space.measure(self.source, points or {"/"}, "liveuser", target)

    def free(self, size):
        return SimpleNamespace(f_bavail=size // 4096, f_frsize=4096, f_fsid=17)

    def device_stat(self, path, *args, **kwargs):
        if str(path) == "/dev/testpart":
            return SimpleNamespace(st_mode=stat.S_IFBLK | 0o600)
        return self.original_stat(path, *args, **kwargs)

    def test_16_decimal_gb_layout_has_capacity_for_a_fitting_image(self):
        capacity = 16_000_000_000 - (2048 + 600 + 4) * space.MIB
        plan = [partition("/", capacity), partition("/boot", 2048 * space.MIB),
                partition("/boot/efi", 600 * space.MIB, filesystem="vfat")]
        budgets = {"/": budget(10 * space.GIB, "/"), "/boot": budget(200 * space.MIB, "/boot"),
                   "/boot/efi": budget(30 * space.MIB, "/boot/efi")}
        space.validate_capacity(plan, budgets)
        self.assertLess(capacity, 13 * space.GIB)
        budgets["/"] = budget(13 * space.GIB, "/")
        with self.assertRaisesRegex(backend.InstallError, "this image"):
            space.validate_capacity(plan, budgets)

    def test_invalid_or_missing_geometry_and_small_root_are_rejected(self):
        for change in ({"sectorSize": True}, {"startPoint": -1}, {"endPoint": 1}, {"sectorSize": 0}):
            with self.subTest(change=change), self.assertRaises(backend.InstallError):
                space.capacity(dict(partition("/", 16 * space.GIB), **change))
        with self.assertRaises(backend.InstallError):
            space.capacity({})
        with self.assertRaises(backend.InstallError):
            space.validate_capacity([partition("/", 11 * space.GIB)], {"/": budget(1, "/")})

    def test_hardlinks_count_once_per_destination_filesystem(self):
        original = self.put("usr/original")
        os.link(original, self.source / "usr/another")
        self.put("var/placeholder", 0).unlink()
        os.link(original, self.source / "var/cross-filesystem")
        together = self.measure()["/"]["payload_bytes"]
        separate = self.measure({"/", "/var"})
        self.assertEqual(sum(value["payload_bytes"] for value in separate.values()) - together, 8192)

    def test_every_supported_separate_mount_gets_its_own_budget(self):
        points = {"/", "/boot", "/boot/efi", "/home", "/var", "/srv", "/opt", "/usr/local"}
        for point in points - {"/"}:
            self.put(point.lstrip("/") + "/fixture")
        measured = self.measure(points)
        self.assertEqual(set(measured), points)
        self.assertTrue(all(item["payload_bytes"] > 0 for item in measured.values()))

    def test_hardlinks_are_separate_across_rsync_batches_and_each_fat_path(self):
        original = self.put("usr/original")
        for relative in ("boot/grub2/alias", "etc/sysconfig/alias", "etc/sysconfig/second",
                         "usr/lib/grub/alias", "boot/efi/first", "boot/efi/second"):
            destination = self.source / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            os.link(original, destination)
        directory_bytes = 0
        for path in self.source.rglob("*"):
            if path.is_dir():
                metadata = path.stat()
                directory_bytes += max(4096, ((metadata.st_size + 4095) // 4096) * 4096,
                                       metadata.st_blocks * 512)
        # Main + three -H batches + two independent FAT paths. The second
        # sysconfig member shares data only with its own batch's first member.
        self.assertEqual(self.measure()["/"]["payload_bytes"], directory_bytes + 6 * 8192)

    def test_runtime_live_home_and_symlink_targets_are_not_counted(self):
        self.put("usr/file")
        before = self.measure()["/"]["payload_bytes"]
        self.put("dev/huge", 32768)
        self.put("home/liveuser/private", 32768)
        self.put("etc/fedora-dde-installer/private", 32768)
        link = self.source / "usr/external"
        link.symlink_to("/proc")
        # Only the home/etc parent directories and the symlink itself are added.
        self.assertEqual(self.measure()["/"]["payload_bytes"] - before, 3 * 4096)

    def test_retry_credits_complete_files_but_not_temporary_or_changed_files(self):
        source = self.put("usr/file")
        (self.target / "usr").mkdir()
        complete = self.target / "usr/file"
        shutil.copy2(source, complete)
        self.assertEqual(self.measure(target=self.target)["/"]["remaining_bytes"], 0)
        complete.unlink()
        shutil.copy2(source, self.target / "usr/.file.partial")
        self.assertEqual(self.measure(target=self.target)["/"]["remaining_bytes"], 8192)
        shutil.copy2(source, complete)
        os.utime(complete, ns=(1, 1))
        self.assertEqual(self.measure(target=self.target)["/"]["remaining_bytes"], 8192)

    def test_retry_credits_hardlink_group_only_when_all_members_match(self):
        source = self.put("usr/first")
        os.link(source, self.source / "usr/second")
        (self.target / "usr").mkdir()
        shutil.copy2(source, self.target / "usr/second")
        self.assertEqual(self.measure(target=self.target)["/"]["remaining_bytes"], 8192)
        shutil.copy2(source, self.target / "usr/first")
        self.assertEqual(self.measure(target=self.target)["/"]["remaining_bytes"], 0)

    def test_target_parent_symlink_never_credits_files_outside_target(self):
        self.put("usr/file")
        (self.target / "usr").symlink_to(self.source / "usr")
        self.assertEqual(self.measure(target=self.target)["/"]["remaining_bytes"], 3 * 4096)

    def test_actual_free_space_rejects_consumed_reserve_and_accepts_safe_retry(self):
        source = self.put("usr/file")
        self.backend.state["space_requirements"] = self.measure()
        (self.target / "usr").mkdir()
        shutil.copy2(source, self.target / "usr/file")
        with mock.patch.object(space.os, "statvfs", return_value=self.free(2 * space.GIB)):
            space.before_copy(self.backend)
        with mock.patch.object(space.os, "statvfs", return_value=self.free(space.GIB)), self.assertRaises(backend.InstallError):
            space.before_copy(self.backend)

    def test_shared_btrfs_capacity_is_not_counted_twice(self):
        self.put("home/user/file")
        self.backend.state["space_requirements"] = self.measure({"/", "/home"})
        self.backend.state["mounts"].append({"mountpoint": "/home", "device": "/dev/testroot"})
        with mock.patch.object(space.os, "statvfs", return_value=self.free(9 * space.GIB // 4)), self.assertRaises(backend.InstallError):
            space.before_copy(self.backend)
        with mock.patch.object(space.os, "statvfs", return_value=self.free(3 * space.GIB)):
            space.before_copy(self.backend)

    def test_source_growth_is_rejected_before_copy(self):
        self.backend.state["space_requirements"] = self.measure()
        self.put("new-file")
        with self.assertRaisesRegex(backend.InstallError, "size changed"):
            space.before_copy(self.backend)

    def test_retained_filesystems_disable_journal_replay_and_always_unmount(self):
        self.original_stat = os.stat
        expected = {"ext4": "noload", "xfs": "norecovery", "btrfs": "nologreplay", "vfat": "ro"}
        for filesystem, option in expected.items():
            self.calls.clear()
            self.filesystem = filesystem
            plan = [partition("/", 16 * space.GIB), partition("/home", space.GIB, formatted=False)]
            budgets = {"/": budget(0, "/"), "/home": budget(0, "/home")}
            with mock.patch.object(space.os, "stat", side_effect=self.device_stat), \
                    mock.patch.object(space.os, "statvfs", return_value=self.free(space.GIB)):
                space.retained_space(self.backend, plan, budgets)
            mount = next(call for call in self.calls if call[0] == "mount")
            self.assertIn(option, mount[mount.index("--options") + 1].split(","))
            self.assertIn("ro", mount[mount.index("--options") + 1].split(","))
            self.assertEqual(self.calls[-1][0], "umount")
            self.assertFalse(list(self.backend.state_dir.glob("space-probe-*")))

    def test_retained_home_low_free_space_aborts_preflight_without_writing_disks(self):
        self.original_stat = os.stat
        plan = [partition("/", 16 * space.GIB), partition("/home", 10 * space.GIB, formatted=False)]
        with mock.patch.object(space.os, "stat", side_effect=self.device_stat), \
                mock.patch.object(space.os, "statvfs", return_value=self.free(64 * space.MIB)), \
                self.assertRaisesRegex(backend.InstallError, "No partitions have been changed"):
            space.prepare(self.backend, self.source, plan)
        self.assertEqual(self.calls[-1][0], "umount")
        self.assertFalse(any(call[0] in {backend.PARTITION_TOOL, "parted", "mkfs"} for call in self.calls))

    def test_failed_probe_unmount_reports_path_and_keeps_mount_directory(self):
        self.original_stat = os.stat
        self.fail_unmount = True
        plan = [partition("/", 16 * space.GIB), partition("/home", space.GIB, formatted=False)]
        budgets = {"/": budget(0, "/"), "/home": budget(0, "/home")}
        with mock.patch.object(space.os, "stat", side_effect=self.device_stat), \
                mock.patch.object(space.os, "statvfs", return_value=self.free(space.GIB)), \
                self.assertRaisesRegex(backend.InstallError, "space-probe-.*unmount it before retrying"):
            space.retained_space(self.backend, plan, budgets)
        self.assertEqual(len(list(self.backend.state_dir.glob("space-probe-*"))), 1)
        self.assertEqual(self.calls[-1][0], "umount")

    def test_budget_is_rechecked_before_destructive_helper(self):
        plan = [partition("/", 12 * space.GIB)]
        path = self.root / "plan.json"
        path.write_text(json.dumps(plan))
        self.backend.state.update(phase="preflight", plan_path=str(path),
                                  plan_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                                  space_requirements={"/": budget(13 * space.GIB, "/")})
        with mock.patch.object(self.backend, "load"), mock.patch.object(payload, "verify_source"), \
                self.assertRaises(backend.InstallError):
            self.backend.partition()
        self.assertEqual(self.backend.state["phase"], "preflight")
        self.assertEqual(self.calls, [])


if __name__ == "__main__":
    unittest.main()
