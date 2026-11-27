"""Boot deployment regression tests. All installer commands are simulated."""

from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools/scripts"))
import fedora_backend as backend
import fedora_boot as boot
import fedora_crypto as crypto
from profile_fixtures import ISO_PROFILE


TEST_MACHINE_ID = "1234567890abcdef1234567890abcdef"
TEST_KERNEL = "6.12.9-200.fc45.x86_64"


class BootConfig:
    def __init__(self, values=None):
        self.values = values or {}

    def get(self, key, default=""):
        return self.values.get(key, default)


class BootRunner:
    """Model the artifacts written by Fedora's external hooks, without running them."""

    def __init__(self, target):
        self.target = target
        self.calls = []
        self.firmware_failure = False
        self.omit_bls = False
        self.omit_rescue = False
        self.omit_stub = False
        self.has_plymouth = True
        self.extra_options = ""
        self.omit_crypt_module = False
        self.omit_systemd_cryptsetup = False
        self.omit_unlock_program = False
        self.initrd_files = {
            "usr/lib/systemd/system-generators/systemd-cryptsetup-generator": "-rwxr-xr-x",
            "usr/bin/systemd-ask-password": "-rwxr-xr-x",
            "usr/bin/systemd-tty-ask-password-agent": "-rwxr-xr-x",
            "usr/lib/systemd/system/systemd-ask-password-console.service": "-rw-r--r--",
            "usr/bin/systemd-cryptsetup": "-rwxr-xr-x",
        }
        self.initrd_symlinks = {"usr/lib/systemd/systemd-cryptsetup": "../../bin/systemd-cryptsetup"}
        self.initrd_crypttab_override = None
        self.swap_sizes = {"/dev/swap-small": 1024, "/dev/swap-large": 4096}

    def write(self, relative, text):
        path = self.target / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)

    def run(self, argv, check=True, capture=True, input_text=None, **kwargs):
        argv = [str(arg) for arg in argv]
        self.calls.append(argv)
        command = argv[2:] if argv[0] == "chroot" else argv
        output, code = "", 0
        name = command[0]
        if name == "sort":
            output = "\n".join(sorted(input_text.splitlines(), key=lambda value: tuple(
                int(piece) if piece.isdigit() else piece for piece in re.split(r"(\d+)", value)))) + "\n"
        elif name == "systemd-machine-id-setup":
            self.write("etc/machine-id", TEST_MACHINE_ID + "\n")
        elif name == "rpm":
            if "-ql" in command:
                package = command[-1]
                suffix = package.rsplit("-", 1)[1]
                names = (f"shim{suffix}.efi", f"mm{suffix}.efi") if package.startswith("shim-") else (f"grub{suffix}.efi",)
                output = "".join(f"/usr/lib/efi/{package}/{entry}\n" for entry in names)
            elif "-qf" in command:
                output = "kernel-uki-dtbloader\n" if command[-1].endswith("vmlinuz-dtbloader.efi") else "kernel-core\n"
            elif "--whatprovides" in command:
                output = "plymouth-24.004.60-1.fc45.x86_64\n" if self.has_plymouth else "no package provides plymouth\nno package provides rhgb\n"
                code = 0 if self.has_plymouth else 1
            else:
                raise AssertionError(f"Unexpected RPM query: {command}")
        elif name == "grub2-editenv":
            if command[-1] == "create":
                self.write("boot/grub2/grubenv", "# GRUB Environment Block\n")
            else:
                path = self.target / "boot/grub2/grubenv"
                self.write("boot/grub2/grubenv", path.read_text() + "\n".join(command[3:]) + "\n")
        elif name == "kernel-install":
            version = command[2]
            source = self.target / command[3].lstrip("/")
            destination = self.target / f"boot/vmlinuz-{version}"
            destination.parent.mkdir(parents=True, exist_ok=True)
            if source != destination:
                shutil.copyfile(source, destination)
            self.write(f"boot/initramfs-{version}.img", "initramfs made by Fedora kernel-install\n")
            options = (self.target / "etc/kernel/cmdline").read_text().strip() + self.extra_options
            if not self.omit_bls:
                self.write(f"boot/loader/entries/{TEST_MACHINE_ID}-{version}.conf",
                           f"title Fedora\nversion {version}\nlinux /vmlinuz-{version}\n"
                           f"initrd /initramfs-{version}.img\noptions {options}\n")
            if not self.omit_rescue:
                self.write(f"boot/vmlinuz-0-rescue-{TEST_MACHINE_ID}", "rescue kernel\n")
                self.write(f"boot/initramfs-0-rescue-{TEST_MACHINE_ID}.img", "rescue initramfs from 51-dracut-rescue.install\n")
                self.write(f"boot/loader/entries/{TEST_MACHINE_ID}-0-rescue.conf",
                           f"title Fedora rescue\nversion {version}\nlinux /vmlinuz-0-rescue-{TEST_MACHINE_ID}\n"
                           f"initrd /initramfs-0-rescue-{TEST_MACHINE_ID}.img\noptions {options}\n")
            path = self.target / "boot/grub2/grubenv"
            self.write("boot/grub2/grubenv", path.read_text() + f"saved_entry={TEST_MACHINE_ID}-{version}\n")
        elif name == "dracut":
            output_path = next(value for value in command if value.startswith("/boot/initramfs-"))
            self.write(output_path.lstrip("/"), "regenerated target initramfs\n")
        elif name == "lsinitrd":
            if "--mod" in command:
                output = ("base\nsystemd\nsystemd-ask-password\n" + ("" if self.omit_crypt_module else "crypt\n")
                          + ("" if self.omit_systemd_cryptsetup else "systemd-cryptsetup\n"))
            elif "--file" in command:
                output = (self.initrd_crypttab_override if self.initrd_crypttab_override is not None else
                          (self.target / "etc/crypttab").read_text().replace("UUID=", "/dev/disk/by-uuid/"))
            else:
                output = "".join(f"{mode} 1 root root 1024 Jan 1 2026 {path}\n"
                                 for path, mode in self.initrd_files.items()
                                 if not (self.omit_unlock_program and path.endswith("/systemd-cryptsetup")))
                output += "".join(f"lrwxrwxrwx 1 root root 30 Jan 1 2026 {path} -> {target}\n"
                                  for path, target in self.initrd_symlinks.items())
        elif name == "grub2-mkconfig":
            self.write("boot/grub2/grub.cfg", "# generated by grub2-mkconfig\nblscfg\n")
        elif name == "gen_grub_cfgstub":
            if not self.omit_stub:
                self.write("boot/efi/EFI/fedora/grub.cfg", "# generated by gen_grub_cfgstub\nconfigfile $prefix/grub.cfg\n")
        elif name == "lsblk":
            output = "/dev/testdisk\n" if "PKNAME" in command else str(self.swap_sizes[command[-1]]) + "\n"
        elif name == "udevadm":
            output = "ID_PART_ENTRY_NUMBER=1\n"
        elif name == "efibootmgr":
            code = int(self.firmware_failure)
        elif name not in {"grubby", "depmod", "grub2-install"}:
            raise AssertionError(f"Unexpected external command: {command}")
        if check and code:
            raise backend.InstallError(f"Simulated command failure: {name}")
        return subprocess.CompletedProcess(argv, code, output, "")


class FedoraBootTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.target = self.root / "target"
        self.target.mkdir()
        self.config = BootConfig()
        self.runner = BootRunner(self.target)
        self.backend = backend.Backend(self.config, self.runner, self.target, self.root / "state", boot_id="boot-test")
        self.backend.state = {
            "boot_id": "boot-test", "target": str(self.target), "architecture": "x86_64",
            "uefi": True, "efi_suffix": "x64", "devices": ["/dev/testdisk"],
            "live_kernel_cmdline": "root=live:CDLABEL=Fedora rd.live.image rd.luks=0 nomodeset quiet",
            "mounts": [
                {"device": "/dev/testroot", "mountpoint": "/", "uuid": "aaaa-bbbb", "fstype": "ext4", "options": "defaults"},
                {"device": "/dev/testboot", "mountpoint": "/boot", "uuid": "cccc-dddd", "fstype": "ext4", "options": "defaults"},
                {"device": "/dev/testesp", "mountpoint": "/boot/efi", "uuid": "eeee-ffff", "fstype": "vfat", "options": "defaults"},
            ],
        }
        self.add_kernel(TEST_KERNEL)
        self.add_efi("x64")

    def add_kernel(self, version, name="vmlinuz"):
        self.runner.write(f"usr/lib/modules/{version}/{name}", f"Fedora kernel {version}\n")

    def add_efi(self, suffix):
        for package, name in ((f"shim-{suffix}", f"shim{suffix}.efi"),
                              (f"shim-{suffix}", f"mm{suffix}.efi"),
                              (f"grub2-efi-{suffix}", f"grub{suffix}.efi")):
            self.runner.write(f"usr/lib/efi/{package}/{name}", f"Fedora signed payload {name}\n")

    def commands(self, name):
        return [command[2:] if command[0] == "chroot" else command for command in self.runner.calls
                if (command[2] if command[0] == "chroot" else command[0]) == name]

    def prepare_boot(self):
        self.backend.install_kernels()

    def encrypted_root(self, filesystem="btrfs"):
        identifier = "12345678-1234-4abc-8def-123456789abc"
        mapper = "/dev/mapper/luks-" + identifier
        self.backend.state["encrypted_root"] = {"device": "/dev/testroot", "uuid": identifier,
                                                 "name": "luks-" + identifier, "mapper": mapper,
                                                 "fstype": filesystem, "filesystem_uuid": "aaaa-bbbb"}
        self.backend.state["mounts"][0].update(device=mapper, fstype=filesystem,
                                                options="subvol=root,compress=zstd:1" if filesystem == "btrfs" else "defaults")
        self.runner.write("etc/selinux/config", "SELINUX=enforcing\nSELINUXTYPE=targeted\n")
        self.runner.write("etc/crypttab", crypto.crypttab(self.backend.state))
        return identifier

    def test_encrypted_boot_and_rescue_use_luks_uuid_and_verified_crypt_initramfs(self):
        identifier = self.encrypted_root()
        self.prepare_boot()
        for name in (TEST_KERNEL, "0-rescue"):
            contents = (self.target / f"boot/loader/entries/{TEST_MACHINE_ID}-{name}.conf").read_text()
            self.assertIn("root=UUID=aaaa-bbbb", contents)
            self.assertIn("rd.luks.uuid=luks-" + identifier, contents)
            self.assertIn("rootflags=subvol=root", contents)
            self.assertNotIn("rd.luks=0", contents)
            self.assertNotIn("rd.luks.key", contents)
        dracut = (self.target / "etc/dracut.conf.d/90-fedora-dde-installer.conf").read_text()
        self.assertIn('add_dracutmodules+=" crypt systemd-cryptsetup "', dracut)
        self.assertIn('install_items+=" /etc/crypttab "', dracut)
        self.assertEqual(len(self.commands("lsinitrd")), 6)

    def test_encrypted_ext4_boot_needs_no_btrfs_option(self):
        self.encrypted_root("ext4")
        arguments = boot.kernel_arguments(self.backend)
        self.assertTrue(any(option.startswith("rd.luks.uuid=") for option in arguments))
        self.assertFalse(any(option.startswith("rootflags=") for option in arguments))

    def test_encrypted_initramfs_accepts_legacy_libexec_binary(self):
        self.encrypted_root()
        mode = self.runner.initrd_files.pop("usr/bin/systemd-cryptsetup")
        self.runner.initrd_files["usr/lib/systemd/systemd-cryptsetup"] = mode
        self.runner.initrd_symlinks.clear()
        crypto.verify_initramfs(self.backend, f"boot/initramfs-{TEST_KERNEL}.img")

    def test_encrypted_initramfs_rejects_missing_or_nonexecutable_unlock_components(self):
        self.encrypted_root()
        for path, mode in list(self.runner.initrd_files.items()):
            for missing in (False, True):
                if not missing and path.endswith(".service"):
                    continue
                with self.subTest(path=path, missing=missing):
                    if missing:
                        del self.runner.initrd_files[path]
                    else:
                        self.runner.initrd_files[path] = "-rw-r--r--"
                    with self.assertRaisesRegex(backend.InstallError, Path(path).name):
                        crypto.verify_initramfs(self.backend, f"boot/initramfs-{TEST_KERNEL}.img")
                    self.runner.initrd_files[path] = mode

    def test_missing_crypt_module_or_initramfs_keyfile_cannot_count_as_installed(self):
        self.encrypted_root()
        self.runner.omit_crypt_module = True
        with self.assertRaisesRegex(backend.InstallError, "crypt module"):
            self.prepare_boot()
        self.runner.omit_crypt_module = False
        self.runner.omit_systemd_cryptsetup = True
        with self.assertRaisesRegex(backend.InstallError, "systemd-cryptsetup module"):
            self.prepare_boot()
        self.runner.omit_systemd_cryptsetup = False
        self.runner.omit_unlock_program = True
        with self.assertRaisesRegex(backend.InstallError, "password unlock component"):
            self.prepare_boot()
        self.runner.omit_unlock_program = False
        self.runner.initrd_crypttab_override = crypto.crypttab(self.backend.state).replace(" none ", " /secret-key ")
        with self.assertRaisesRegex(backend.InstallError, "password-only"):
            self.prepare_boot()

    def test_encrypted_boot_rejects_disabled_or_changed_selinux_policy(self):
        self.encrypted_root()
        self.backend.state["install_profile"] = dict(ISO_PROFILE)
        with self.assertRaises(backend.InstallError):
            boot.kernel_arguments(self.backend)
        self.backend.state.pop("install_profile")
        self.runner.write("etc/selinux/config", "SELINUX=permissive\nSELINUXTYPE=targeted\n")
        with self.assertRaises(backend.InstallError):
            self.prepare_boot()

    def test_live_arguments_are_replaced_and_hardware_blacklist_persisted(self):
        self.backend.state["live_kernel_cmdline"] += " acpi=off modprobe.blacklist=nouveau,floppy selinux=0 enforcing=0 inst.stage2=bad"
        args = boot.kernel_arguments(self.backend)
        self.assertEqual(args[:2], ["root=UUID=aaaa-bbbb", "ro"])
        self.assertIn("nomodeset", args)
        self.assertIn("acpi=off", args)
        self.assertIn("modprobe.blacklist=nouveau,floppy", args)
        self.assertEqual(args[-2:], ["rhgb", "quiet"])
        self.assertFalse(any(arg.startswith(("rd.live", "root=live:", "rd.luks", "inst.", "selinux=", "enforcing=")) for arg in args))
        self.assertEqual((self.target / "etc/modprobe.d/anaconda-denylist.conf").read_text(), "blacklist nouveau\nblacklist floppy\n")

    def test_explicit_disabled_profile_reaches_normal_and_rescue_bls(self):
        self.backend.state["install_profile"] = dict(ISO_PROFILE)
        self.backend.state["live_kernel_cmdline"] += " selinux=1 enforcing=1"
        self.runner.write("etc/selinux/config", "SELINUX=disabled\nSELINUXTYPE=targeted\n")
        self.prepare_boot()
        for relative in ("etc/default/grub", "etc/kernel/cmdline",
                         f"boot/loader/entries/{TEST_MACHINE_ID}-{TEST_KERNEL}.conf",
                         f"boot/loader/entries/{TEST_MACHINE_ID}-0-rescue.conf"):
            with self.subTest(relative=relative):
                contents = (self.target / relative).read_text()
                self.assertEqual(contents.count("selinux=0"), 1)
                self.assertNotIn("selinux=1", contents)
                self.assertNotIn("enforcing=1", contents)

    def test_disabled_boot_generation_rejects_mismatched_target_policy(self):
        self.backend.state["install_profile"] = dict(ISO_PROFILE)
        self.runner.write("etc/selinux/config", "SELINUX=enforcing\n")
        with self.assertRaisesRegex(backend.InstallError, "SELINUX=disabled"):
            self.prepare_boot()
        self.assertEqual(self.runner.calls, [])

    def test_console_order_and_graphical_boot(self):
        self.backend.state["live_kernel_cmdline"] = "console=tty0 console=ttyS0,115200n8 console=ttyS1,9600"
        args = boot.kernel_arguments(self.backend)
        self.assertEqual([arg for arg in args if arg.startswith("console=")],
                         ["console=tty0", "console=ttyS0,115200n8", "console=ttyS1,9600"])
        self.assertNotIn("quiet", args)
        self.backend.state["live_kernel_cmdline"] = ""
        self.runner.has_plymouth = False
        self.assertNotIn("rhgb", boot.kernel_arguments(self.backend))

    def test_btrfs_root_and_largest_explicit_swap(self):
        self.backend.state["mounts"][0].update(fstype="btrfs", options="subvol=root,compress=zstd:1")
        self.backend.state["mounts"] += [
            {"device": "/dev/swap-small", "mountpoint": "swap", "uuid": "1111-2222"},
            {"device": "/dev/swap-large", "mountpoint": "swap", "uuid": "3333-4444"},
        ]
        args = boot.kernel_arguments(self.backend)
        self.assertIn("rootflags=subvol=root", args)
        self.assertIn("resume=UUID=3333-4444", args)
        self.assertNotIn("resume=UUID=1111-2222", args)

    def test_unconfigured_btrfs_and_unsafe_arguments_fail(self):
        for value in ('acpi="$(touch /tmp/pwned)"', "console=ttyS0;injected", "modprobe.blacklist=bad/module", "fips=1"):
            with self.subTest(value=value), self.assertRaises(backend.InstallError):
                self.backend.state["live_kernel_cmdline"] = value
                boot.kernel_arguments(self.backend)
        self.backend.state["live_kernel_cmdline"] = ""
        self.backend.state["mounts"][0]["fstype"] = "btrfs"
        with self.assertRaisesRegex(backend.InstallError, "subvolume"):
            boot.kernel_arguments(self.backend)

    def test_payload_requires_enabled_fedora_kernel_and_rescue_hooks(self):
        for name in ("20-grub.install", "50-dracut.install", "51-dracut-rescue.install"):
            self.runner.write("usr/lib/kernel/install.d/" + name, "#!/bin/sh\nexit 0\n")
            (self.target / "usr/lib/kernel/install.d" / name).chmod(0o755)
        self.runner.write("usr/lib/dracut/dracut.conf.d/02-rescue.conf", 'dracut_rescue_image="yes"\n')
        boot.verify_kernel_hooks(self.target)
        self.runner.write("etc/kernel/install.d/20-grub.install", "")
        with self.assertRaisesRegex(backend.InstallError, "active Fedora boot"):
            boot.verify_kernel_hooks(self.target)
        (self.target / "etc/kernel/install.d/20-grub.install").unlink()
        self.runner.write("etc/dracut.conf.d/02-rescue.conf", 'dracut_rescue_image="no"\n')
        with self.assertRaisesRegex(backend.InstallError, "rescue image generation"):
            boot.verify_kernel_hooks(self.target)
        (self.target / "etc/dracut.conf.d/02-rescue.conf").unlink()
        (self.target / "usr/lib/kernel/install.d/51-dracut-rescue.install").chmod(0o644)
        with self.assertRaisesRegex(backend.InstallError, "not executable"):
            boot.verify_kernel_hooks(self.target)

    def test_hook_cannot_reintroduce_live_arguments_into_bls(self):
        self.runner.extra_options = " rd.live.image root=live:CDLABEL=wrong"
        with self.assertRaisesRegex(backend.InstallError, "kernel parameters"):
            self.prepare_boot()

    def test_fedora_hooks_generate_entries_rescue_and_target_default_initramfs(self):
        self.runner.write("etc/dracut.conf.d/90-fedora-dde-installer.conf", 'hostonly="no"\n')
        self.prepare_boot()
        self.assertEqual(self.commands("kernel-install"), [["kernel-install", "add", TEST_KERNEL, f"/usr/lib/modules/{TEST_KERNEL}/vmlinuz"]])
        self.assertEqual(self.commands("depmod"), [["depmod", "-a", TEST_KERNEL]])
        self.assertTrue(self.commands("systemd-machine-id-setup"))
        self.assertFalse(any("--no-hostonly" in call or "-N" in call for call in self.commands("dracut")))
        self.assertNotIn("hostonly", (self.target / "etc/dracut.conf.d/90-fedora-dde-installer.conf").read_text())
        self.assertEqual((self.target / "etc/sysconfig/kernel").read_text(), "UPDATEDEFAULT=yes\nDEFAULTKERNEL=kernel-core\n")
        self.assertTrue((self.target / f"boot/loader/entries/{TEST_MACHINE_ID}-0-rescue.conf").is_file())
        self.assertEqual(self.backend.state["default_kernel"], TEST_KERNEL)

    def test_machine_id_is_generated_once_and_reused_on_retry(self):
        self.runner.write("etc/machine-id", "f" * 32 + "\n")
        self.runner.write("var/lib/dbus/machine-id", "e" * 32 + "\n")
        self.prepare_boot()
        self.prepare_boot()
        self.assertEqual(len(self.commands("systemd-machine-id-setup")), 1)
        self.assertEqual(self.backend.state["machine_id"], TEST_MACHINE_ID)
        self.assertEqual((self.target / "etc/kernel/entry-token").read_text().strip(), TEST_MACHINE_ID)
        self.assertEqual((self.target / "var/lib/dbus/machine-id").readlink(), Path("/etc/machine-id"))

    def test_changed_target_machine_id_cannot_be_silently_replaced(self):
        self.prepare_boot()
        self.runner.write("etc/machine-id", "f" * 32 + "\n")
        with self.assertRaisesRegex(backend.InstallError, "machine ID changed"):
            self.prepare_boot()
        self.assertEqual(len(self.commands("systemd-machine-id-setup")), 1)

    def test_missing_regular_or_rescue_entries_fail_even_if_commands_succeed(self):
        for flag in ("omit_bls", "omit_rescue"):
            with self.subTest(flag=flag):
                setattr(self.runner, flag, True)
                with self.assertRaises(backend.InstallError):
                    self.prepare_boot()
                setattr(self.runner, flag, False)
        self.assertNotIn("kernels", self.backend.state)

    def test_numeric_kernel_order_selects_newest_normal_kernel(self):
        old = self.target / "usr/lib/modules" / TEST_KERNEL
        shutil.rmtree(old)
        for version in ("6.9.12-200.fc45.x86_64", "6.10.1-200.fc45.x86_64", "6.10.1-200.fc45.x86_64+debug"):
            self.add_kernel(version)
        self.prepare_boot()
        self.assertEqual(self.backend.state["default_kernel"], "6.10.1-200.fc45.x86_64")
        self.assertEqual(self.commands("grubby")[-1][-1], "/boot/vmlinuz-6.10.1-200.fc45.x86_64")

    def test_aarch64_dtbloader_uses_kernel_install_and_its_actual_rpm_owner(self):
        shutil.rmtree(self.target / "usr/lib/modules")
        version = "6.17.1-300.fc45.aarch64"
        self.add_kernel(version, "vmlinuz-dtbloader.efi")
        self.backend.state.update(architecture="aarch64", efi_suffix="aa64")
        self.prepare_boot()
        self.assertEqual(self.commands("kernel-install")[0][-1], f"/usr/lib/modules/{version}/vmlinuz-dtbloader.efi")
        self.assertIn("DEFAULTKERNEL=kernel-uki-dtbloader", (self.target / "etc/sysconfig/kernel").read_text())
        call_names = [call[2] for call in self.runner.calls if call[0] == "chroot"]
        self.assertLess(call_names.index("dracut"), call_names.index("kernel-install"))

    def test_signed_uefi_payload_and_generated_stub_are_used(self):
        self.prepare_boot()
        self.backend.install_bootloader()
        self.assertEqual(self.commands("gen_grub_cfgstub"), [["gen_grub_cfgstub", "/boot/grub2", "/boot/efi/EFI/fedora"]])
        self.assertFalse(self.commands("grub2-install"))
        self.assertEqual((self.target / "boot/efi/EFI/fedora/shimx64.efi").read_bytes(),
                         (self.target / "usr/lib/efi/shim-x64/shimx64.efi").read_bytes())
        self.assertEqual((self.target / "boot/efi/EFI/BOOT/grub.cfg").read_bytes(),
                         (self.target / "boot/efi/EFI/fedora/grub.cfg").read_bytes())
        self.assertIn("saved_entry=", (self.target / "boot/grub2/grubenv").read_text())
        self.backend.install_bootloader()
        self.assertEqual(len(self.commands("efibootmgr")), 1)
        self.assertEqual(len([call for call in self.commands("grub2-editenv") if call[-1] == "create"]), 1)

    def test_firmware_failure_is_accepted_only_with_owned_complete_fallback(self):
        self.prepare_boot()
        self.runner.firmware_failure = True
        self.backend.install_bootloader()
        self.assertTrue(self.backend.state["firmware_fallback"])
        self.runner.write("boot/efi/EFI/BOOT/grubx64.efi", "changed by another system\n")
        with self.assertRaisesRegex(backend.InstallError, "fallback path"):
            self.backend.install_bootloader()
        self.assertEqual((self.target / "boot/efi/EFI/BOOT/grubx64.efi").read_text(), "changed by another system\n")

    def test_foreign_fallback_and_orphan_companions_are_preserved(self):
        self.prepare_boot()
        for filename in ("BOOTX64.EFI", "grub.cfg"):
            with self.subTest(filename=filename):
                fallback = self.target / "boot/efi/EFI/BOOT"
                if fallback.exists():
                    shutil.rmtree(fallback)
                self.runner.write("boot/efi/EFI/BOOT/" + filename, "foreign payload\n")
                self.backend.state.pop("firmware_entry_created", None)
                self.runner.firmware_failure = True
                with self.assertRaisesRegex(backend.InstallError, "fallback path"):
                    self.backend.install_bootloader()
                self.assertEqual((fallback / filename).read_text(), "foreign payload\n")
                self.assertNotIn("fallback_efi_hashes", self.backend.state)

    def test_partial_owned_fallback_can_be_completed_after_copy_failure(self):
        self.prepare_boot()
        original = boot._copy_efi

        def interrupt(source, destination):
            if destination.name == "grubx64.efi" and destination.parent.name == "BOOT":
                raise OSError("simulated interrupted copy")
            original(source, destination)

        with patch.object(boot, "_copy_efi", side_effect=interrupt), self.assertRaises(OSError):
            self.backend.install_bootloader()
        self.assertTrue(self.backend.state["fallback_efi_installing"])
        self.assertTrue((self.target / "boot/efi/EFI/BOOT/BOOTX64.EFI").is_file())
        self.runner.firmware_failure = True
        self.backend.install_bootloader()
        self.assertFalse(self.backend.state["fallback_efi_installing"])
        self.assertTrue(self.backend.state["firmware_fallback"])

    def test_missing_stub_cannot_reach_firmware_write(self):
        self.prepare_boot()
        self.runner.omit_stub = True
        with self.assertRaises(backend.InstallError):
            self.backend.install_bootloader()
        self.assertFalse(self.commands("efibootmgr"))

    def test_efi_source_and_destination_symlink_escapes_are_rejected(self):
        outside = self.root / "outside"
        outside.mkdir()
        (outside / "shimx64.efi").write_text("untrusted\n")
        payload = self.target / "usr/lib/efi/shim-x64/shimx64.efi"
        payload.unlink()
        payload.symlink_to(outside / "shimx64.efi")
        with self.assertRaises(backend.InstallError):
            self.backend.efi_file("shim-x64", "shimx64.efi")
        payload.unlink()
        self.add_efi("x64")
        self.prepare_boot()
        vendor = self.target / "boot/efi/EFI/fedora"
        vendor.parent.mkdir(parents=True, exist_ok=True)
        vendor.symlink_to(outside, target_is_directory=True)
        with self.assertRaises(backend.InstallError):
            self.backend.install_bootloader()
        self.assertFalse(self.commands("efibootmgr"))

    def test_mixed_mode_uefi_requires_ia32_payload(self):
        self.assertEqual(boot.firmware_suffix("x86_64", "32"), "ia32")
        self.assertEqual(boot.firmware_suffix("x86_64", "64"), "x64")
        self.assertEqual(boot.firmware_suffix("aarch64", "64"), "aa64")
        with self.assertRaises(backend.InstallError):
            boot.firmware_suffix("aarch64", "32")
        self.prepare_boot()
        self.backend.state["efi_suffix"] = "ia32"
        with self.assertRaisesRegex(backend.InstallError, "shim-ia32"):
            self.backend.install_bootloader()
        self.add_efi("ia32")
        self.backend.install_bootloader()
        self.assertTrue((self.target / "boot/efi/EFI/BOOT/BOOTIA32.EFI").is_file())
        self.assertEqual(self.commands("efibootmgr")[-1][-1], "\\EFI\\fedora\\shimia32.efi")

    def test_bios_uses_only_confirmed_disk(self):
        self.prepare_boot()
        self.backend.state["uefi"] = False
        self.config.values["DI_BOOT_DISK"] = "/dev/unconfirmed"
        with self.assertRaisesRegex(backend.InstallError, "confirmed installation plan"):
            self.backend.install_bootloader()
        self.assertFalse(self.commands("grub2-install"))
        self.config.values["DI_BOOT_DISK"] = "/dev/testdisk"
        self.backend.install_bootloader()
        self.assertEqual(self.commands("grub2-install"), [["grub2-install", "--target=i386-pc", "--recheck", "/dev/testdisk"]])


if __name__ == "__main__":
    unittest.main()
