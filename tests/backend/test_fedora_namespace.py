"""Namespace policy tests with no unshare, mount, block-device or GUI actions."""

import contextlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools/scripts"))
import fedora_backend as backend
import fedora_namespace as namespace


HOST = {"device": 4, "inode": 100}
SESSION = {"device": 4, "inode": 200}


class NamespacePolicyTests(unittest.TestCase):
    def setUp(self):
        self.enterContext(mock.patch.object(namespace.os, "geteuid", return_value=0))
        self.identities = self.enterContext(mock.patch.object(namespace, "namespace_identity",
            side_effect=lambda path=None: HOST if path and str(path) == "/proc/1/ns/mnt" else SESSION))
        self.mountinfo = "1 1 0:1 / / rw - overlay LiveOS_rootfs rw\n2 1 0:2 / /dev rw - devtmpfs devtmpfs rw\n"
        self.enterContext(mock.patch.object(Path, "open", side_effect=lambda *a, **k: io.StringIO(self.mountinfo)))

    def test_private_non_host_namespace_returns_the_kernel_identity(self):
        self.assertEqual(namespace.verify_private_namespace(), SESSION)

    def test_host_namespace_is_rejected_even_without_shared_mounts(self):
        self.identities.side_effect = None
        self.identities.return_value = HOST
        with self.assertRaisesRegex(backend.InstallError, "Live host mount namespace"):
            namespace.verify_private_namespace()

    def test_non_root_cannot_check_or_create_an_installer_namespace(self):
        with mock.patch.object(namespace.os, "geteuid", return_value=1000), \
                mock.patch.object(namespace.ctypes, "CDLL") as libc:
            with self.assertRaises(backend.InstallError):
                namespace.verify_private_namespace()
            with self.assertRaises(backend.InstallError):
                namespace.enter_private_namespace()
        libc.assert_not_called()

    def test_shared_or_slave_submounts_are_rejected_even_if_root_is_private(self):
        for field in ("shared:9", "master:9", "propagate_from:9"):
            with self.subTest(field=field):
                self.mountinfo = "1 1 0:1 / / rw - overlay LiveOS_rootfs rw\n" + (
                    f"2 1 0:2 / /dev rw {field} - devtmpfs devtmpfs rw\n")
                with self.assertRaisesRegex(backend.InstallError, "recursively private"):
                    namespace.verify_private_namespace()

    def test_missing_root_and_malformed_or_oversized_mountinfo_fail_closed(self):
        for content in ("", "broken\n", "2 1 0:2 / /dev rw - devtmpfs devtmpfs rw\n", "x" * (1024 * 1024 + 1)):
            with self.subTest(length=len(content)):
                self.mountinfo = content
                with self.assertRaises(backend.InstallError):
                    namespace.verify_private_namespace()

    def test_unshare_happens_before_any_propagation_change(self):
        calls = []
        unshare = mock.Mock(side_effect=lambda flags: calls.append(("unshare", flags)) or 0)
        self.identities.side_effect = [HOST, SESSION]
        with mock.patch.object(namespace.ctypes, "CDLL", return_value=mock.Mock(unshare=unshare)), \
                mock.patch.object(namespace.subprocess, "run", side_effect=lambda argv, **kwargs:
                                  calls.append(("mount", argv)) or subprocess.CompletedProcess(argv, 0)), \
                mock.patch.object(namespace, "verify_private_namespace", return_value=SESSION):
            self.assertEqual(namespace.enter_private_namespace(), SESSION)
        self.assertEqual(calls, [("unshare", namespace.CLONE_NEWNS),
                                 ("mount", ["/usr/bin/mount", "--make-rprivate", "/"])])

    def test_unshare_failure_never_changes_host_mount_propagation(self):
        unshare = mock.Mock(return_value=-1)
        with mock.patch.object(namespace.ctypes, "CDLL", return_value=mock.Mock(unshare=unshare)), \
                mock.patch.object(namespace.subprocess, "run") as mount:
            with self.assertRaisesRegex(backend.InstallError, "Cannot create"):
                namespace.enter_private_namespace()
        mount.assert_not_called()

    def test_unchanged_namespace_is_rejected_before_mount(self):
        self.identities.side_effect = [HOST, HOST]
        with mock.patch.object(namespace.ctypes, "CDLL", return_value=mock.Mock(unshare=mock.Mock(return_value=0))), \
                mock.patch.object(namespace.subprocess, "run") as mount:
            with self.assertRaisesRegex(backend.InstallError, "did not change"):
                namespace.enter_private_namespace()
        mount.assert_not_called()

    def test_failed_private_remount_does_not_run_session_command(self):
        self.identities.side_effect = [HOST, SESSION]
        with mock.patch.object(namespace.ctypes, "CDLL", return_value=mock.Mock(unshare=mock.Mock(return_value=0))), \
                mock.patch.object(namespace.subprocess, "run", return_value=subprocess.CompletedProcess([], 1)), \
                mock.patch.object(namespace, "protect_process_secrets"), \
                mock.patch.object(namespace.os, "execvp") as execute, \
                mock.patch.object(namespace.sys, "argv", ["fedora_namespace.py", "--exec", "/bin/false"]), \
                contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(namespace.main(), 1)
        execute.assert_not_called()

    def test_check_cli_emits_only_one_identity_json_object(self):
        output = io.StringIO()
        with mock.patch.object(namespace, "protect_process_secrets"), \
                mock.patch.object(namespace.sys, "argv", ["fedora_namespace.py", "--check"]), \
                contextlib.redirect_stdout(output):
            self.assertEqual(namespace.main(), 0)
        self.assertEqual(json.loads(output.getvalue()), SESSION)
        self.assertEqual(len(output.getvalue().splitlines()), 1)

    def test_backend_host_rejection_precedes_config_or_partition_access(self):
        with mock.patch.object(backend, "protect_process_secrets"), \
                mock.patch.object(namespace, "verify_private_namespace", side_effect=backend.InstallError("host rejected")), \
                mock.patch.object(backend, "Config") as config, \
                mock.patch.object(backend.sys, "argv", ["fedora_backend.py", "preflight"]), \
                contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(backend.main(), 1)
        config.assert_not_called()


class NamespaceStateTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(self.enterContext(tempfile.TemporaryDirectory()))
        with mock.patch.object(namespace, "namespace_identity", return_value=SESSION):
            self.backend = backend.Backend(mock.Mock(), mock.Mock(), self.root / "target",
                                           self.root / "runtime", "test-boot")
        self.backend.state = {"boot_id": "test-boot", "target": str(self.root / "target"),
                              "owned_mounts": [], "owned_mappers": []}
        self.backend.save("preflight")

    def test_independent_stage_objects_reuse_same_namespace_state(self):
        with mock.patch.object(namespace, "namespace_identity", return_value=SESSION):
            following = backend.Backend(mock.Mock(), mock.Mock(), self.root / "target",
                                         self.root / "runtime", "test-boot")
        following.load("preflight")
        self.assertEqual(following.state["mount_namespace"], SESSION)

    def test_stage_in_a_different_namespace_cannot_use_or_overwrite_active_state(self):
        original = self.backend.state_file.read_bytes()
        with mock.patch.object(namespace, "namespace_identity", return_value=HOST):
            other = backend.Backend(mock.Mock(), mock.Mock(), self.root / "target",
                                     self.root / "runtime", "test-boot")
        with self.assertRaisesRegex(backend.InstallError, "another mount namespace"):
            other.load("preflight")
        self.assertEqual(self.backend.state_file.read_bytes(), original)

    def test_old_state_without_namespace_is_not_adopted_for_an_active_installation(self):
        value = json.loads(self.backend.state_file.read_text())
        value.pop("mount_namespace")
        self.backend.state_file.write_text(json.dumps(value))
        with self.assertRaisesRegex(backend.InstallError, "another mount namespace"):
            self.backend.load("preflight", allow_finalized_namespace=True)

    def test_only_fully_finalized_state_can_start_a_new_namespace_session(self):
        self.backend.save("finalized")
        self.backend.mount_namespace = HOST
        self.backend.load(allow_finalized_namespace=True)
        for key in ("owned_mounts", "owned_mappers"):
            with self.subTest(key=key):
                self.backend.state[key] = ["still-owned"]
                self.backend.save()
                with self.assertRaises(backend.InstallError):
                    self.backend.load(allow_finalized_namespace=True)
                self.backend.state[key] = []
                self.backend.save()

    def test_boot_identity_still_rejects_reuse_before_namespace_checks(self):
        self.backend.boot_id = "another-boot"
        with self.assertRaisesRegex(backend.InstallError, "different session"):
            self.backend.load("preflight")


if __name__ == "__main__":
    unittest.main()
