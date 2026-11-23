"""Pinned image-integration fixtures; tests never alter host PAM or ownership."""

from contextlib import contextmanager
import os
from pathlib import Path
from unittest.mock import patch


ISO_PROFILE = {"schema": 1, "selinux": "disabled", "pam": "ddm-system-login"}

# linuxdeepin/ddm 0.5.0, commit 53d1e8b1a9f46f209da60f259edf30874393090e,
# services/ddm.pam, as installed unmodified by the maintainer's Fedora RPM.
MAINTAINER_DDM_PAM = """#%PAM-1.0
auth        sufficient  pam_succeed_if.so user ingroup nopasswdlogin
auth        include     system-login
-auth       optional    pam_deepin_keyring.so
-auth       optional    pam_gnome_keyring.so
-auth       optional    pam_kwallet5.so
account     include     system-login
password    include     system-login
-password   optional    pam_gnome_keyring.so use_authtok
session     optional    pam_keyinit.so force revoke
session     include     system-login
session     required    pam_systemd.so
-session    optional    pam_deepin_keyring.so
-session    optional    pam_gnome_keyring.so auto_start
-session    optional    pam_kwallet5.so auto_start
"""

# ISO-Build's separate configuration RPM owns this bridge, not DDM's files.
FEDORA_BRIDGE_PAM = """#%PAM-1.0
auth substack password-auth
auth include postlogin
account required pam_nologin.so
account include password-auth
password include password-auth
session required pam_selinux.so close
session required pam_loginuid.so
session required pam_selinux.so open
session optional pam_keyinit.so force revoke
session required pam_namespace.so
session include password-auth
session include postlogin
"""


@contextmanager
def image_owner(uid=0):
    """Model RPM-owned image files even on an unprivileged CI account."""
    original = Path.lstat

    def lstat(path, *args, **kwargs):
        fields = list(original(path, *args, **kwargs))
        fields[4] = uid
        return os.stat_result(fields)

    with patch.object(Path, "lstat", lstat):
        yield
