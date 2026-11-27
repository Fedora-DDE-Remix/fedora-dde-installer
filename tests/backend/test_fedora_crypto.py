"""LUKS lifecycle tests. Disk commands are strict recording fakes, never run."""

import fcntl
import hashlib
import json
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock
import uuid

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools/scripts"))
import fedora_backend as backend
import fedora_crypto as crypto
import fedora_space as space
import fedora_storage as storage


LUKS_UUID = "12345678-1234-4abc-8def-123456789abc"
TEST_SECRET = "test-only:密码;$()"


def plan(filesystem="btrfs", extra=None):
    result = [
        {"type": "partition", "operate": "new", "device": "/dev/disk", "path": "",
         "mountPoint": "/", "filesystem": filesystem, "isCrypt": True, "isneedformat": True,
         "startPoint": 2097152, "endPoint": 35651583, "sectorSize": 512},
        {"type": "partition", "operate": "new", "device": "/dev/disk", "mountPoint": "/boot",
         "filesystem": "ext4", "isneedformat": True},
        {"type": "partition", "operate": "edit", "device": "/dev/disk", "mountPoint": "/boot/efi",
         "filesystem": "vfat", "isneedformat": False},
    ]
    if extra:
        result.append({"type": "partition", "operate": "edit", "device": "/dev/disk",
                       "filesystem": "ext4", "isneedformat": True, **extra})
    return result


class Config:
    def __init__(self):
        self.values = {}

    def get(self, key, default=""):
        return self.values.get(key, default)

    def set(self, key, value):
        self.values[key] = value


class CryptoRunner:
    def __init__(self):
        self.calls = []
        self.active = False
        self.identifier = LUKS_UUID
        self.status_device = "/dev/disk3"
        self.inner_type = "btrfs"
        self.fail = None
        self.geometry_bad = False
        self.partition_number = 3
        self.partition_table = "gpt"
        self.offset = crypto.LUKS_HEADER_BYTES
        self.mounts = {}

    def run(self, argv, *, input_text=None, check=True, capture=True):
        self.calls.append((list(argv), input_text))
        command = argv[2:] if argv[0] == "chroot" else argv
        operation = " ".join(command[:2]) if command[0] == "cryptsetup" else command[0]
        if operation == self.fail:
            raise backend.InstallError("Simulated encrypted-install failure")
        output, code = "", 0
        if command[0] == "lsblk":
            output = "/dev/disk\n" if "PKNAME" in command else ""
        elif command[0] == "udevadm":
            output = f"ID_PART_ENTRY_NUMBER={self.partition_number}\n" if "info" in command else ""
        elif command[0] == "parted":
            start = 2097152 * 512 + int(self.geometry_bad)
            end = 35651584 * 512 - 1
            output = (f"BYT;\n/dev/disk:34359738368B:scsi:512:512:{self.partition_table}:disk;\n"
                      f"{self.partition_number}:{start}B:{end}B:{end-start+1}B::root:;\n")
        elif command[:2] == ["cryptsetup", "luksFormat"]:
            self.identifier = command[command.index("--uuid") + 1]
        elif command[:2] == ["cryptsetup", "luksDump"]:
            output = json.dumps({"segments": {"0": {"offset": str(self.offset)}}})
        elif command[:2] == ["cryptsetup", "luksUUID"]:
            output = self.identifier + "\n"
        elif command[:2] == ["cryptsetup", "open"]:
            self.active = True
        elif command[:2] == ["cryptsetup", "status"]:
            code = 0 if self.active else 4
            output = f"{command[-1]} is active.\n  type: LUKS2\n  device: {self.status_device}\n" if self.active else "inactive\n"
        elif command[:2] == ["cryptsetup", "close"]:
            self.active = False
        elif command[0].startswith("mkfs."):
            self.inner_type = command[0].partition(".")[2]
        elif command[0] == "blkid":
            if "UUID" in command:
                output = "aaaa-bbbb\n"
            else:
                output = ({"/dev/disk1": "vfat", "/dev/disk2": "ext4"}.get(command[-1], self.inner_type)) + "\n"
        elif command[0] == "mount":
            self.mounts[command[-1]] = command[-2]
        elif command[0] == "umount":
            self.mounts.pop(command[-1], None)
        elif command[:3] == ["btrfs", "subvolume", "create"]:
            Path(command[-1]).mkdir()
        elif command[0] == "findmnt":
            output = self.mounts.get(command[command.index("--mountpoint") + 1], "/dev/foreign") + "\n"
        elif command[0] in {"sync", "restorecon"}:
            pass
        else:
            raise AssertionError("Unexpected test command: " + repr(command))
        if check and code:
            raise backend.InstallError("Simulated command error")
        return subprocess.CompletedProcess(argv, code, output, "")


class CryptoTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        self.target = self.directory / "target"
        self.target.mkdir()
        self.runner = CryptoRunner()
        self.config = Config()
        self.backend = backend.Backend(self.config, self.runner, self.target, self.directory / "state", "boot-test")
        self.backend.state = {"boot_id": "boot-test", "target": str(self.target), "phase": "partitioning",
                              "installation_id": "test-install", "architecture": "x86_64", "uefi": True,
                              "owned_mounts": [], "owned_mappers": [], "mounts": []}
        self.addCleanup(mock.patch.stopall)
        mock.patch.object(crypto.uuid, "uuid4", return_value=uuid.UUID(LUKS_UUID)).start()
        original = os.stat

        def device_stat(path, *args, **kwargs):
            if str(path).startswith("/dev/"):
                return SimpleNamespace(st_mode=stat.S_IFBLK | 0o600)
            return original(path, *args, **kwargs)

        mock.patch.object(crypto.os, "stat", side_effect=device_stat).start()

    def mappings(self):
        return backend.parse_mounts("/dev/disk3=/[compress=zstd:1];/dev/disk2=/boot;/dev/disk1=/boot/efi")

    def create(self, filesystem="btrfs"):
        mappings = self.mappings()
        if filesystem != "btrfs":
            mappings[0]["options"] = "defaults"
        # os.stat is mocked for block-device validation, but the randomly named
        # mapper is absent before the simulated cryptsetup open.
        with mock.patch.object(Path, "exists", autospec=True, side_effect=lambda path:
                               False if str(path).startswith("/dev/mapper/") else path.is_file() or path.is_dir()):
            crypto.create_root(self.backend, mappings, plan(filesystem), TEST_SECRET)
        return mappings

    def test_all_supported_inner_filesystems_use_luks2_and_stdin_only(self):
        for filesystem in ("btrfs", "ext4", "xfs"):
            with self.subTest(filesystem=filesystem):
                self.backend.state.update(encrypted_root=None, owned_mappers=[], mounts=[])
                self.runner.calls.clear()
                mappings = self.create(filesystem)
                self.assertEqual(mappings[0]["device"], "/dev/mapper/luks-" + LUKS_UUID)
                formatting = next(call for call in self.runner.calls if call[0][:2] == ["cryptsetup", "luksFormat"])
                self.assertIn("luks2", formatting[0])
                self.assertEqual(formatting[0][formatting[0].index("--offset") + 1], "65536")
                self.assertEqual(formatting[1], TEST_SECRET)
                self.assertNotIn(TEST_SECRET, json.dumps([call[0] for call in self.runner.calls]))
                self.assertNotIn(TEST_SECRET, self.backend.state_file.read_text())
                mkfs = next(call[0] for call in self.runner.calls if call[0][0].startswith("mkfs."))
                self.assertEqual(mkfs[-1], mappings[0]["device"])
                self.assertEqual(mkfs[0], "mkfs." + filesystem)

    def test_plaintext_data_or_swap_and_wrong_encryption_flags_are_rejected(self):
        for point in ("/home", "/var", "/tmp", "/srv", "/opt", "/usr/local", "swap"):
            with self.subTest(point=point), self.assertRaises(backend.InstallError):
                backend.validate_plan(plan(extra={"mountPoint": point}))
        for filesystem in ("swap", "linux-swap"):
            for operation in ("new", "edit"):
                with self.subTest(filesystem=filesystem, operation=operation), self.assertRaises(backend.InstallError):
                    backend.validate_plan(plan(extra={"mountPoint": "", "filesystem": filesystem, "operate": operation}))
        for change in ({"isneedformat": False}, {"isCrypt": "true"}, {"cryptPassword": "unsafe"},
                       {"isCryptHeader": True}, {"isNeedCryptHeader": True}, {"filesystem": "crypto_luks"}):
            altered = plan()
            altered[0].update(change)
            with self.subTest(change=change), self.assertRaises(backend.InstallError):
                backend.validate_plan(altered)
        for index in (1, 2):
            altered = plan()
            altered[index]["isCrypt"] = True
            with self.assertRaises(backend.InstallError):
                backend.validate_plan(altered)

    def test_plaintext_manual_layouts_and_untouched_encrypted_partitions_still_work(self):
        unencrypted = plan(extra={"mountPoint": "/home", "isneedformat": False})
        unencrypted[0]["isCrypt"] = False
        unencrypted.append({"type": "partition", "operate": "normal", "isCrypt": True, "device": "/dev/other"})
        self.assertEqual(backend.validate_plan(unencrypted), ["/dev/disk"])

    def test_wrong_physical_geometry_is_rejected_before_luks_format(self):
        self.runner.geometry_bad = True
        with self.assertRaisesRegex(backend.InstallError, "geometry"):
            self.create()
        self.assertFalse(any(args[:2] == ["cryptsetup", "luksFormat"] for args, _ in self.runner.calls))

    def test_mbr_logical_renumbering_uses_current_number_and_exact_geometry(self):
        reviewed = plan()[0]
        reviewed.update(operate="edit", path="/dev/disk6")
        mapping = {"device": "/dev/disk5", "mountpoint": "/", "options": "defaults"}
        self.runner.partition_number = 5
        self.runner.partition_table = "msdos"
        crypto._verify_partition(self.backend, mapping, reviewed)
        self.assertIn((["udevadm", "info", "--query=property", "--name", "/dev/disk5"], None), self.runner.calls)
        self.runner.geometry_bad = True
        with self.assertRaisesRegex(backend.InstallError, "geometry"):
            crypto._verify_partition(self.backend, mapping, reviewed)
        self.assertFalse(any(args[:2] == ["cryptsetup", "luksFormat"] for args, _ in self.runner.calls))

    def test_missing_passphrase_fails_before_partition_helper_or_state_transition(self):
        plan_file = self.directory / "plan.json"
        raw = json.dumps(plan()).encode()
        plan_file.write_bytes(raw)
        self.backend.state.update(plan_path=str(plan_file), plan_sha256=hashlib.sha256(raw).hexdigest(), encryption_requested=True)
        self.backend.save("preflight")
        with mock.patch.dict(os.environ, {}, clear=True), self.assertRaises(backend.InstallError):
            self.backend.partition()
        self.assertEqual(self.backend.state["phase"], "preflight")
        self.assertEqual(self.runner.calls, [])

    def encryption_payload(self, cryptsetup_path="usr/bin/systemd-cryptsetup"):
        files = ("etc/selinux/config", "usr/sbin/cryptsetup", "usr/bin/lsinitrd",
                 "usr/lib/dracut/modules.d/70crypt/module-setup.sh", cryptsetup_path,
                 "usr/lib/dracut/modules.d/71systemd-cryptsetup/module-setup.sh",
                 "usr/lib/dracut/modules.d/11systemd-ask-password/module-setup.sh",
                 "usr/bin/systemd-ask-password", "usr/bin/systemd-tty-ask-password-agent",
                 "usr/lib/systemd/system-generators/systemd-cryptsetup-generator")
        for relative in files:
            path = self.target / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("SELINUX=enforcing\n" if relative == "etc/selinux/config" else "fixture")
            path.chmod(0o644 if relative == "etc/selinux/config" else 0o755)

    def test_encryption_preflight_requires_current_dracut_and_enforcing_payload(self):
        self.encryption_payload()
        # Fedora 45 systemd-udev 262.1 installs the real program in /usr/bin
        # and provides this relative symlink for dracut 111's old entry point.
        (self.target / "usr/lib/systemd/systemd-cryptsetup").symlink_to("../../bin/systemd-cryptsetup")
        with mock.patch.object(crypto.shutil, "which", return_value="/usr/bin/utility"):
            crypto.verify_requirements(self.backend, self.target, plan(), {})
            module = self.target / "usr/lib/dracut/modules.d/71systemd-cryptsetup/module-setup.sh"
            module.unlink()
            with self.assertRaisesRegex(backend.InstallError, "systemd-cryptsetup module"):
                crypto.verify_requirements(self.backend, self.target, plan(), {})
            module.write_text("fixture")
            (self.target / "usr/bin/lsinitrd").unlink()
            with self.assertRaisesRegex(backend.InstallError, "lsinitrd"):
                crypto.verify_requirements(self.backend, self.target, plan(), {})
            from profile_fixtures import ISO_PROFILE
            with self.assertRaisesRegex(backend.InstallError, "Enforcing"):
                crypto.verify_requirements(self.backend, self.target, plan(), {"install_profile": dict(ISO_PROFILE)})

    def test_encryption_preflight_accepts_legacy_libexec_binary(self):
        self.encryption_payload("usr/lib/systemd/systemd-cryptsetup")
        with mock.patch.object(crypto.shutil, "which", return_value="/usr/bin/utility"):
            crypto.verify_requirements(self.backend, self.target, plan(), {})

    def test_encryption_preflight_rejects_missing_or_nonexecutable_unlock_components(self):
        self.encryption_payload()
        # A symlink must not hide a missing or non-executable real program.
        (self.target / "usr/lib/systemd/systemd-cryptsetup").symlink_to("/usr/bin/systemd-cryptsetup")
        files = ("usr/sbin/cryptsetup", "usr/bin/lsinitrd", "usr/bin/systemd-cryptsetup",
                 "usr/lib/systemd/system-generators/systemd-cryptsetup-generator",
                 "usr/bin/systemd-ask-password", "usr/bin/systemd-tty-ask-password-agent")
        with mock.patch.object(crypto.shutil, "which", return_value="/usr/bin/utility"):
            for relative in files:
                path = self.target / relative
                for missing in (False, True):
                    with self.subTest(path=relative, missing=missing):
                        if missing:
                            path.unlink()
                        else:
                            path.chmod(0o644)
                        with self.assertRaisesRegex(backend.InstallError, path.name):
                            crypto.verify_requirements(self.backend, self.target, plan(), {})
                        path.write_text("fixture")
                        path.chmod(0o755)

    def test_helper_cannot_smuggle_plaintext_home_into_encrypted_install(self):
        mappings = self.mappings() + [{"device": "/dev/disk4", "mountpoint": "/home", "options": "defaults"}]
        with self.assertRaisesRegex(backend.InstallError, "reviewed encrypted layout"):
            crypto.create_root(self.backend, mappings, plan(), TEST_SECRET)
        self.assertEqual(self.runner.calls, [])

    def test_unexpected_luks_header_offset_stops_before_open_or_mkfs(self):
        self.runner.offset = 16 * space.MIB
        with self.assertRaisesRegex(backend.InstallError, "offset"):
            self.create()
        self.assertFalse(any(args[0].startswith("mkfs.") or args[:2] == ["cryptsetup", "open"]
                             for args, _ in self.runner.calls))

    def test_mkfs_failure_closes_only_owned_mapper_and_prevents_reformat(self):
        self.runner.fail = "mkfs.btrfs"
        with self.assertRaises(backend.InstallError):
            self.create()
        self.runner.fail = None
        crypto.cleanup_partition_failure(self.backend)
        self.assertFalse(self.runner.active)
        self.assertEqual(self.backend.state["owned_mappers"], [])
        before = sum(args[:2] == ["cryptsetup", "luksFormat"] for args, _ in self.runner.calls)
        with self.assertRaises(backend.InstallError):
            self.create()
        self.assertEqual(sum(args[:2] == ["cryptsetup", "luksFormat"] for args, _ in self.runner.calls), before)

    def test_replaced_mapper_device_or_uuid_is_never_closed(self):
        self.create()
        for device, identifier in (("/dev/other", LUKS_UUID), ("/dev/disk3", "aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee")):
            self.runner.status_device, self.runner.identifier = device, identifier
            with self.subTest(device=device), self.assertRaises(backend.InstallError):
                crypto.close_owned_mappers(self.backend)
        self.assertFalse(any(args[:2] == ["cryptsetup", "close"] for args, _ in self.runner.calls))
        self.assertEqual(len(self.backend.state["owned_mappers"]), 1)

    def test_btrfs_home_shares_encrypted_root_and_crypttab_has_no_key_file(self):
        mappings = self.create()
        storage.mount_filesystems(self.backend, mappings)
        self.backend.write_fstab()
        record = self.backend.state["encrypted_root"]
        self.assertEqual(record["filesystem_uuid"], "aaaa-bbbb")
        home = next(item for item in self.backend.state["mounts"] if item["mountpoint"] == "/home")
        self.assertEqual(home["device"], record["mapper"])
        self.assertEqual(home["options"], "subvol=home,compress=zstd:1")
        self.assertEqual((self.target / "etc/crypttab").read_text(),
                         f"luks-{LUKS_UUID} UUID={LUKS_UUID} none luks,x-initrd.attach\n")
        self.assertNotIn(TEST_SECRET, (self.target / "etc/crypttab").read_text())
        self.assertEqual(stat.S_IMODE((self.target / "etc/crypttab").stat().st_mode), 0o600)

    def test_finalizing_retry_can_close_mapper_after_root_is_already_unmounted(self):
        mappings = self.create("ext4")
        storage.mount_filesystems(self.backend, mappings)
        self.backend.write_fstab()
        self.backend.state.update(user="alice", summary_written=True)
        self.backend.save("user-configured")
        self.runner.fail = "cryptsetup close"
        with mock.patch.object(self.backend, "verify_target") as verify, \
                mock.patch.object(backend.os.path, "ismount", return_value=False):
            with self.assertRaises(backend.InstallError):
                self.backend.finalize()
            self.assertEqual(self.backend.state["phase"], "finalizing")
            self.assertEqual(self.backend.state["owned_mounts"], [])
            self.runner.fail = None
            self.backend.configure()
            self.backend.finalize()
        self.assertEqual(verify.call_count, 1)
        self.assertEqual(self.backend.state["phase"], "finalized")
        self.assertEqual(self.backend.state["owned_mappers"], [])
        self.assertEqual(self.backend.state["encrypted_root"]["filesystem_uuid"], "aaaa-bbbb")

    def test_foreign_mount_is_not_unmounted_by_error_cleanup(self):
        self.create()
        self.backend.state["owned_mounts"] = [str(self.target)]
        self.backend.state["mounts"] = [{"mountpoint": "/", "device": "/dev/mapper/luks-" + LUKS_UUID}]
        with mock.patch.object(backend.os.path, "ismount", return_value=True), self.assertRaises(backend.InstallError):
            crypto.cleanup_partition_failure(self.backend)
        self.assertFalse(any(args[0] == "umount" or args[:2] == ["cryptsetup", "close"]
                             for args, _ in self.runner.calls))

    def test_capacity_accounts_for_fixed_luks_header_on_512_and_4096_byte_disks(self):
        for sector in (512, 4096):
            entry = plan()[0]
            entry.update(startPoint=256, sectorSize=sector)
            budgets = {"/": {"required_bytes": space.ROOT_FLOOR}}
            for excess, accepted in ((0, False), (crypto.LUKS_HEADER_BYTES, True)):
                entry["endPoint"] = entry["startPoint"] + (space.ROOT_FLOOR + excess) // sector - 1
                if accepted:
                    space.validate_capacity([entry], budgets)
                else:
                    with self.assertRaises(backend.InstallError):
                        space.validate_capacity([entry], budgets)


class PassphraseDescriptorTests(unittest.TestCase):
    def descriptor(self, value=TEST_SECRET.encode(), sealed=True, name=crypto.SECRET_MEMFD_NAME):
        descriptor = os.memfd_create(name, os.MFD_CLOEXEC | os.MFD_ALLOW_SEALING)
        self.addCleanup(os.close, descriptor)
        os.write(descriptor, value)
        if sealed:
            fcntl.fcntl(descriptor, crypto.F_ADD_SEALS, crypto.REQUIRED_SEALS)
        return {crypto.SECRET_FD_ENV: str(descriptor)}

    def test_sealed_utf8_descriptor_is_read_repeatedly_without_shared_seek(self):
        environment = self.descriptor()
        for _ in range(2):
            self.assertEqual(crypto.read_passphrase(environment), TEST_SECRET)
        self.assertEqual(os.lseek(int(environment[crypto.SECRET_FD_ENV]), 0, os.SEEK_CUR), len(TEST_SECRET.encode()))

    def test_unsealed_wrongly_named_or_missing_descriptors_are_rejected(self):
        for environment in ({}, {crypto.SECRET_FD_ENV: "0"}, {crypto.SECRET_FD_ENV: "not-a-number"},
                            self.descriptor(sealed=False), self.descriptor(name="other")):
            with self.subTest(environment=environment), self.assertRaises(backend.InstallError):
                crypto.read_passphrase(environment)

    def test_short_multiline_nul_invalid_utf8_and_oversize_secrets_are_rejected(self):
        for value in (b"short", b"test\nsecret", b"test\rsecret", b"test\0secret", b"invalid-\xff", b"a" * 1025):
            with self.subTest(length=len(value)), self.assertRaises(backend.InstallError):
                crypto.read_passphrase(self.descriptor(value))

    def test_regular_file_cannot_be_used_as_a_persisted_passphrase_channel(self):
        with tempfile.TemporaryFile() as handle:
            handle.write(TEST_SECRET.encode())
            handle.flush()
            with self.assertRaises(backend.InstallError):
                crypto.read_passphrase({crypto.SECRET_FD_ENV: str(handle.fileno())})


if __name__ == "__main__":
    unittest.main()
