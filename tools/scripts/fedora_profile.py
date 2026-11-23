"""Explicit image policy, read from the validated read-only payload only."""

from __future__ import annotations

import json
from pathlib import Path
import stat

from fedora_backend import InstallError, require


PROFILE_PATH = "etc/fedora-dde-installer/install-profile.json"
DEFAULT_PROFILE = {"schema": 1, "selinux": "enforcing", "pam": "fedora"}
DDM_ISO_PROFILE = {"schema": 1, "selinux": "disabled", "pam": "ddm-system-login"}


def validate_profile(value: object) -> dict:
    require(isinstance(value, dict) and set(value) == {"schema", "selinux", "pam"}
            and type(value["schema"]) is int and value["schema"] == 1
            and value in (DEFAULT_PROFILE, DDM_ISO_PROFILE),
            "The image installation profile is unsupported or malformed.")
    return dict(value)


def _unique_keys(pairs: list[tuple]) -> dict:
    result = {}
    for key, value in pairs:
        require(key not in result, "The image installation profile has duplicate keys.")
        result[key] = value
    return result


def _require_trusted(path: Path, *, directory: bool = False) -> None:
    metadata = path.lstat()
    require((stat.S_ISDIR(metadata.st_mode) if directory else stat.S_ISREG(metadata.st_mode))
            and metadata.st_uid == 0 and not stat.S_IMODE(metadata.st_mode) & 0o022,
            "The image installation profile and its directories must be root-owned and not writable by other users.")
    if not directory:
        require(metadata.st_size <= 4096, "The image installation profile is too large.")


def read_profile(root: Path) -> dict:
    """Call only after verifying that root is a distinct read-only image mount.

    No environment variable, kernel argument, running Live configuration or
    target file can select a weaker policy. Symlinks are not accepted here.
    """
    path = root / PROFILE_PATH
    if not path.exists() and not path.is_symlink():
        return dict(DEFAULT_PROFILE)
    for directory in (root / "etc", root / "etc/fedora-dde-installer"):
        _require_trusted(directory, directory=True)
    _require_trusted(path)
    try:
        return validate_profile(json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=_unique_keys))
    except (ValueError, UnicodeError) as error:
        raise InstallError("The image installation profile is not valid UTF-8 JSON.") from error


def state_profile(state: dict) -> dict:
    # Older in-session state has no opt-in and must retain the enforcing policy.
    return validate_profile(state.get("install_profile", DEFAULT_PROFILE))


def selinux_disabled(state: dict) -> bool:
    return state_profile(state)["selinux"] == "disabled"
