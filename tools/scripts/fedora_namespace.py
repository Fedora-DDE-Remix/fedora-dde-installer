#!/usr/bin/python3
"""Run one privileged installation session in an isolated mount namespace.

The GUI or an authenticated automation shell is the long-lived parent. All
backend stages inherit its namespace; this helper never enters another process's
namespace, creates a keeper, or changes propagation in the Live host namespace.
"""

from __future__ import annotations

import ctypes
import json
import os
from pathlib import Path
import subprocess
import sys


sys.path.insert(0, str(Path(__file__).resolve().parent))
from fedora_backend import InstallError, protect_process_secrets, require


CLONE_NEWNS = 0x00020000


def namespace_identity(path: Path = Path("/proc/self/ns/mnt")) -> dict[str, int]:
    value = path.stat()
    return {"device": value.st_dev, "inode": value.st_ino}


def verify_private_namespace() -> dict[str, int]:
    """Reject the host and any mount tree with a propagation entrance or exit."""
    require(os.geteuid() == 0, "The installer mount namespace requires root privileges.")
    identity = namespace_identity()
    require(identity != namespace_identity(Path("/proc/1/ns/mnt")),
            "Start the installer through its launcher, or run all CLI stages in one "
            "fedora_namespace.py --exec session; the Live host mount namespace is forbidden.")
    root_seen = False
    with Path("/proc/self/mountinfo").open("r", encoding="utf-8") as stream:
        information = stream.read(1024 * 1024 + 1)
    require(len(information) <= 1024 * 1024, "Cannot verify an unexpectedly large mount namespace.")
    for line in information.splitlines():
        before, separator, after = line.partition(" - ")
        fields = before.split()
        require(separator and len(fields) >= 6 and len(after.split()) >= 3,
                "Cannot verify malformed kernel mount information.")
        require(not any(item.startswith(("shared:", "master:", "propagate_from:")) for item in fields[6:]),
                "The installer mount namespace must be recursively private before any installation mount.")
        root_seen |= fields[4] == "/"
    require(root_seen, "The installer mount namespace has no visible root mount.")
    return identity


def enter_private_namespace() -> dict[str, int]:
    """Create a fresh namespace, isolate it, then verify before running a child."""
    require(os.geteuid() == 0, "The installer mount namespace requires root privileges.")
    before = namespace_identity()
    unshare = ctypes.CDLL(None, use_errno=True).unshare
    unshare.argtypes = [ctypes.c_int]
    unshare.restype = ctypes.c_int
    require(unshare(CLONE_NEWNS) == 0, "Cannot create the isolated installer mount namespace.")
    require(namespace_identity() != before, "The installer mount namespace did not change.")
    # This is deliberately after unshare. Applying this to the Live host would
    # change unrelated services' mount propagation and would not be acceptable.
    result = subprocess.run(["/usr/bin/mount", "--make-rprivate", "/"], check=False,
                            stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
                            text=True, timeout=15)
    require(result.returncode == 0, "Cannot make the isolated installer mount tree private.")
    return verify_private_namespace()


def main() -> int:
    os.umask(0o077)
    try:
        require(os.geteuid() == 0, "The installer mount namespace requires root privileges.")
        protect_process_secrets()
        arguments = sys.argv[1:]
        if arguments == ["--check"]:
            print(json.dumps(verify_private_namespace(), sort_keys=True), flush=True)
            return 0
        require(len(arguments) >= 2 and arguments[0] == "--exec",
                "Use --check or --exec followed by the privileged session command.")
        for key in ("LD_PRELOAD", "LD_LIBRARY_PATH", "PYTHONPATH", "PYTHONHOME", "BASH_ENV", "ENV"):
            os.environ.pop(key, None)
        os.environ["PATH"] = "/usr/sbin:/usr/bin:/sbin:/bin"
        enter_private_namespace()
        os.execvp(arguments[1], arguments[1:])
    except (InstallError, OSError, ValueError, AttributeError, subprocess.TimeoutExpired) as error:
        print(f"Fedora DDE Installer: {error}", file=sys.stderr, flush=True)
        return 1
    return 1


if __name__ == "__main__":
    sys.exit(main())
