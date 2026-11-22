#!/bin/bash
# Paths are fixed for the privileged installer; caller-provided DI_* values are
# never used as executable or configuration locations.
export DI_HOME_BIN_ENV=/usr/bin
export DI_HOME_DIR_ENV=/usr/share/fedora-dde-installer
export DI_TOOLS_DIR_ENV="$DI_HOME_DIR_ENV/tools"
export DI_CONFIG_DIR_ENV="$DI_HOME_DIR_ENV/configs"
export DI_SRC_DIR_ENV="$DI_HOME_DIR_ENV/src"
export DI_MODULE_DIR_ENV=/usr/lib/fedora-dde-installer/modules
export DI_CONFIG_FILE_ENV=/run/fedora-dde-installer/installer.conf
export DI_INSTALL_DIR_ENV="$DI_HOME_DIR_ENV"
export DI_INSTALL_TOOLS_DIR_ENV="$DI_TOOLS_DIR_ENV"
export DI_INSTALL_MODULE_DIR_ENV="$DI_MODULE_DIR_ENV"
export DI_LIVE_DIR_ENV=/run/initramfs/live
export DI_OEM_DIR="$DI_LIVE_DIR_ENV/oem"
export DI_TARGET_LIVE_DIR_ENV="$DI_LIVE_DIR_ENV"
DI_OS_VERSION_ENV="$(sed -n 's/^VERSION_ID=//p' /etc/os-release | tr -d '"')"
export DI_OS_VERSION_ENV
