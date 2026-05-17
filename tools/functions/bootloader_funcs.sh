#!/bin/bash

init_bootloader_type() {
   local is_efi=$(installer_get DI_BOOTLOADER_IS_EFI)
   if [ -n "$is_efi" ]; then
        info "Use the user - configured boot mode <$is_efi>"
   else
        installer_set DI_BOOTLOADER_IS_EFI false
        # 申威平台默认支持efi，但固件又不支持用/sys/firmare/efi判断
        if is_sw || [ -d "/sys/firmware/efi" ]; then
            installer_set DI_BOOTLOADER_IS_EFI true
        elif is_mips && [ -d "/sys/firmware/lefi" ]; then
            installer_set DI_BOOTLOADER_IS_EFI true
        fi
   fi
}

get_bootloader_partition() {
    local dl_device=""
    local mp_list=("/boot/efi" "/boot" "/sysroot" "/")
    for input in ${mp_list[@]}; do
        local dl_device=$(lsblk -lnfp -o NAME,MOUNTPOINT | grep -E "$input$" | awk '{print $1}')
        if [ -n "$dl_device" ]; then
            echo -n "$dl_device"
            return
        fi
    done

    echo -n ""
}

is_uefi() {
    local is_efi=$(installer_get DI_BOOTLOADER_IS_EFI)
    if [ "x$is_efi" = "xtrue" ]; then
        return 0
    else
        return 1
    fi
}

is_ghost() {
    local ghost_mode=$(installer_get DI_IS_GHOST_MODE)
    if [ "x$ghost_mode" = "xtrue" ]; then
        return 0
    else
        return 1
    fi
}

install_esp_package() {
    if is_ghost; then
        info "ghost not need install esp package"
        return 0
    fi

    if is_x86; then
        if is_uefi; then
            if [ x$(cat /sys/firmware/efi/fw_platform_size 2>/dev/null) = 'x32' ]; then
                install_package grub-efi-ia32
            else
                install_package shim-signed grub-efi-amd64-signed efibootmgr
            fi
        else
             install_package grub-pc
        fi
    fi

    if is_arm64 && is_uefi; then
        if [ x$(cat /sys/firmware/efi/fw_platform_size 2>/dev/null) = 'x32' ]; then
            install_package grub-efi-arm
        else
            install_package efibootmgr grub-efi-arm64 grub-efi-arm64-bin
        fi
    fi

    if is_loongarch64; then
        install_package bash
    fi

    if is_sw && is_uefi; then
        install_package grub-efi-sw64 efibootmgr
    fi

}

add_start_option() {
    local arch_info=$@
    local bootloader_id=$(installer_get "DI_BOOTLOADER_OPTION_CONFIG")
    echo "bootloader_id=${bootloader_id}"
    if [ -z "${bootloader_id}" ]; then
        bootloader_id="uos"
    fi
    ## 基础启动项,默认UOS
    ## --force-extra-removable 选项确保了 /boot/efi/EFI/BOOT 目录的创建，并同时满足的申威的固件要求
    grub-install $arch_info --efi-directory=/boot/efi --bootloader-id="${bootloader_id}" --force-extra-removable --recheck \
        || error "grub-install failed with $arch_info" "${bootloader_id}"

    # Copy signed grub efi file. (sw_64 does not want this directry)
    if is_x86 || is_arm64; then
      # x86和arm使用 /boot/efi/EFI/boot， sw和loongarch64使用/boot/efi/EFI/BOOT
      rm -rf /boot/efi/EFI/BOOT
      mkdir -p /boot/efi/EFI/boot
      cp -vf /boot/efi/EFI/${bootloader_id}/grub* /boot/efi/EFI/boot/
    fi

    # ghost安装器兼容103x之前的系统，103x之前的系统由于安全启动的原因使用的是ubuntu目录作为引导目录
    [ -d /boot/efi/EFI/ubuntu ] && cp -vf /boot/efi/EFI/${bootloader_id}/grub* /boot/efi/EFI/ubuntu

    if is_x86; then
        # 32bit机型默认的efi引导文件
        fallback_efi=/boot/efi/EFI/boot/bootia32.efi
        fallback_efi_bak="${fallback_efi}-$(date +%s).bak"
        [ -f "${fallback_efi}" ] && cp "${fallback_efi}" "${fallback_efi_bak}"
        # Override fallback efi with shim.
        cp -vf /boot/efi/EFI/${bootloader_id}/shim*.efi "${fallback_efi}"

        # x86的64bit机型默认的efi引导文件
        fallback_efi=/boot/efi/EFI/boot/bootx64.efi
        fallback_efi_bak="${fallback_efi}-$(date +%s).bak"
        [ -f "${fallback_efi}" ] && cp "${fallback_efi}" "${fallback_efi_bak}"
        # Override fallback efi with shim.
        cp -vf /boot/efi/EFI/${bootloader_id}/shim*.efi "${fallback_efi}"
    fi

    # arm64的64bit机型默认的efi引导文件
    if is_arm64; then
        fallback_efi=/boot/efi/EFI/boot/bootaa64.efi
        fallback_efi_bak="${fallback_efi}-$(date +%s).bak"
        [ -f "${fallback_efi}" ] && cp "${fallback_efi}" "${fallback_efi_bak}"
        # Override fallback efi with shim.
        if ls /boot/efi/EFI/${bootloader_id}/shim* 1>/dev/null 2>&1; then
            cp -vf /boot/efi/EFI/${bootloader_id}/shim*.efi "${fallback_efi}"
        else
            cp -vf /boot/efi/EFI/${bootloader_id}/grubaa64.efi "${fallback_efi}"
        fi
    fi
}

# update grub config by locale
update_grub_local() {
    info "update grub..."
    update_local
    info "update local"
    [ -x /usr/sbin/update-grub ] && /usr/sbin/update-grub
    info "update grub end!"
}

install_x86_legacy() {
    local bootloader_part=$(get_bootloader_partition)
    local bootloader_dev=$(part_to_device ${bootloader_part})
    info "detected legacy machine, installing grub to ${bootloader_dev}"

    # bootloader_dev 要安装 GRUB/dev/sdX的磁盘, 不是分区
    grub-install --no-floppy ${bootloader_dev} --target=i386-pc --force 2>/dev/null \
        || error "grub-install --no-floppy ${bootloader_dev} --target=i386-pc --force failed!"
}

intsall_x86_uefi() {
    if [ x$(cat /sys/firmware/efi/fw_platform_size 2>/dev/null) = 'x32' ]; then
        add_start_option --target=i386-efi --efi-directory=/boot/efi
    else
        mount -t efivarfs efivarfs /sys/firmware/efi/efivars
        rm -vfr /sys/firmware/efi/efivars/dump-*

        # uefi-secure-boot options is enabled by default
        add_start_option --target=x86_64-efi --uefi-secure-boot
        umount /sys/firmware/efi/efivars

    fi
}

install_bootloader_x86() {
    info "install bootloader x86..."
    is_uefi && intsall_x86_uefi
    is_uefi || install_x86_legacy
    info "install bootloader x86 end"
}

install_bootloader_arm64() {
    if [ x$(cat /sys/firmware/efi/fw_platform_size 2>/dev/null) = 'x32' ]; then
        add_start_option --target=arm-efi  --recheck # 增加系统启动选项
    else
        add_start_option --target=arm64-efi --uefi-secure-boot  --recheck # 增加系统启动选项
    fi
}

install_bootloader_sw() {
    if [ "x$(installer_get "DI_BOOTLOADER_IS_EFI")" != "xtrue" ]; then
        error "sw just support uefi"
    fi

    add_start_option --target=sw64-efi  --recheck # 增加系统启动选项
    echo "# avoid kernel update fails with /boot on FAT32
do_symlinks = no" | tee /etc/kernel-img.conf
}

install_bootloader_loongarch64() {
    add_start_option --target=loongarch64-efi
}

install_bootloader_mips() {
    info "install bootloader mips..."
    local bootloader_id=$(installer_get "DI_BOOTLOADER_OPTION_CONFIG")
    # mips架构grub-install会执行失败，但是不影响启动。 这里暂时忽略掉错误
    grub-install --target=mips64el-efi --efi-directory=/boot/efi --bootloader-id="${bootloader_id}" --force-extra-removable --recheck \
        || warning "grub-install --target=mips64el-efi failed"
}

install_riscv64_uefi() {

       mount -t efivarfs efivarfs /sys/firmware/efi/efivars
       rm -vfr "/sys/firmware/efi/efivars/dump-*"

       add_start_option --target=riscv64-efi
       umount /sys/firmware/efi/efivars
}

install_bootloader_riscv64() {
    info "install bootloader riscv64..."
    install_riscv64_uefi
    info "install bootloader riscv64 end"
}

install_bootloader() {
    info "install bootloader..."
    if is_x86; then
        install_bootloader_x86
    fi

    if is_arm64 && is_uefi; then
        install_bootloader_arm64
    fi

    if is_loongarch64; then
	    install_bootloader_loongarch64
    fi

    if is_sw && is_uefi; then
        install_bootloader_sw
    fi

    if is_riscv64 && is_uefi; then
        install_bootloader_riscv64
    fi

    # if is_mips; then
    #     install_bootloader_mips
    # fi
    info "install bootloader end."
}

setup_bootloader_x86() {
    local GRUB_TIMEOUT=$(installer_get "DI_GRUB_TIMEOUT_CONFIG")
    local GRUB_RESOLUTION=$(installer_get "DI_GRUB_RESOLUTION_CONFIG")
    local LOCALES=$(installer_get "DI_LOCALE")
    if [ -n  "$LOCALES" ]; then
        LOCALES="${LOCALES}.UTF-8"
    else
        LOCALES="zh_CN.UTF-8"
    fi

    local NOMODESET=""
    if [[ $(cat /proc/cmdline) =~ \ nomodeset(\ |$) ]]; then
        NOMODESET="nomodeset"
    fi

    backgroundPicPath="/boot/grub/themes/deepin-fallback"
    if [ -d "${backgroundPicPath}" ];then
        info "use ${backgroundPicPath}"
    else
        warning "${backgroundPicPath} not found ! turn to /boot/grub/themes/deepin"
        backgroundPicPath="/boot/grub/themes/deepin"
    fi

    cat > /etc/default/grub << EOF
# Generated by deepin-installer
GRUB_BACKGROUND="${backgroundPicPath}/background.jpg"
GRUB_CMDLINE_LINUX_DEFAULT="video=efifb:nobgrt splash quiet loglevel=0 $NOMODESET locales=$LOCALES"
GRUB_DEFAULT=0
GRUB_DISTRIBUTOR="\`/usr/bin/lsb_release -d -s 2>/dev/null\`"
GRUB_THEME="${backgroundPicPath}/theme.txt"
GRUB_TIMEOUT=${GRUB_TIMEOUT}
GRUB_GFXMODE=${GRUB_RESOLUTION}
DEEPIN_GFXMODE_DETECT=1
EOF
}

setup_bootloader_arm64() {
    local GRUB_TIMEOUT=$(installer_get "DI_GRUB_TIMEOUT_CONFIG")
    local GRUB_RESOLUTION=$(installer_get "DI_GRUB_RESOLUTION_CONFIG")
    local LOCALES=$(installer_get "DI_LOCALE")
    if [ -n  "$LOCALES" ]; then
        LOCALES="${LOCALES}.UTF-8"
    else
        LOCALES="zh_CN.UTF-8"
    fi

    backgroundPicPath="/boot/grub/themes/deepin-fallback"
    if [ -d "${backgroundPicPath}" ];then
        info "use ${backgroundPicPath}"
    else
        warning "${backgroundPicPath} not found ! turn to /boot/grub/themes/deepin"
        backgroundPicPath="/boot/grub/themes/deepin"
    fi

    cat > /etc/default/grub << EOF
# Generated by deepin-installer
GRUB_BACKGROUND="${backgroundPicPath}/background.jpg"
GRUB_CMDLINE_LINUX_DEFAULT="video=efifb:nobgrt splash quiet console=tty loglevel=0 plymouth.ignore-serial-consoles locales=$LOCALES"
GRUB_DEFAULT=0
GRUB_DISTRIBUTOR="\`/usr/bin/lsb_release -d -s 2>/dev/null\`"
GRUB_THEME="${backgroundPicPath}/theme.txt"
GRUB_TIMEOUT=${GRUB_TIMEOUT}
GRUB_GFXMODE=${GRUB_RESOLUTION}
DEEPIN_GFXMODE_DETECT=1
EOF
}

setup_bootloader_mips() {
    local GRUB_TIMEOUT=$(installer_get "DI_GRUB_TIMEOUT_CONFIG")
    local GRUB_RESOLUTION=$(installer_get "DI_GRUB_RESOLUTION_CONFIG")
    local LOCALES=$(installer_get "DI_LOCALE")
    if [ -n  "$LOCALES" ]; then
        LOCALES="${LOCALES}.UTF-8"
    else
        LOCALES="zh_CN.UTF-8"
    fi

    backgroundPicPath="/boot/grub/themes/deepin-fallback"
    if [ -d "${backgroundPicPath}" ];then
        info "use ${backgroundPicPath}"
    else
        warning "${backgroundPicPath} not found ! turn to /boot/grub/themes/deepin"
        backgroundPicPath="/boot/grub/themes/deepin"
    fi

    cat > /etc/default/grub << EOF
# Generated by deepin-installer
GRUB_BACKGROUND="${backgroundPicPath}/background.jpg"
GRUB_CMDLINE_LINUX_DEFAULT="video=efifb:nobgrt splash quiet console=tty loglevel=0 locales=$LOCALES"
GRUB_DEFAULT=0
GRUB_DISTRIBUTOR="\`/usr/bin/lsb_release -d -s 2>/dev/null\`"
GRUB_THEME="${backgroundPicPath}/theme.txt"
GRUB_TIMEOUT=${GRUB_TIMEOUT}
GRUB_GFXMODE=${GRUB_RESOLUTION}
EOF
}

setup_bootloader_loongarch64() {
    info "setup bootloader loongarch64..."
    setup_bootloader_mips
}

setup_bootloader_riscv64() {
    info "setup bootloader riscv64..."
    setup_bootloader_mips
}

setup_bootloader_sw() {
    info "setup bootloader sw..."
    local GRUB_TIMEOUT=$(installer_get "DI_GRUB_TIMEOUT_CONFIG")
    local GRUB_RESOLUTION=$(installer_get "DI_GRUB_RESOLUTION_CONFIG")
    local LOCALES=$(installer_get "DI_LOCALE")
    if [ -n  "$LOCALES" ]; then
        LOCALES="${LOCALES}.UTF-8"
    else
        LOCALES="zh_CN.UTF-8"
    fi

    backgroundPicPath="/boot/grub/themes/deepin-fallback"
    if [ -d "${backgroundPicPath}" ];then
        info "use ${backgroundPicPath}"
    else
        warning "${backgroundPicPath} not found ! turn to /boot/grub/themes/deepin"
        backgroundPicPath="/boot/grub/themes/deepin"
    fi

cat > /etc/default/grub << EOF
# Generated by deepin-installer
GRUB_BACKGROUND="${backgroundPicPath}/background.jpg"
GRUB_CMDLINE_LINUX_DEFAULT="video=efifb:nobgrt splash quiet loglevel=0 locales=$LOCALES"
GRUB_DEFAULT=0
GRUB_DISTRIBUTOR="\`/usr/bin/lsb_release -d -s 2>/dev/null\`"
GRUB_THEME="${backgroundPicPath}/theme.txt"
GRUB_TIMEOUT=${GRUB_TIMEOUT}
GRUB_GFXMODE=${GRUB_RESOLUTION}
DEEPIN_GFXMODE_DETECT=1
EOF
}

setup_grub_theme() {
    export XAUTHORITY=/run/lightdm/root/${DISPLAY}
    LC_ALL="" LANGUAGE=$(installer_get "DI_LOCALE") LANG=$(installer_get "DI_LOCALE").UTF-8 /usr/lib/deepin-daemon/grub2 -prepare-gfxmode-detect
    if [ -f  /etc/default/grub.d/11_dde.cfg ]; then
        rm -f  /etc/default/grub.d/11_dde.cfg
    fi
    /usr/lib/deepin-daemon/grub2 -prepare-gfxmode-detect
}

setup_pcie_compat() {
    local machineType=$(echo -n $(cat /sys/devices/virtual/dmi/id/product_name))
    local kernel=`uname -r`
    if [ "${machineType}" = "81VQ" ] && [[ "${kernel}" =~ "5.10.0-" ]];then
        cat > /etc/default/grub.d/01_setup_pcie_ports_compat.cfg << EOF
# Generated by deepin-installer
GRUB_CMDLINE_LINUX_DEFAULT="\$GRUB_CMDLINE_LINUX_DEFAULT pcie_ports=compat"
EOF
    fi
}

setup_bootloader() {
    info "setup bootloader..."
    if is_x86; then
        setup_bootloader_x86
    fi

    if is_arm64; then
        setup_bootloader_arm64
    fi

    if is_mips; then
        setup_bootloader_mips
    fi

    if is_loongarch64; then
        setup_bootloader_loongarch64
    fi

    if is_riscv64; then

        setup_bootloader_riscv64

    fi

    if is_sw; then
        setup_bootloader_sw
    fi

    # 针对设置了tpm的机器，增加内核启动参数
    # rng_core.default_quality=1000
    if is_tpm; then
        cat > /etc/default/grub.d/01_setup_rng_core.cfg << EOF
# Generated by deepin-installer
GRUB_CMDLINE_LINUX_DEFAULT="\$GRUB_CMDLINE_LINUX_DEFAULT rng_core.default_quality=1000"
EOF
    fi

    setup_pcie_compat

    info "setup bootloader end."
}
