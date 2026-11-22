"""Offline Fedora system configuration helpers.

Keyboard conversion follows systemd's vconsole-util.c, using the payload's
kbd-model-map and keymaps.  The Live desktop's localed settings are never changed.
"""

from __future__ import annotations

import os
from pathlib import Path
import re

from fedora_backend import InstallError, require


KEYMAP_DIRS = ("usr/lib/kbd/keymaps", "usr/share/kbd/keymaps", "usr/share/keymaps")
FONT_DIRS = ("usr/lib/kbd/consolefonts", "usr/share/kbd/consolefonts", "usr/share/consolefonts")
NAME = re.compile(r"[A-Za-z0-9_+.,()\-]+\Z")


def clean_live_autologin(contents: str, live_user: str) -> str:
    """Clear just a Live user's INI autologin stanza, preserving other settings."""
    sections = re.split(r"(?m)(?=^\s*\[[^\n]+\]\s*$)", contents)
    result = []
    for section in sections:
        lines = section.splitlines(keepends=True)
        header = next((line.strip().lower() for line in lines if line.lstrip().startswith("[")), "")
        user_keys = {"autologin-user", "automaticlogin", "automatic-login", "autologin"}
        if header == "[autologin]":
            user_keys.add("user")
        entries = [re.match(r"\s*([A-Za-z-]+)\s*=\s*(.*?)\s*$", line) for line in lines]
        live = any(match and match[1].lower() in user_keys
                   and match[2].strip("\"'") == live_user for match in entries)
        if not live:
            result.append(section)
            continue
        for line, match in zip(lines, entries):
            key = match[1].lower() if match else ""
            if key in user_keys:
                result.append(f"{match[1]}=\n")
            elif key in {"automaticloginenable", "automatic-login-enable", "relogin"}:
                result.append(f"{match[1]}=false\n")
            elif header == "[autologin]" and key == "session":
                result.append(f"{match[1]}=\n")
            else:
                result.append(line)
    return "".join(result)


def target_path(root: Path, relative: str) -> Path:
    """Resolve payload symlinks with chroot semantics, without reading the host."""
    require(not Path(relative).is_absolute() and ".." not in Path(relative).parts,
            "Invalid payload configuration path.")
    root = root.resolve()
    path, remaining, links = root, list(Path(relative).parts), 0
    while remaining:
        name = remaining.pop(0)
        if name == "..":
            path = path.parent if path != root else root
            continue
        if name in ("", ".", "/"):
            continue
        path /= name
        if path.is_symlink():
            links += 1
            require(links <= 40, "A payload configuration symlink is cyclic.")
            link = Path(os.readlink(path))
            path = root if link.is_absolute() else path.parent
            remaining = list(link.parts) + remaining
    require(path.is_relative_to(root), "A payload configuration path escaped the target.")
    return path


def verify_fedora_pam(root: Path) -> None:
    """Require the Fedora DDM RPM's stack; authselect remains package-managed."""
    pam = target_path(root, "etc/pam.d/ddm")
    require(pam.is_file(), "The Fedora DDM package is missing its PAM configuration.")
    lines = [line.split() for line in pam.read_text().splitlines()
             if line.strip() and not line.lstrip().startswith("#")]
    required = (("auth", "password-auth"), ("auth", "postlogin"),
                ("account", "password-auth"), ("password", "password-auth"),
                ("session", "password-auth"), ("session", "postlogin"))
    require(all(any(len(row) >= 3 and row[0].lstrip("-") == kind
                        and row[1] in {"include", "substack"} and row[2] == stack
                    for row in lines) for kind, stack in required),
            "The Live image needs the Fedora DDM PAM package (ddm >= 0.5.0-6).")
    require(not any(token.startswith("common-") or token == "system-login"
                    for row in lines for token in row),
            "DDM still references a non-Fedora PAM stack. Rebuild the DDM RPM for Fedora.")
    sessions = [row for row in lines if row[0].lstrip("-") == "session"]
    close = next((i for i, row in enumerate(sessions)
                  if len(row) >= 4 and row[2] == "pam_selinux.so" and "close" in row[3:]), -1)
    login = next((i for i, row in enumerate(sessions)
                  if len(row) >= 3 and row[2] == "pam_loginuid.so"), -1)
    opened = next((i for i, row in enumerate(sessions)
                   if len(row) >= 4 and row[2] == "pam_selinux.so" and "open" in row[3:]), -1)
    require(0 <= close < login < opened,
            "The DDM RPM must configure Fedora SELinux and loginuid PAM sessions.")
    for include in ("password-auth", "postlogin"):
        require(target_path(root, "etc/pam.d/" + include).is_file(),
                "The payload's authselect PAM configuration is incomplete.")


def verify_selinux_source(root: Path) -> None:
    """Require an active SELinux LSM and a Fedora-labelled read-only payload.

    The later Anaconda-style targeted restorecon pass relies on rsync preserving
    the payload's existing labels. This is not support for unlabelled EROFS.
    """
    try:
        enforcing = Path("/sys/fs/selinux/enforce").read_text(encoding="ascii").strip()
    except (OSError, UnicodeError) as error:
        raise InstallError("Boot the Live installer with SELinux enabled; selinux=0 cannot preserve Fedora labelling.") from error
    require(enforcing in {"0", "1"}, "Cannot determine the Live kernel's SELinux mode.")
    expected = {"usr/bin/bash": "shell_exec_t", "usr/lib/systemd/systemd": "init_exec_t",
                "etc/passwd": "passwd_file_t"}
    for relative, file_type in expected.items():
        path = target_path(root, relative)
        require(path.is_file(), f"The payload is missing the Fedora system file {relative}.")
        try:
            context = os.getxattr(path, "security.selinux", follow_symlinks=False).rstrip(b"\0").decode("ascii")
        except (OSError, UnicodeError) as error:
            raise InstallError("The Live payload lacks Fedora SELinux labels. Recompose the ISO preserving security.selinux xattrs.") from error
        fields = context.split(":", 3)
        require(len(fields) == 4 and fields[0] and fields[1] == "object_r"
                and fields[2] == file_type and fields[3],
                f"The Live payload's {relative} SELinux label is invalid; recompose the ISO with Fedora labels.")


def _installed_keymaps(root: Path) -> tuple[set[str], set[str]]:
    installed, converted = set(), set()
    for relative in KEYMAP_DIRS:
        directory = target_path(root, relative)
        if not directory.is_dir():
            continue
        for path in directory.rglob("*.map*"):
            name = path.name
            if name.endswith(".gz"):
                name = name[:-3]
            if not name.endswith(".map"):
                continue
            name = name[:-4]
            if not NAME.fullmatch(name) or not target_path(root, str(path.relative_to(root.resolve()))).is_file():
                continue
            installed.add(name)
            if path.parent.name == "xkb":
                converted.add(name)
    return installed, converted


def console_keymap(root: Path, layout: str, variant: str) -> str:
    """Convert XKB to an installed console map with systemd's matching order."""
    require(bool(re.fullmatch(r"[a-z0-9_-]+(?:,[a-z0-9_-]+)*", layout))
            and bool(re.fullmatch(r"[a-zA-Z0-9_,()-]*", variant)), "Invalid keyboard layout.")
    installed, converted = _installed_keymaps(root)

    def converted_map(xlayout: str, xvariant: str) -> str | None:
        name = xlayout + ("-" + xvariant if xvariant else "")
        return name if name in converted else None

    result = converted_map(layout, variant)
    if result:
        return result
    mapping = target_path(root, "usr/share/systemd/kbd-model-map")
    require(mapping.is_file(), "The payload lacks systemd's keyboard conversion table.")
    best, score = None, 0
    for line in mapping.read_text().splitlines():
        columns = line.split("#", 1)[0].split()
        if len(columns) < 5 or not NAME.fullmatch(columns[0]):
            continue
        keymap, xlayout, _model, xvariant, _options = columns[:5]
        matching = (10 if layout == xlayout else
                    9 if layout == ",".join(reversed(xlayout.split(","))) else
                    5 if layout.startswith(xlayout + ",") else
                    1 if layout.split(",", 1)[0] == xlayout.split(",", 1)[0] else 0)
        if matching:
            matching += 1  # The UI does not choose an XKB hardware model.
            if variant == xvariant or (not variant.strip(",") and xvariant == "-"):
                matching += 1
        if matching > score and keymap in installed:
            best, score = keymap, matching
    if score < 9:
        best = converted_map(layout.split(",", 1)[0], variant.split(",", 1)[0]) or best
    if not best and variant:
        best = converted_map(layout, "")
    require(best is not None,
            "The payload lacks a console keymap for the selected keyboard. Include the matching kbd keymaps in the ISO.")
    return best


def font_candidates(locale: str) -> list[str]:
    try:
        import langtable
    except ImportError as error:
        raise InstallError("The Live image needs python3-langtable for Fedora console font selection.") from error
    return langtable.list_consolefonts(languageId=locale)


def console_font(root: Path, locale: str) -> str:
    # Anaconda chooses langtable's first candidate or eurlatgr when no locale
    # specific font is available. Do not force a font from the Live session.
    candidates = font_candidates(locale)
    font = candidates[0] if candidates else "eurlatgr"
    require(bool(NAME.fullmatch(font)), "The selected console font name is invalid.")
    suffixes = (".psfu.gz", ".psf.gz", ".psfu", ".psf")
    require(any(target_path(root, f"{directory}/{font}{suffix}").is_file()
                for directory in FONT_DIRS for suffix in suffixes),
            "The payload lacks the selected language's console font. Include the matching kbd fonts in the ISO.")
    return font
