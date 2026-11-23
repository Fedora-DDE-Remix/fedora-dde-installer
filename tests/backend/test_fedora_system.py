"""System configuration tests; commands are recorded and never executed."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools/scripts"))
import fedora_backend as backend
import fedora_system as system
from profile_fixtures import FEDORA_BRIDGE_PAM, ISO_PROFILE, MAINTAINER_DDM_PAM


PAM = """#%PAM-1.0
auth substack password-auth
auth include postlogin
account required pam_nologin.so
account include password-auth
password include password-auth
session required pam_selinux.so close
session required pam_loginuid.so
session required pam_selinux.so open
session include password-auth
session include postlogin
"""


class Config:
    def __init__(self):
        self.values = {"DI_USERNAME": "alice", "DI_PASSWORD": "test;not-a-command",
                       "DI_HOSTNAME": "workstation.example", "DI_LOCALE": "en_US",
                       "DI_TIMEZONE": "UTC", "DI_LAYOUT": "fr", "DI_IS_ENABLE_NTP": "false"}

    def get(self, key, default=""):
        return self.values.get(key, default) or default

    def set(self, key, value):
        self.values[key] = value


class Recorder:
    def __init__(self, target):
        self.target = target
        self.calls = []
        self.users = {"root": 0, "liveuser": 1000}
        self.groups = ["wheel"]
        self.fail = None

    def run(self, argv, *, input_text=None, check=True, capture=True):
        self.calls.append((list(argv), input_text))
        command = argv[2:] if argv[0] == "chroot" else argv
        if command[0] == self.fail:
            raise backend.InstallError("Recorded failure")
        code, output = 0, ""
        if command[:2] == ["getent", "passwd"]:
            user = command[2]
            code = 0 if user in self.users else 2
            if not code:
                output = f"{user}:x:{self.users[user]}:{self.users[user]}::/home/{user}:/bin/bash\n"
        elif command[0] == "userdel":
            self.users.pop(command[-1])
        elif command[0] == "useradd":
            uid = next(number for number in range(1000, 1100) if number not in self.users.values())
            self.users[command[-1]] = uid
            (self.target / "home" / command[-1]).mkdir(parents=True, exist_ok=True)
        elif command[:2] == ["locale", "-a"]:
            output = "C\nC.utf8\nen_US.utf8\nfr_FR.utf8\n"
        elif command[:3] == ["id", "--name", "--groups"]:
            output = " ".join([command[-1], *self.groups]) + "\n"
        elif command[:2] == ["rpm", "--query"]:
            code = 1
        elif command[0] == "findmnt":
            output = "/dev/testroot 8:1 ext4 aaaa-bbbb rw,relatime /\n"
        return subprocess.CompletedProcess(argv, code, output, "")


class SystemTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.target = self.root / "target"
        self.target.mkdir()
        self.config = Config()
        self.runner = Recorder(self.target)
        self.backend = backend.Backend(self.config, self.runner, self.target, self.root / "state", boot_id="test-boot")
        self.backend.state = {
            "boot_id": "test-boot", "target": str(self.target), "installation_id": "test-id",
            "phase": "awaiting-user", "live_user": "liveuser", "architecture": "x86_64", "uefi": True,
            "mount_identity": {"source": "/dev/testroot", "major_minor": "8:1", "fstype": "ext4", "uuid": "aaaa-bbbb", "fsroot": "/"},
            "mounts": [{"device": "/dev/testroot", "mountpoint": "/", "uuid": "aaaa-bbbb", "options": "defaults"}],
            "owned_mounts": [str(self.target)],
        }
        fixtures = {
            "etc/fedora-dde-installer-target": "test-id\n",
            "usr/share/zoneinfo/UTC": "timezone fixture",
            "usr/share/wayland-sessions/treeland.desktop": "[Desktop Entry]\n",
            "usr/share/systemd/kbd-model-map": "us us pc105 - -\nfr fr pc105 - -\n",
            "usr/lib/kbd/keymaps/legacy/i386/azerty/fr.map.gz": "keymap fixture",
            "usr/lib/kbd/keymaps/legacy/i386/qwerty/us.map.gz": "keymap fixture",
            "usr/lib/kbd/consolefonts/eurlatgr.psfu.gz": "font fixture",
            "usr/lib/systemd/system/chronyd.service": "[Unit]\n",
            "usr/lib/systemd/system/initial-setup.service": "[Unit]\n",
            "etc/selinux/targeted/contexts/files/file_contexts": "policy fixture",
            "etc/selinux/config": "# Packaged settings\nSELINUX=enforcing\nSELINUXTYPE=targeted\nSETLOCALDEFS=0\n",
            "etc/hosts": "# Custom aliases survive\n127.0.0.1 localhost\n192.0.2.10 build.example\n",
            "etc/pam.d/ddm": PAM,
            "etc/authselect/password-auth": "# authselect fixture\n",
            "etc/authselect/postlogin": "# authselect fixture\n",
        }
        for relative, contents in fixtures.items():
            self.put(relative, contents)
        for name in ("password-auth", "postlogin"):
            (self.target / "etc/pam.d" / name).symlink_to("/etc/authselect/" + name)
        self.backend.save()
        self.addCleanup(mock.patch.stopall)
        mock.patch.object(system, "font_candidates", return_value=[]).start()
        self.kernels = mock.patch.object(self.backend, "install_kernels").start()
        self.bootloader = mock.patch.object(self.backend, "install_bootloader").start()

    def put(self, relative, contents):
        path = self.target / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(contents)
        return path

    def commands(self):
        return [argv[2:] if argv[0] == "chroot" else argv for argv, _ in self.runner.calls]

    def test_live_uid_is_freed_before_creating_the_first_user(self):
        retained = self.put("home/liveuser/retained-document", "keep me")
        self.backend.configure()
        self.assertEqual(self.runner.users["alice"], 1000)
        commands = self.commands()
        removal = commands.index(["userdel", "liveuser"])
        creation = next(i for i, command in enumerate(commands) if command[0] == "useradd")
        self.assertLess(removal, creation)
        self.assertEqual(retained.read_text(), "keep me")
        self.assertIn(["chage", "-d", "", "alice"], commands)
        self.assertIn(["chage", "-d", "", "root"], commands)
        self.assertIn(["passwd", "--lock", "root"], commands)

    def test_account_failure_is_retryable_without_recreating_or_deleting_the_user(self):
        self.runner.fail = "chpasswd"
        with self.assertRaises(backend.InstallError):
            self.backend.configure()
        self.assertEqual(self.backend.state["phase"], "configuring-user")
        self.assertTrue(self.config.get("DI_PASSWORD"))
        self.kernels.assert_not_called()
        self.runner.fail = None
        self.backend.configure()
        self.assertEqual(sum(command[0] == "useradd" for command in self.commands()), 1)
        self.assertEqual(sum(command[0] == "userdel" for command in self.commands()), 1)
        self.assertEqual(self.backend.state["phase"], "user-configured")

    def test_reusing_live_username_creates_a_normal_account_once(self):
        self.config.values["DI_USERNAME"] = "liveuser"
        self.runner.fail = "chpasswd"
        with self.assertRaises(backend.InstallError):
            self.backend.configure()
        self.runner.fail = None
        self.backend.configure()
        self.assertEqual(self.runner.users["liveuser"], 1000)
        commands = self.commands()
        self.assertEqual(sum(command[0] == "userdel" for command in commands), 1)
        self.assertEqual(sum(command[0] == "useradd" for command in commands), 1)

    def test_final_kernel_generation_sees_the_selected_keyboard_and_account(self):
        events = []

        def kernels():
            self.assertEqual((self.target / "etc/vconsole.conf").read_text(), 'KEYMAP="fr"\nFONT="eurlatgr"\n')
            self.assertIn(["chage", "-d", "", "alice"], self.commands())
            self.assertIn('Option "XkbLayout" "fr"', (self.target / "etc/X11/xorg.conf.d/00-keyboard.conf").read_text())
            events.append("kernel")

        self.kernels.side_effect = kernels
        self.bootloader.side_effect = lambda: events.append("bootloader")
        with mock.patch.object(self.backend, "remove_installer_from_target", side_effect=lambda: events.append("cleanup")):
            self.backend.configure()
        self.assertEqual(events, ["kernel", "bootloader", "cleanup"])
        self.assertFalse(any(command[0] in {"reboot", "umount", "shutdown"} for command in self.commands()))
        self.assertEqual(self.config.get("DI_PASSWORD"), "")

    def test_hosts_selinux_and_authselect_configuration_survive(self):
        paths = ("etc/hosts", "etc/selinux/config", "etc/pam.d/ddm", "etc/authselect/password-auth")
        before = {path: (self.target / path).read_bytes() for path in paths}
        self.backend.configure_fedora_pam()
        self.backend.configure()
        self.assertEqual(before, {path: (self.target / path).read_bytes() for path in paths})
        self.assertEqual((self.target / "etc/hostname").read_text(), "workstation.example\n")
        self.assertFalse(any(command[0] == "authselect" for command in self.commands()))

    def test_ntp_false_and_initial_setup_are_disabled(self):
        self.put("etc/reconfigSys", "")
        self.backend.configure()
        commands = self.commands()
        for service in ("chronyd.service", "initial-setup.service"):
            self.assertIn(["systemctl", "--root", str(self.target), "disable", service], commands)
        self.assertFalse((self.target / "etc/reconfigSys").exists())

    def test_ntp_true_enables_chronyd(self):
        self.config.values["DI_IS_ENABLE_NTP"] = "true"
        self.backend.configure()
        self.assertIn(["systemctl", "--root", str(self.target), "enable", "chronyd.service"], self.commands())

    def test_retained_home_is_not_recursively_relabelled(self):
        self.put("home/alice/private-data", "retained")
        self.backend.configure()
        relabels = [command for command in self.commands() if command[0] == "restorecon"]
        self.assertTrue(relabels)
        self.assertFalse(any("-F" in command or "/home" in command for command in relabels))
        self.assertFalse(any("-ir" in command and "/home/alice" in command for command in relabels))
        self.assertIn(["restorecon", "-i", "/home/alice", "/home/alice/.config",
                       "/home/alice/.config/locale.conf"], relabels)

    def test_new_home_is_labelled_and_boot_efi_is_excluded(self):
        self.backend.configure()
        commands = self.commands()
        self.assertIn(["restorecon", "-ir", "/home/alice"], commands)
        system_pass = next(command for command in commands if command[:2] == ["restorecon", "-ir"] and "/etc" in command)
        self.assertEqual(system_pass[system_pass.index("/boot/efi") - 1], "-e")
        self.assertNotIn("-F", system_pass)

    def test_missing_keyboard_data_fails_before_account_mutation(self):
        self.config.values["DI_LAYOUT"] = "missing"
        with self.assertRaises(backend.InstallError):
            self.backend.configure()
        self.assertEqual(self.runner.users, {"root": 0, "liveuser": 1000})
        self.assertFalse(any(command[0] in {"useradd", "userdel", "chpasswd"} for command in self.commands()))
        self.assertEqual(self.backend.state["phase"], "awaiting-user")

    def test_converted_variant_and_multiple_layouts_follow_payload_data(self):
        self.put("usr/lib/kbd/keymaps/xkb/fr-oss.map.gz", "converted")
        self.assertEqual(system.console_keymap(self.target, "fr", "oss"), "fr-oss")
        self.assertEqual(system.console_keymap(self.target, "fr,us", "oss,"), "fr-oss")
        self.assertEqual(system.console_keymap(self.target, "us", ""), "us")

    def test_console_font_uses_the_selected_language(self):
        self.put("usr/lib/kbd/consolefonts/LatArCyrHeb-16.psfu.gz", "font")
        with mock.patch.object(system, "font_candidates", return_value=["LatArCyrHeb-16"]):
            self.assertEqual(system.console_font(self.target, "ru_RU.UTF-8"), "LatArCyrHeb-16")

    def test_pam_validation_rejects_upstream_other_distribution_includes(self):
        self.put("etc/pam.d/ddm", "auth include system-login\naccount include common-account\n")
        with self.assertRaises(backend.InstallError):
            self.backend.configure_fedora_pam()
        self.assertIn("system-login", (self.target / "etc/pam.d/ddm").read_text())

    def test_authselect_absolute_symlink_is_resolved_inside_the_target(self):
        system.verify_fedora_pam(self.target)
        self.assertEqual(system.target_path(self.target, "etc/pam.d/password-auth"),
                         self.target / "etc/authselect/password-auth")
        self.assertEqual(self.runner.calls, [])

    def test_opt_in_pam_bridge_preserves_both_package_owned_files(self):
        ddm = self.put("etc/pam.d/ddm", MAINTAINER_DDM_PAM)
        bridge = self.put("etc/pam.d/system-login", FEDORA_BRIDGE_PAM)
        with self.assertRaises(backend.InstallError):
            system.verify_fedora_pam(self.target)
        self.backend.state["install_profile"] = dict(ISO_PROFILE)
        self.backend.configure_fedora_pam()
        self.assertEqual(ddm.read_text(), MAINTAINER_DDM_PAM)
        self.assertEqual(bridge.read_text(), FEDORA_BRIDGE_PAM)
        self.assertEqual(self.runner.calls, [])

    def test_bridge_rejects_missing_includes_and_authentication_bypasses(self):
        self.put("etc/pam.d/ddm", MAINTAINER_DDM_PAM)
        self.put("etc/pam.d/system-login", FEDORA_BRIDGE_PAM)
        for row in FEDORA_BRIDGE_PAM.splitlines():
            if not row or row.startswith("#"):
                continue
            with self.subTest(missing=row), self.assertRaises(backend.InstallError):
                self.put("etc/pam.d/system-login", FEDORA_BRIDGE_PAM.replace(row + "\n", ""))
                system.verify_fedora_pam(self.target, "ddm-system-login")
        for unsafe in (FEDORA_BRIDGE_PAM.replace("auth substack", "auth optional", 1),
                       "auth sufficient pam_permit.so\n" + FEDORA_BRIDGE_PAM):
            with self.subTest(unsafe=unsafe.splitlines()[0]), self.assertRaises(backend.InstallError):
                self.put("etc/pam.d/system-login", unsafe)
                system.verify_fedora_pam(self.target, "ddm-system-login")
        self.put("etc/pam.d/system-login", FEDORA_BRIDGE_PAM)
        self.put("etc/pam.d/ddm", "auth sufficient pam_permit.so\n" + MAINTAINER_DDM_PAM)
        with self.assertRaises(backend.InstallError):
            system.verify_fedora_pam(self.target, "ddm-system-login")
        self.put("etc/pam.d/ddm", MAINTAINER_DDM_PAM)
        (self.target / "etc/authselect/password-auth").unlink()
        with self.assertRaises(backend.InstallError):
            system.verify_fedora_pam(self.target, "ddm-system-login")

    def test_disabled_account_still_requires_password_and_skips_relabel(self):
        self.backend.state["install_profile"] = dict(ISO_PROFILE)
        self.backend.save()
        self.put("etc/selinux/config", "SELINUX=disabled\n")
        self.backend.configure()
        self.assertEqual(self.backend.state["phase"], "user-configured")
        self.assertIn(["id", "--name", "--groups", "alice"], self.commands())
        self.assertIn(["chpasswd"], self.commands())
        self.assertFalse(any(command[0] == "restorecon" for command in self.commands()))

    def test_legacy_profile_refuses_passwordless_group_before_password_or_boot_work(self):
        self.backend.state["install_profile"] = dict(ISO_PROFILE)
        self.backend.save()
        self.runner.groups.append("nopasswdlogin")
        with self.assertRaisesRegex(backend.InstallError, "passwordless-login"):
            self.backend.configure()
        self.assertNotIn(["chpasswd"], self.commands())
        self.kernels.assert_not_called()

    def test_enforcing_relabel_remains_enabled_and_disabled_config_must_match(self):
        self.backend.relabel()
        self.assertTrue(any(command[0] == "restorecon" for command in self.commands()))
        self.backend.state["install_profile"] = dict(ISO_PROFILE)
        self.runner.calls.clear()
        with self.assertRaisesRegex(backend.InstallError, "SELINUX=disabled"):
            self.backend.relabel()
        self.assertEqual(self.runner.calls, [])
        self.put("etc/selinux/config", "SELINUX=disabled\nSELINUX=enforcing\n")
        with self.assertRaises(backend.InstallError):
            self.backend.relabel()

    def test_selinux_source_accepts_enforcing_and_permissive_labelled_payloads(self):
        labels = {"usr/bin/bash": b"system_u:object_r:shell_exec_t:s0\0",
                  "usr/lib/systemd/systemd": b"system_u:object_r:init_exec_t:s0\0",
                  "etc/passwd": b"system_u:object_r:passwd_file_t:s0\0"}
        for path in labels:
            self.put(path, "fixture")
        for mode in ("0\n", "1\n"):
            with self.subTest(mode=mode), mock.patch.object(Path, "read_text", return_value=mode), \
                    mock.patch.object(system.os, "getxattr", side_effect=lambda path, *args, **kwargs:
                                      labels[str(path.relative_to(self.target))]):
                system.verify_selinux_source(self.target)

    def test_selinux_disabled_live_boot_is_rejected_before_inspecting_labels(self):
        with mock.patch.object(Path, "read_text", side_effect=FileNotFoundError), \
                mock.patch.object(system.os, "getxattr") as getxattr, self.assertRaises(backend.InstallError):
            system.verify_selinux_source(self.target)
        getxattr.assert_not_called()

    def test_unlabelled_or_incorrectly_labelled_payload_is_rejected(self):
        self.put("usr/bin/bash", "fixture")
        for label in (b"", b"system_u:object_r:default_t:s0", b"system_u:object_r:init_exec_t:s0"):
            with self.subTest(label=label), mock.patch.object(Path, "read_text", return_value="1\n"), \
                    mock.patch.object(system.os, "getxattr", return_value=label), self.assertRaises(backend.InstallError):
                system.verify_selinux_source(self.target)
        with mock.patch.object(Path, "read_text", return_value="1\n"), \
                mock.patch.object(system.os, "getxattr", side_effect=OSError), self.assertRaises(backend.InstallError):
            system.verify_selinux_source(self.target)

    def test_live_cleanup_preserves_package_defaults_and_unrelated_configuration(self):
        defaults = {"contents": {"autologinUser": {"value": ""}, "lastSession": {"value": "treeland"}}}
        schema = self.put("usr/share/dsg/configs/org.deepin.dde.ddm/org.deepin.dde.ddm.json", json.dumps(defaults))
        live = {"contents": {"autologinUser": {"value": "liveuser"}, "autologinSession": {"value": "treeland"},
                             "theme": {"value": "keep-theme"}}}
        override = self.put("etc/dsg/configs/overrides/org.deepin.dde.ddm/live.json", json.dumps(live))
        sddm = self.put("etc/sddm.conf.d/50-live.conf", "[Autologin]\nUser=liveuser\nSession=treeland\n[Theme]\nCurrent=retained\n")
        sudoers = self.put("etc/sudoers.d/custom", "liveuser ALL=(ALL) NOPASSWD: ALL\n%builders ALL=(ALL) /usr/bin/true\n")
        self.put("etc/machine-info", 'PRETTY_HOSTNAME="Live Media"\n')
        self.backend.clean_live_settings()
        self.assertIn(["systemctl", "--root", str(self.target), "disable", "fedora-dde-live.service"], self.commands())
        self.assertEqual(json.loads(schema.read_text()), defaults)
        contents = json.loads(override.read_text())["contents"]
        self.assertEqual(contents["autologinUser"]["value"], "")
        self.assertEqual(contents["autologinSession"]["value"], "")
        self.assertEqual(contents["theme"]["value"], "keep-theme")
        self.assertIn("Current=retained", sddm.read_text())
        self.assertNotIn("User=liveuser", sddm.read_text())
        self.assertEqual(sudoers.read_text(), "%builders ALL=(ALL) /usr/bin/true\n")
        self.assertFalse((self.target / "etc/machine-info").exists())


if __name__ == "__main__":
    unittest.main()
