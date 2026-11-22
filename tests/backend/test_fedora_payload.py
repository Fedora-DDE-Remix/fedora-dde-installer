"""Live source and copying tests; every mount and rsync command is a fake."""

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools/scripts"))
import fedora_backend as backend
import fedora_payload as payload
import fedora_system as system


class Config:
    def __init__(self):
        self.values = {}

    def get(self, key, default=""):
        return self.values.get(key, default)

    def set(self, key, value):
        self.values[key] = value


class PayloadRunner:
    def __init__(self):
        self.calls = []
        self.filesystem = {"source": "/dev/image", "maj:min": "7:0", "fstype": "erofs",
                           "uuid": "aaaa-bbbb", "options": "ro,nosuid,nodev", "fsroot": "/"}
        self.fail_unmount = False

    def run(self, args, **kwargs):
        self.calls.append(args)
        if args[0] == "umount" and self.fail_unmount:
            raise backend.InstallError("Simulated busy image")
        output = json.dumps({"filesystems": [self.filesystem]}) if args[0] == "findmnt" else ""
        return subprocess.CompletedProcess(args, 0, output, "")


class PayloadTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / "source"
        self.source.mkdir()
        # Kernel SELinux state and xattr policy have dedicated system tests.
        # The temporary fixture is an ordinary unprivileged test directory.
        selinux = patch.object(system, "verify_selinux_source")
        selinux.start()
        self.addCleanup(selinux.stop)
        self.config, self.runner = Config(), PayloadRunner()
        self.backend = backend.Backend(self.config, self.runner, self.root / "target", self.root / "state", "test-boot")
        self.backend.target.mkdir()
        self.backend.state = {"boot_id": "test-boot", "target": str(self.backend.target),
                              "source": str(self.source), "source_mount_owned": False,
                              "source_identity": {key: value for key, value in self.runner.filesystem.items() if key != "options"},
                              "installation_id": "payload-test", "owned_mounts": []}
        self.write("etc/os-release", 'ID=fedora\nVERSION_ID="45"\n')
        self.write("usr/lib/modules/6.12-test/vmlinuz", "kernel")
        for name in ("rpm", "dnf", "systemctl", "dracut", "chpasswd", "useradd", "userdel", "usermod", "chage",
                     "grub2-mkconfig", "grub2-editenv", "grubby", "kernel-install", "systemd-machine-id-setup",
                     "depmod", "restorecon"):
            self.write("usr/bin/" + name, "executable fixture")
        self.write("usr/lib/systemd/zram-generator.conf", "[zram0]\n")
        self.write("etc/selinux/config", "SELINUX=enforcing\nSELINUXTYPE=targeted\n")
        self.write("etc/pam.d/ddm", "auth substack password-auth\nauth include postlogin\n"
                   "account include password-auth\npassword include password-auth\n"
                   "session required pam_selinux.so close\nsession required pam_loginuid.so\n"
                   "session required pam_selinux.so open\nsession include password-auth\nsession include postlogin\n")
        for name in ("password-auth", "postlogin"):
            self.write("etc/authselect/" + name, "# payload authselect\n")
            (self.source / "etc/pam.d" / name).symlink_to("/etc/authselect/" + name)
        for name in ("20-grub.install", "50-dracut.install", "51-dracut-rescue.install"):
            self.write("usr/lib/kernel/install.d/" + name, "#!/bin/sh\nexit 0\n").chmod(0o755)
        self.write("usr/lib/dracut/dracut.conf.d/02-rescue.conf", 'dracut_rescue_image="yes"\n')

    def write(self, relative, contents):
        path = self.source / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(contents)
        return path

    def test_readonly_base_accepts_authselect_absolute_links_inside_the_image(self):
        self.assertEqual(payload.validate_root(self.backend, self.source), self.backend.state["source_identity"])
        self.assertEqual(payload.rooted_path(self.source, "etc/pam.d/password-auth"),
                         self.source / "etc/authselect/password-auth")

    def test_running_root_writable_mount_and_overlay_cannot_be_sources(self):
        with self.assertRaises(backend.InstallError):
            payload.validate_root(self.backend, Path("/"))
        for filesystem, options in (("erofs", "rw"), ("overlay", "ro")):
            with self.subTest(filesystem=filesystem, options=options):
                self.runner.filesystem.update(fstype=filesystem, options=options)
                with self.assertRaises(backend.InstallError):
                    payload.validate_root(self.backend, self.source)

    def test_wrong_fedora_release_or_ostree_is_rejected(self):
        self.write("etc/os-release", "ID=fedora\nVERSION_ID=44\n")
        with self.assertRaises(backend.InstallError):
            payload.validate_root(self.backend, self.source)
        self.write("etc/os-release", "ID=fedora\nVERSION_ID=45\n")
        (self.source / "ostree/repo").mkdir(parents=True)
        with self.assertRaises(backend.InstallError):
            payload.validate_root(self.backend, self.source)

    def test_broken_authselect_and_disabled_kernel_hooks_fail_before_copy(self):
        (self.source / "etc/authselect/password-auth").unlink()
        with self.assertRaises(backend.InstallError):
            payload.copy_payload(self.backend)
        self.assertFalse(any(call[0] == "rsync" for call in self.runner.calls))
        self.write("etc/authselect/password-auth", "restored")
        self.write("etc/kernel/install.d/20-grub.install", "")
        with self.assertRaises(backend.InstallError):
            payload.copy_payload(self.backend)

    def test_source_mount_replacement_aborts_before_partition_helper(self):
        plan = self.root / "plan.json"
        plan.write_text("[]")
        self.backend.state.update(plan_path=str(plan), plan_sha256=hashlib.sha256(plan.read_bytes()).hexdigest())
        self.backend.save("preflight")
        self.runner.filesystem["maj:min"] = "7:1"
        with self.assertRaises(backend.InstallError):
            self.backend.partition()
        self.assertEqual(self.backend.state["phase"], "preflight")
        self.assertFalse(any(call[0] == backend.PARTITION_TOOL for call in self.runner.calls))

    def test_payload_preserves_rpm_directories_and_separates_fat_copy(self):
        for relative in ("boot/efi/EFI/fedora", "boot/grub2", "etc/sysconfig", "usr/lib/grub"):
            (self.source / relative).mkdir(parents=True)
        self.write("root/package-owned", "preserve")
        payload.copy_payload(self.backend)
        calls = [call for call in self.runner.calls if call[0] == "rsync"]
        self.assertEqual(len(calls), 5)
        main = calls[0]
        for flag in ("-aHAX", "--numeric-ids", "--one-file-system"):
            self.assertIn(flag, main)
        for path in ("/etc/machine-id", "/etc/machine-info", "/home/liveuser/***", "/boot/loader/***", "/boot/efi/***"):
            self.assertIn("--exclude=" + path, main)
        for path in ("/root/***", "/var/log/***", "/var/cache/***"):
            self.assertNotIn("--exclude=" + path, main)
        esp = next(call for call in calls if call[-2] == str(self.source / "boot/efi") + "/")
        self.assertIn("-rx", esp)
        self.assertIn("--exclude=/EFI/BOOT/***", esp)
        self.assertNotIn("-aHAX", esp)
        self.assertTrue(all("--delete" not in call for call in calls))

    def test_user_provided_source_is_not_unmounted(self):
        self.config.values["DI_SOURCE_ROOT"] = str(self.source)
        self.backend.state.update(payload.prepare_source(self.backend))
        payload.release_source(self.backend)
        self.assertFalse(self.backend.state["source_mount_owned"])
        self.assertFalse(any(call[0] == "umount" for call in self.runner.calls))

    def test_failed_validation_does_not_unmount_a_preexisting_foreign_source(self):
        identity = dict(self.backend.state["source_identity"], source="/dev/foreign")
        with patch.object(payload, "detect_live_device", return_value="/dev/image"), \
                patch.object(payload.os.path, "ismount", return_value=True), \
                patch.object(payload, "validate_root", return_value=identity), self.assertRaises(backend.InstallError):
            payload.prepare_source(self.backend)
        self.assertFalse(any(call[0] in {"mount", "umount"} for call in self.runner.calls))

    def test_new_source_mount_is_readonly_and_released_after_bad_validation(self):
        with patch.object(payload, "detect_live_device", return_value="/dev/image"), \
                patch.object(payload.os.path, "ismount", return_value=False), \
                patch.object(payload, "validate_root", side_effect=backend.InstallError("Bad source")), \
                self.assertRaises(backend.InstallError):
            payload.prepare_source(self.backend)
        mount = next(call for call in self.runner.calls if call[0] == "mount")
        self.assertIn("--read-only", mount)
        self.assertTrue(any(call[0] == "umount" for call in self.runner.calls))

    def test_source_unmount_failure_retries_without_copying_or_formatting_again(self):
        self.backend.state["source_mount_owned"] = True
        self.backend.save("partitioned")
        self.runner.fail_unmount = True
        with patch.object(self.backend, "verify_target"):
            with self.assertRaises(backend.InstallError):
                self.backend.copy()
            self.assertEqual(self.backend.state["phase"], "copied")
            copies = sum(call[0] == "rsync" for call in self.runner.calls)
            self.runner.fail_unmount = False
            self.backend.copy()
        self.assertEqual(sum(call[0] == "rsync" for call in self.runner.calls), copies)
        self.assertTrue(self.backend.state["source_released"])


if __name__ == "__main__":
    unittest.main()
