"""Safe backend tests: all external commands are recorded fakes, never executed."""

import contextlib
import sys
import io
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock


sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools/scripts"))
import fedora_backend as backend
import fedora_payload as payload
import fedora_system as system
from profile_fixtures import ISO_PROFILE


class FakeConfig:
    def __init__(self, values=None):
        self.values = dict(values or {})

    def get(self, key, default=""):
        return self.values.get(key, default) or default

    def set(self, key, value):
        self.values[key] = value


class FakeRunner:
    def __init__(self, target):
        self.target = target
        self.calls = []
        self.users = {"liveuser"}
        self.fail_command = None
        self.firmware_failure = False

    def run(self, argv, *, input_text=None, check=True, capture=True):
        self.calls.append((list(argv), input_text))
        command = argv[2:] if argv[0] == "chroot" else argv
        if command[0] == self.fail_command:
            raise backend.InstallError("Simulated command failure")
        code, output = 0, ""
        if command[:2] == ["getent", "passwd"]:
            code = 0 if command[2] in self.users else 2
            output = f"{command[2]}:x:1000:1000::/home/{command[2]}:/bin/bash\n" if code == 0 else ""
        elif command[:2] == ["locale", "-a"]:
            output = "C\nC.utf8\nen_US.utf8\nen_GB.utf8\nzh_CN.utf8\n"
        elif command[0] == "useradd":
            self.users.add(command[-1])
            (self.target / "home" / command[-1]).mkdir(parents=True, exist_ok=True)
        elif command[0] == "userdel":
            self.users.discard(command[-1])
        elif command[:2] == ["rpm", "--query"]:
            code = 1
        elif command[0] == "rpm" and "-ql" in command:
            package = command[-1]
            family = "shim" if package.startswith("shim-") else "grub2"
            filenames = ("shimx64.efi", "mmx64.efi") if family == "shim" else ("grubx64.efi",)
            output = "".join(f"/usr/lib/efi/{family}/1.0/EFI/fedora/{filename}\n" for filename in filenames)
        elif command[0] == "lsblk" and "PKNAME" in command:
            output = "/dev/testdisk\n"
        elif command[0] == "udevadm":
            output = "ID_PART_ENTRY_NUMBER=1\n"
        elif command[0] == "efibootmgr":
            code = 1 if self.firmware_failure else 0
        elif command[0] == "grub2-mkconfig":
            (self.target / "boot/grub2").mkdir(parents=True, exist_ok=True)
            (self.target / "boot/grub2/grub.cfg").write_text("# fake generated config\n")
        elif command[0] == "dracut":
            output_file = next(value for value in command if value.startswith("/boot/initramfs-"))
            (self.target / output_file.lstrip("/")).write_text("fake initramfs")
        elif command[0] == "blkid":
            output = "1111-2222\n" if "UUID" in command else "ext4\n"
        elif command[0] == "findmnt":
            output = "/dev/testroot 8:1 ext4 aaaa-bbbb rw,relatime /\n"
        if check and code:
            raise backend.InstallError("Simulated unsuccessful command")
        return subprocess.CompletedProcess(argv, code, output, "")


class ValidationTests(unittest.TestCase):
    def test_mount_protocol_rejects_escape_and_runtime_mounts(self):
        for value in ("/dev/sda1=/../../etc", "/dev/sda1=/proc", "/dev/sda1=/persistent",
                      "/dev/sda1=/;/dev/sda2=/", "/dev/sda1=/[bind]", "/dev/../etc=/"):
            with self.subTest(value=value), self.assertRaises(backend.InstallError):
                backend.parse_mounts(value)
        self.assertEqual(backend.parse_mounts("/dev/testroot=/;/dev/testboot=/boot")[0]["mountpoint"], "/")

    def test_untouched_disks_do_not_become_destructive_targets(self):
        plan = [
            {"type": "partition", "operate": "normal", "device": "/dev/liveusb", "isCrypt": True},
            {"type": "partition", "operate": "new", "isneedformat": True, "device": "/dev/disk", "mountPoint": "/", "filesystem": "ext4"},
        ]
        self.assertEqual(backend.validate_plan(plan), ["/dev/disk"])

    def test_root_reuse_and_encryption_fail_before_partitioning(self):
        for extra in ({"operate": "edit", "isneedformat": False}, {"isneedformat": False}, {"isCrypt": True},
                      {"filesystem": "crypto_luks"}, {"mountPoint": "/persistent"},
                      {"label": "safe;touch/tmp/bad"}, {"path": "/dev/a;bad"}):
            item = {"type": "partition", "operate": "new", "isneedformat": True, "device": "/dev/disk",
                    "mountPoint": "/", "filesystem": "ext4"}
            item.update(extra)
            with self.subTest(extra=extra), self.assertRaises(backend.InstallError):
                backend.validate_plan([item])

    def test_password_is_single_line_but_shell_punctuation_is_allowed(self):
        values = {"DI_USERNAME": "alice", "DI_PASSWORD": '@;#\\"\x60$(example):safe', "DI_LOCALE": "zh_CN"}
        self.assertEqual(backend.validate_account(values)["password"], values["DI_PASSWORD"])
        for password in ("", "line\nroot:injected", "nul\0byte"):
            with self.subTest(password=repr(password)), self.assertRaises(backend.InstallError):
                backend.validate_account(dict(values, DI_PASSWORD=password))

    def test_old_password_encoding_is_rejected(self):
        with self.assertRaises(backend.InstallError):
            backend.validate_account({"DI_USERNAME": "alice", "DI_PASSWORD": "encoded",
                                      "DI_PASSWORD_ENCRYPTION_CONFIG": "true", "DI_LOCALE": "en_US"})

    def test_timezone_and_host_validation(self):
        values = {"DI_USERNAME": "alice", "DI_PASSWORD": "secret", "DI_LOCALE": "en_US",
                  "DI_TIMEZONE": "Asia/Beijing", "DI_REGION_FORMAT_LOCALE": "en_GB"}
        account = backend.validate_account(values)
        self.assertEqual(account["timezone"], "Asia/Shanghai")
        self.assertEqual(account["region_locale"], "en_GB.UTF-8")
        for key, value in (("DI_TIMEZONE", "../../etc"), ("DI_HOSTNAME", "host\ninjected"),
                           ("DI_USERNAME", "root"), ("DI_LAYOUT", 'us"; exec bad')):
            with self.subTest(key=key), self.assertRaises(backend.InstallError):
                backend.validate_account(dict(values, **{key: value}))


class WorkflowTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.target = self.root / "target"
        self.target.mkdir()
        self.config = FakeConfig({"DI_USERNAME": "alice", "DI_PASSWORD": '@;#\\"$(not-a-command)',
                                  "DI_LOCALE": "zh_CN", "DI_TIMEZONE": "Asia/Shanghai",
                                  "DI_LAYOUT": "us", "DI_PASSWORD_ENCRYPTION_CONFIG": "false",
                                  "DI_REGION_FORMAT_LOCALE": "en_GB"})
        self.runner = FakeRunner(self.target)
        self.backend = backend.Backend(self.config, self.runner, self.target, self.root / "state", boot_id="test-boot")
        self.identity = {"source": "/dev/testroot", "major_minor": "8:1", "fstype": "ext4", "uuid": "aaaa-bbbb", "fsroot": "/"}
        self.backend.state = {"boot_id": "test-boot", "target": str(self.target), "installation_id": "test-install",
                              "phase": "awaiting-user", "mount_identity": self.identity, "live_user": "liveuser",
                              "owned_mounts": [str(self.target), str(self.target / "boot"), str(self.target / "dev")],
                              "architecture": "x86_64", "uefi": True, "devices": ["/dev/testdisk"],
                              "mounts": [{"device": "/dev/testroot", "mountpoint": "/", "uuid": "aaaa-bbbb", "options": "defaults"},
                                         {"device": "/dev/testboot", "mountpoint": "/boot", "uuid": "cccc-dddd", "options": "defaults"},
                                         {"device": "/dev/testesp", "mountpoint": "/boot/efi", "uuid": "eeee-ffff", "options": "defaults"}]}
        for relative in ("etc/fedora-dde-installer-target", "usr/share/zoneinfo/Asia/Shanghai",
                         "usr/share/wayland-sessions/treeland.desktop",
                         "etc/selinux/targeted/contexts/files/file_contexts"):
            path = self.target / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("test-install\n" if relative.endswith("-target") else "fixture\n")
        for relative, contents in {
                "usr/share/systemd/kbd-model-map": "us us pc105 - -\n",
                "usr/lib/kbd/keymaps/legacy/i386/qwerty/us.map.gz": "keymap fixture",
                "usr/lib/kbd/consolefonts/eurlatgr.psfu.gz": "font fixture",
                "etc/pam.d/ddm": "auth substack password-auth\nauth include postlogin\n"
                    "account include password-auth\npassword include password-auth\n"
                    "session required pam_selinux.so close\nsession required pam_loginuid.so\n"
                    "session required pam_selinux.so open\nsession include password-auth\n"
                    "session include postlogin\n",
                "etc/pam.d/password-auth": "# authselect fixture\n",
                "etc/pam.d/postlogin": "# authselect fixture\n",
                "etc/pam.d/ddm-autologin": "# Fedora package-owned automatic-login stack\n",
                "etc/selinux/config": "SELINUX=enforcing\nSELINUXTYPE=targeted\n",
        }.items():
            path = self.target / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(contents)
        for name in ("install_kernels", "install_bootloader"):
            patcher = mock.patch.object(self.backend, name)
            setattr(self, name, patcher.start())
            self.addCleanup(patcher.stop)
        fonts = mock.patch.object(system, "font_candidates", return_value=[])
        fonts.start()
        self.addCleanup(fonts.stop)
        self.backend.save()

    def test_configures_target_without_reboot_and_clears_credentials(self):
        password = self.config.get("DI_PASSWORD")
        self.backend.configure()
        account_call = next(call for call in self.runner.calls if call[0][-1] == "chpasswd")
        self.assertEqual(account_call[0][:2], ["chroot", str(self.target)])
        self.assertEqual(account_call[1], "alice:" + password + "\n")
        self.assertTrue(all(password not in " ".join(call[0]) for call in self.runner.calls))
        self.assertEqual(self.config.get("DI_PASSWORD"), "")
        self.assertEqual(self.backend.state["phase"], "user-configured")
        locale = (self.target / "etc/locale.conf").read_text()
        self.assertIn("LANG=zh_CN.UTF-8\n", locale)
        self.assertIn("LC_TIME=en_GB.UTF-8\n", locale)
        self.assertIn("%wheel ALL=(ALL) ALL", (self.target / "etc/sudoers.d/90-fedora-dde-admin").read_text())
        self.assertFalse(any("reboot" in call[0] or "umount" in call[0] for call in self.runner.calls))
        self.assertNotIn("liveuser", self.runner.users)

    def test_configure_failure_keeps_mounts_and_credentials_for_retry(self):
        self.runner.fail_command = "chpasswd"
        with self.assertRaises(backend.InstallError):
            self.backend.configure()
        self.assertEqual(self.backend.state["phase"], "configuring-user")
        self.assertEqual(self.backend.state["user"], "alice")
        self.assertTrue(self.config.get("DI_PASSWORD"))
        self.assertFalse(any(call[0][0] == "umount" for call in self.runner.calls))
        self.runner.fail_command = None
        self.backend.configure()
        self.assertEqual(self.backend.state["phase"], "user-configured")
        self.assertEqual(sum(call[0][2] == "useradd" for call in self.runner.calls if call[0][0] == "chroot"), 1)

    def test_changed_mount_aborts_before_any_account_command(self):
        self.backend.state["mount_identity"]["uuid"] = "another-target"
        self.backend.save()
        with self.assertRaises(backend.InstallError):
            self.backend.configure()
        self.assertFalse(any(call[0][0] == "chroot" for call in self.runner.calls))

    def test_changed_boot_mount_aborts_even_when_root_is_unchanged(self):
        self.backend.state["mount_identities"] = {
            str(self.target): dict(self.identity),
            str(self.target / "boot"): dict(self.identity, uuid="different-boot"),
        }
        self.backend.save()
        with self.assertRaises(backend.InstallError):
            self.backend.configure()
        self.assertFalse(any(call[0][0] == "chroot" for call in self.runner.calls))

    def test_stale_boot_cannot_configure_host(self):
        self.backend.state["boot_id"] = "a-different-boot"
        self.backend.save()
        with self.assertRaises(backend.InstallError):
            self.backend.configure()
        self.assertEqual(self.runner.calls, [])

    def test_parent_symlink_cannot_escape_target(self):
        external = self.root / "outside"
        external.mkdir()
        (self.target / "srv").symlink_to(external)
        with self.assertRaises(backend.InstallError):
            self.backend.write("srv/unauthorized", "data")
        self.assertFalse((external / "unauthorized").exists())

    def test_ready_retains_mounts_for_user_form(self):
        self.backend.save("system-configured")
        with contextlib.redirect_stdout(io.StringIO()):
            self.backend.ready()
        self.assertEqual(self.backend.state["phase"], "awaiting-user")
        self.assertEqual(len(self.backend.state["owned_mounts"]), 3)
        self.assertFalse(any(call[0][0] == "umount" for call in self.runner.calls))

    def test_finalization_requires_successful_account_phase(self):
        with self.assertRaises(backend.InstallError):
            self.backend.finalize()
        self.assertEqual(self.runner.calls, [])

    def test_disabled_final_summary_records_profile_without_restorecon(self):
        self.backend.state.update(install_profile=dict(ISO_PROFILE), user="alice")
        self.backend.save("user-configured")
        (self.target / "etc/selinux/config").write_text("SELINUX=disabled\n")
        with mock.patch.object(backend.os.path, "ismount", return_value=False), contextlib.redirect_stdout(io.StringIO()):
            self.backend.finalize()
        summary = json.loads((self.target / "var/log/fedora-dde-installer/install-summary.json").read_text())
        self.assertEqual(summary["install_profile"], ISO_PROFILE)
        self.assertEqual(self.backend.state["phase"], "finalized")
        self.assertFalse(any("restorecon" in command for command, _ in self.runner.calls))

    def test_finalize_retry_does_not_require_password(self):
        self.backend.configure()
        self.runner.fail_command = "umount"
        with mock.patch.object(backend.os.path, "ismount", return_value=True), self.assertRaises(backend.InstallError):
            self.backend.finalize()
        self.assertEqual(self.backend.state["phase"], "user-configured")
        self.assertEqual(self.config.get("DI_PASSWORD"), "")
        self.runner.fail_command = None
        self.backend.configure()
        with mock.patch.object(backend.os.path, "ismount", return_value=True), contextlib.redirect_stdout(io.StringIO()):
            self.backend.finalize()
        unmounts = [call[0][-1] for call in self.runner.calls if call[0][0] == "umount"]
        self.assertEqual(unmounts[-3:], [str(self.target / "dev"), str(self.target / "boot"), str(self.target)])
        self.assertEqual(self.backend.state["phase"], "finalized")
        self.assertEqual(self.config.get("DI_INSTALLER_STATUS"), "100")
        self.assertEqual(self.backend.state["owned_mounts"], [])
        self.assertFalse(any("reboot" in call[0] for call in self.runner.calls))

    def test_finalize_retry_does_not_write_through_an_unmounted_var(self):
        self.backend.state["owned_mounts"] = [str(self.target), str(self.target / "var")]
        self.backend.save()
        self.backend.configure()
        original_run = self.runner.run

        def fail_root_unmount(argv, **kwargs):
            if argv[0] == "umount" and argv[-1] == str(self.target):
                raise backend.InstallError("Root is still busy")
            return original_run(argv, **kwargs)

        with mock.patch.object(self.backend, "write", wraps=self.backend.write) as write:
            with mock.patch.object(backend.os.path, "ismount", return_value=True):
                with mock.patch.object(self.runner, "run", side_effect=fail_root_unmount):
                    with self.assertRaises(backend.InstallError):
                        self.backend.finalize()
                self.assertEqual(self.backend.state["owned_mounts"], [str(self.target)])
                self.backend.configure()
                with contextlib.redirect_stdout(io.StringIO()):
                    self.backend.finalize()
            summaries = [call for call in write.call_args_list
                         if call.args[0] == "var/log/fedora-dde-installer/install-summary.json"]
            self.assertEqual(len(summaries), 1)
        self.assertEqual(self.backend.state["phase"], "finalized")

    def test_fstab_uses_only_explicitly_selected_swap(self):
        self.backend.state["mounts"] = [self.backend.state["mounts"][0]]
        self.backend.write_fstab()
        fstab = (self.target / "etc/fstab").read_text()
        self.assertIn("UUID=1111-2222", fstab)
        self.assertNotIn("swap", fstab)
        self.assertFalse(any(call[0][0] in {"swapon", "swapoff"} for call in self.runner.calls))

    def test_copy_excludes_live_mounts_machine_identity_and_credentials(self):
        source = self.root / "source"
        source.mkdir()
        self.backend.state["source"] = str(source)
        self.backend.save("partitioned")
        with mock.patch.object(payload, "verify_source"):
            self.backend.copy()
        rsync = next(call[0] for call in self.runner.calls if call[0][0] == "rsync")
        for item in ("/dev/***", "/proc/***", "/sys/***", "/run/***", "/tmp/***", "/target/***",
                     "/etc/machine-id", "/etc/ssh/ssh_host_*", "/etc/fedora-dde-installer/***"):
            self.assertIn("--exclude=" + item, rsync)
        self.assertIn("--one-file-system", rsync)
        self.assertNotIn("--delete", rsync)
        self.assertEqual(self.backend.state["phase"], "copied")

    def test_fedora_pam_uses_authselect_and_preserves_selinux_sessions(self):
        original = (self.target / "etc/pam.d/ddm").read_bytes()
        original_autologin = (self.target / "etc/pam.d/ddm-autologin").read_bytes()
        self.backend.configure_fedora_pam()
        self.assertEqual(original, (self.target / "etc/pam.d/ddm").read_bytes())
        self.assertEqual(original_autologin, (self.target / "etc/pam.d/ddm-autologin").read_bytes())
        text = (self.target / "etc/pam.d/ddm").read_text()
        self.assertIn("password-auth", text)
        self.assertIn("postlogin", text)
        self.assertLess(text.index("pam_selinux.so close"), text.index("pam_selinux.so open"))
        self.assertNotIn("system-login", text)
        self.assertNotIn("common-session", text)

    def test_live_passwordless_wheel_rules_are_removed(self):
        sudoers = self.target / "etc/sudoers"
        sudoers.write_text("Defaults env_reset\n%wheel ALL=(ALL) NOPASSWD: ALL\n")
        dropin = self.target / "etc/sudoers.d/99-live-wheel"
        dropin.parent.mkdir()
        dropin.write_text("# Live administrative access\n%wheel ALL=(ALL:ALL) NOPASSWD: ALL\n")
        self.backend.clean_live_settings()
        self.assertIn("Defaults env_reset", sudoers.read_text())
        self.assertNotIn("NOPASSWD", sudoers.read_text())
        self.assertNotIn("NOPASSWD", dropin.read_text())


class RunnerTests(unittest.TestCase):
    def test_password_input_is_not_relayed_on_command_failure(self):
        result = subprocess.CompletedProcess(["chpasswd"], 1, "", "potential sensitive diagnostic")
        with mock.patch.object(backend.subprocess, "run", return_value=result) as run:
            with self.assertRaises(backend.InstallError) as error:
                backend.Runner().run(["chpasswd"], input_text="alice:secret\n")
        self.assertNotIn("potential sensitive", str(error.exception))
        self.assertNotIn("secret", str(error.exception))
        self.assertEqual(run.call_args.kwargs["input"], "alice:secret\n")
        self.assertFalse(run.call_args.kwargs.get("shell", False))


if __name__ == "__main__":
    unittest.main()
