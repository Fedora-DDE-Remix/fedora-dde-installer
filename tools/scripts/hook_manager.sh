#!/bin/bash
set -euo pipefail
export PATH=/usr/sbin:/usr/bin:/sbin:/bin
unset LD_PRELOAD LD_LIBRARY_PATH PYTHONPATH PYTHONHOME BASH_ENV ENV
source /usr/share/fedora-dde-installer/tools/scripts/init_environment.sh
[[ "$#" -eq 1 && "$EUID" -eq 0 ]] || exit 1
hook=$(realpath -e -- "$1")
case "$hook" in
    "$DI_INSTALL_TOOLS_DIR_ENV"/hooks/before_chroot/*.job|    "$DI_INSTALL_TOOLS_DIR_ENV"/hooks/in_chroot/*.job|    "$DI_INSTALL_TOOLS_DIR_ENV"/hooks/after_chroot/*.job|    "$DI_INSTALL_TOOLS_DIR_ENV"/hooks/user_config/*.job|    "$DI_INSTALL_TOOLS_DIR_ENV"/hooks/cleanup/*.job) ;;
    *) echo 'Refusing a hook outside the installed Fedora workflow.' >&2; exit 1 ;;
esac
# Never infer chroot from a directory's existence. The backend validates the
# target mount and the Live-session state before any target operation.
exec /bin/bash -e "$hook"
