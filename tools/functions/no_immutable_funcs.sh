#!/bin/bash

is_no_immutable_install() {
    local no_immutable=$(installer_get "DI_NO_IMMUTABLE_INSTALL")
    local immutable_system=$(installer_get "DI_IMMUTABLE_SYSTEM")

    [[ "${no_immutable}" == "true" || "${immutable_system}" == "false" ]]
}

restore_no_immutable_real_file() {
    local root="$1"
    local rel="$2"

    if [[ -e "${root}/${rel}.real" || -L "${root}/${rel}.real" ]]; then
        rm -f "${root:?}/${rel}"
        mv "${root}/${rel}.real" "${root}/${rel}"
    fi
}

remove_no_immutable_pkg_paragraphs() {
    local file="$1"
    [[ -f "${file}" ]] || return 0

    local tmp
    tmp=$(mktemp)
    awk '
        BEGIN { RS=""; ORS="\n\n" }
        {
            pkg=""
            n=split($0, lines, "\n")
            for (i = 1; i <= n; i++) {
                if (lines[i] ~ /^Package: /) {
                    pkg=substr(lines[i], 10)
                    break
                }
            }
            if (pkg != "deepin-immutable-boot" && pkg != "deepin-immutable-ctl") {
                print
            }
        }
    ' "${file}" > "${tmp}"
    cat "${tmp}" > "${file}"
    rm -f "${tmp}"
}

remove_no_immutable_diversions() {
    local file="$1"
    [[ -f "${file}" ]] || return 0

    local tmp
    tmp=$(mktemp)
    awk '
        NR % 3 == 1 { a=$0; next }
        NR % 3 == 2 { b=$0; next }
        NR % 3 == 0 {
            c=$0
            if (c != "deepin-immutable-ctl") {
                print a
                print b
                print c
            }
        }
    ' "${file}" > "${tmp}"
    cat "${tmp}" > "${file}"
    rm -f "${tmp}"
}

cleanup_no_immutable_dpkg_db() {
    local db="$1"
    [[ -d "${db}" ]] || return 0

    remove_no_immutable_pkg_paragraphs "${db}/status"
    remove_no_immutable_pkg_paragraphs "${db}/status-old"
    remove_no_immutable_pkg_paragraphs "${db}/available"
    remove_no_immutable_pkg_paragraphs "${db}/available-old"
    remove_no_immutable_diversions "${db}/diversions"
    remove_no_immutable_diversions "${db}/diversions-old"

    rm -f "${db}/info/deepin-immutable-boot".* "${db}/info/deepin-immutable-ctl".*
    sed -i '/deepin-immutable-ctl/d;/deepin-immutable-boot/d' \
        "${db}/triggers/File" \
        "${db}/info/deepin-iso-config.conffiles" \
        "${db}/info/deepin-iso-config.list" \
        "${db}/info/deepin-iso-config.md5sums" 2>/dev/null || true
}

fix_no_immutable_root_label() {
    is_no_immutable_install || return 0

    local mount_points root_device fstype
    mount_points=$(installer_get "DI_MOUNTS_POINTS")
    mount_points=(${mount_points//;/ })
    for p in "${mount_points[@]}"; do
        kv=(${p//=/ })
        if [[ "${kv[1]}" == "/" ]]; then
            root_device="${kv[0]}"
            break
        fi
    done

    [[ -n "${root_device}" ]] || return 0
    fstype=$(lsblk -no FSTYPE "${root_device}" | head -n 1)
    case "${fstype}" in
        ext2|ext3|ext4)
            e2label "${root_device}" Root || warning "set ${root_device} label to Root failed"
            ;;
        btrfs)
            btrfs filesystem label "${root_device}" Root || warning "set ${root_device} label to Root failed"
            ;;
        xfs)
            xfs_admin -L Root "${root_device}" || warning "set ${root_device} label to Root failed"
            ;;
    esac
}

cleanup_no_immutable_target() {
    is_no_immutable_install || return 0

    info "cleanup immutable runtime from non-immutable target"

    restore_no_immutable_real_file /target "usr/bin/apt"
    restore_no_immutable_real_file /target "usr/bin/apt-get"
    restore_no_immutable_real_file /target "usr/bin/dpkg"
    restore_no_immutable_real_file /target "usr/sbin/update-initramfs"
    if [[ ! -e /target/etc/grub.d/10_linux && -e /target/etc/deepin-immutable-ctl/grub_d_10_linux ]]; then
        mv /target/etc/deepin-immutable-ctl/grub_d_10_linux /target/etc/grub.d/10_linux
        chmod 0755 /target/etc/grub.d/10_linux
    fi

    rm -rf \
        /target/etc/deepin-immutable-ctl \
        /target/etc/grub.d/15_immutable \
        /target/etc/grub.d/15_ostree \
        /target/etc/immutable.conf \
        /target/etc/logrotate.d/deepin-immutable-ctl \
        /target/etc/ostree \
        /target/etc/systemd/system/timers.target.wants/deepin-immutable-cleanup.timer \
        /target/ostree \
        /target/persistent \
        /target/usr/bin/deepin-immutable-grub-generate \
        /target/usr/bin/deepin-immutable-mount-root \
        /target/usr/bin/deepin-immutable-writable \
        /target/usr/bin/deepin-mount-root.sh \
        /target/usr/libexec/deepin-immutable-ctl \
        /target/usr/sbin/deepin-immutable-ctl \
        /target/usr/share/bash-completion/completions/deepin-immutable-ctl \
        /target/usr/share/deepin-immutable-ctl \
        /target/usr/share/doc/deepin-immutable-boot \
        /target/usr/share/doc/deepin-immutable-ctl \
        /target/usr/share/initramfs-tools/hooks/immutable \
        /target/usr/share/initramfs-tools/scripts/init-bottom/immutable \
        /target/usr/share/initramfs-tools/scripts/ostree \
        /target/usr/share/locale/zh_CN/LC_MESSAGES/deepin-immutable-ctl.mo \
        /target/var/lib/deepin-rollback-hooks \
        /target/var/lib/systemd/deb-systemd-helper-enabled/deepin-immutable-cleanup.timer.dsh-also \
        /target/var/lib/systemd/deb-systemd-helper-enabled/timers.target.wants/deepin-immutable-cleanup.timer
    rm -f \
        /target/usr/lib/systemd/system/deepin-immutable-cleanup.service \
        /target/usr/lib/systemd/system/deepin-immutable-cleanup.timer \
        /target/lib/systemd/system/deepin-immutable-cleanup.service \
        /target/lib/systemd/system/deepin-immutable-cleanup.timer

    cleanup_no_immutable_dpkg_db /target/usr/lib/dpkg/var
    cleanup_no_immutable_dpkg_db /target/var/lib/dpkg

    mkdir -p /target/usr/lib/dpkg/var /target/var/lib
    if [[ ! -e /target/var/lib/dpkg ]]; then
        ln -s ../../usr/lib/dpkg/var /target/var/lib/dpkg
    fi
}
