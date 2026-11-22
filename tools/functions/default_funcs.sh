#!/bin/bash
installer_get() {
    /usr/bin/fedora-dde-installer-config get "$DI_CONFIG_FILE_ENV" "${2:-}" "$1"
}
installer_set() {
    /usr/bin/fedora-dde-installer-config set "$DI_CONFIG_FILE_ENV" "${3:-}" "$1" "$2"
}
init_config() {
    local directory=/run/fedora-dde-installer
    [[ ! -L "$directory" ]] || return 1
    install -d -m 0700 -o root -g root "$directory"
    [[ ! -L "$DI_CONFIG_FILE_ENV" ]] || return 1
    if [[ ! -f "$DI_CONFIG_FILE_ENV" ]]; then
        /usr/bin/fedora-dde-installer-config init "$DI_CONFIG_FILE_ENV"
    fi
    chown root:root "$DI_CONFIG_FILE_ENV"
    chmod 0600 "$DI_CONFIG_FILE_ENV"
    # Installation always proceeds to account setup in this Live session.
    installer_set DI_INSTALL_MODE default
    installer_set DI_REBOOT_AFTER_SETUP false
    installer_set DI_PASSWORD_ENCRYPTION_CONFIG false
    if [[ -d /sys/firmware/efi ]]; then
        installer_set DI_BOOTLOADER_IS_EFI true
    else
        installer_set DI_BOOTLOADER_IS_EFI false
    fi
}
init_hookslist() {
    local hooks="$DI_INSTALL_TOOLS_DIR_ENV/hooks"
    installer_set DI_HOOKS_LIST "$hooks/before_chroot;$hooks/in_chroot;$hooks/after_chroot"
    installer_set DI_FB_HOOHS_LIST "$hooks/user_config;$hooks/cleanup"
}
is_livecd() { grep -Eq '(^| )(rd.live.image|root=live:)' /proc/cmdline; }
is_livecd_install() { is_livecd; }
get_file_path() { dirname -- "$1"; }
program_exist() { command -v "$1" >/dev/null 2>&1; }
