"""Explicit image-policy validation using ordinary temporary files only."""

import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools/scripts"))
import fedora_backend as backend
import fedora_profile as profile
from profile_fixtures import ISO_PROFILE, image_owner


class ProfileTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.path = self.root / "etc/fedora-dde-installer/install-profile.json"
        self.path.parent.mkdir(parents=True)

    def write(self, value=ISO_PROFILE):
        self.path.write_text(json.dumps(value))
        self.path.chmod(0o644)

    def test_absent_profile_and_older_state_keep_enforcing_defaults(self):
        self.assertEqual(profile.read_profile(self.root), {"schema": 1, "selinux": "enforcing", "pam": "fedora"})
        self.assertFalse(profile.selinux_disabled({}))

    def test_explicit_owned_profile_is_required_for_disabled_mode(self):
        self.write()
        with image_owner():
            selected = profile.read_profile(self.root)
        self.assertEqual(selected, ISO_PROFILE)
        self.assertTrue(profile.selinux_disabled({"install_profile": selected}))

    def test_schema_modes_unknown_keys_and_duplicate_keys_fail_closed(self):
        invalid = [None, [], {}, dict(ISO_PROFILE, schema=True), dict(ISO_PROFILE, schema=2),
                   dict(ISO_PROFILE, selinux="permissive"), dict(ISO_PROFILE, pam="fedora"),
                   dict(ISO_PROFILE, unexpected="ignored")]
        for value in invalid:
            with self.subTest(value=value), image_owner(), self.assertRaises(backend.InstallError):
                self.write(value)
                profile.read_profile(self.root)
        self.path.write_text('{"schema":1,"selinux":"enforcing","selinux":"disabled","pam":"ddm-system-login"}')
        with image_owner(), self.assertRaises(backend.InstallError):
            profile.read_profile(self.root)
        with self.assertRaises(backend.InstallError):
            profile.state_profile({"install_profile": {"selinux": "disabled"}})

    def test_user_owned_writable_and_linked_profiles_are_rejected(self):
        self.write()
        with image_owner(1000), self.assertRaises(backend.InstallError):
            profile.read_profile(self.root)
        for path in (self.path, self.path.parent, self.root / "etc"):
            old_mode = path.stat().st_mode & 0o777
            path.chmod(0o777)
            with self.subTest(path=path), image_owner(), self.assertRaises(backend.InstallError):
                profile.read_profile(self.root)
            path.chmod(old_mode)
        real = self.path.with_suffix(".real")
        self.path.rename(real)
        self.path.symlink_to(real.name)
        with image_owner(), self.assertRaises(backend.InstallError):
            profile.read_profile(self.root)

    def test_malformed_and_oversized_files_are_not_silently_ignored(self):
        for contents in ("{broken", " " * 4097):
            self.path.write_text(contents)
            with self.subTest(contents=contents[:10]), image_owner(), self.assertRaises(backend.InstallError):
                profile.read_profile(self.root)


if __name__ == "__main__":
    unittest.main()
