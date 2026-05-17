#!/bin/bash

installer_get() {
    local key="$1"
    local sesion="$2"
    [ -f "${DI_CONFIG_FILE_ENV}" ] || error "$DI_CONFIG_FILE_ENV is not defined"
    local config_bin=/usr/bin/deepin-installer-config
    if [ ! -f "${config_bin}" ]; then
        config_bin=/var/lib/deepin-installer/bin/deepin-installer-config
    fi
    $config_bin get "${DI_CONFIG_FILE_ENV}" "${sesion}" "${key}" 2>/dev/null
}

# Set value in conf file. Section name is ignored.
installer_set() {
    local key="$1"
    local value="$2"
    local sesion="$3"
    if [ $key = "DI_INSTALLER_STATUS" ]; then
        echo "DI_INSTALLER_STATUS=${value}"
    fi

    local config_bin=/usr/bin/deepin-installer-config
    if [ ! -f "${config_bin}" ]; then
        config_bin=/var/lib/deepin-installer/bin/deepin-installer-config
    fi

    $config_bin set "${DI_CONFIG_FILE_ENV}" "${sesion}" "${key}" "${value}" 2>/dev/null
}


init_config() {
    local CONFIG_FILE=$1
    # 初始化live系统下安装器配置
    mkdir -p $(get_file_path $CONFIG_FILE)
    deepin-installer-config init $CONFIG_FILE
    # 初始化引导的类型的配置
    init_bootloader_type
    # 初始化tpm检测的配置
    init_tpm_check
    # 检查是否存在NVIDIA显卡
    init_nvidia_check

    # 初始化live config的配置到安装器的配置中
    local liveConfigItems=$(cat /etc/live/config.conf.d/deepin-installer-live.conf | tr '\n' ';')
    while [ -n "${liveConfigItems}" ]; do
        local item=${liveConfigItems%%;*}
        local itemName=${item%%=*}
        sed -i "/${itemName}/d" "${CONFIG_FILE}"
        sed -i "1a ${item}" "${CONFIG_FILE}"
        liveConfigItems=${liveConfigItems#*;}
    done

    # 从command line 读取 DI_EDITION_NAME 写到配置文件中
    reade_edition_name_from_cmdline
}

umount_disk() {
    local array=($(lsblk -lpno NAME,MOUNTPOINT |grep -v /usr/lib/live/mount | awk '{print $1}'))
    for element in "${array[@]}"
	  do
        umount ${element} || echo "umount ${element} failed"
    done
}

dconfig_init() {
    if [ -f "/usr/bin/os-config.sh" ]; then
        os-config.sh remove || echo "dconfig os-config.sh remove failed"
        os-config.sh install || echo "dconfig os-config.sh install failed"
    else
        echo "dconfig not exist"
    fi
}

live_hooks() {
    local OEM_DIR_CONFIG=$DI_LIVE_DIR_ENV/oem
    is_empty_dir "$OEM_DIR_CONFIG/live_hooks" || cp -vfr $OEM_DIR_CONFIG/live_hooks $DI_INSTALL_TOOLS_DIR_ENV/live_hooks
    local work_dir="$DI_INSTALL_TOOLS_DIR_ENV/live_hooks/"
    if ! is_empty_dir $work_dir;then
        for job in $(find $work_dir -maxdepth 1 -name "*.job" | sort); do
            bash $DI_INSTALL_TOOLS_DIR_ENV/scripts/hook_manager.sh $job
        done
    fi
}

before_install() {
    local work_dir="$DI_INSTALL_TOOLS_DIR_ENV/hooks/before_install"
    if ! is_empty_dir $work_dir;then
        for job in $(find $work_dir -maxdepth 1 -name "*.job" | sort); do
            bash $DI_INSTALL_TOOLS_DIR_ENV/scripts/hook_manager.sh $job
        done
    fi
}

is_livecd_install() {
    # Reading kernel command line
    for _PARAMETER in $(cat /proc/cmdline); do
        case "${_PARAMETER}" in
            live-config.livecd-installer|livecd-installer)
          return 0
          ;;
        esac
    done
    return 1
}

is_livecd() {
    # Reading kernel command line
    for _PARAMETER in $(cat /proc/cmdline); do
        case "${_PARAMETER}" in
            boot=live)
          return 0
          ;;
        esac
    done
    return 1
}

get_file_path() {
    local key=$1
    echo ${key%/*}
}

is_empty_dir() {
    local dir=$1
    [ ! -d $dir ] && return 0

    local count=$(ls $dir | wc -w)
    if [ $count -ne 0 ];then
        return 1
    else
        return 0
    fi
}

init_lightdm() {
    local key=$1
    local workspace=$2

    local conf_file=$workspace/etc/lightdm/lightdm.conf
    local tmp_conf_file=$workspace/etc/lightdm/lightdm.conf.real
    if [ -f "${conf_file}" ]; then
      if [ ! -e "${tmp_conf_file}" ]; then
         install -v -m644 "${conf_file}" "${tmp_conf_file}"
      fi
    fi

    cat > "$workspace/etc/lightdm/lightdm.conf" <<EOF
[Seat:*]
display-setup-script=bash $DI_INSTALL_TOOLS_DIR_ENV/deepin-installer-init
greeter-session=lightdm-deepin-greeter
greeter-setup-script=bash $DI_INSTALL_TOOLS_DIR_ENV/deepin-installer-core $key
EOF
}

init_workspace() {
    ## 处理定制脚本
    local OEM_DIR_CONFIG=$DI_LIVE_DIR_ENV/oem
    is_empty_dir "$OEM_DIR_CONFIG/tools" || cp -vfr $OEM_DIR_CONFIG/tools/* $DI_INSTALL_TOOLS_DIR_ENV/
    is_empty_dir "$OEM_DIR_CONFIG/hooks" || cp -vfr $OEM_DIR_CONFIG/hooks/* $DI_INSTALL_TOOLS_DIR_ENV/hooks/
    is_empty_dir "$OEM_DIR_CONFIG/modules" || cp -vfr $OEM_DIR_CONFIG/modules/* $DI_INSTALL_MODULE_DIR_ENV/
    ## 处理审核模式定制脚本
    local OEM_CHECK_CONFIG=$DI_LIVE_DIR_ENV/oem/check_hooks
    is_empty_dir "$OEM_CHECK_CONFIG" || cp -vfr $OEM_CHECK_CONFIG/ $DI_INSTALL_TOOLS_DIR_ENV/
}

init_live_workspace() {
    ## 处理定制脚本
    local OEM_DIR_CONFIG=$DI_LIVE_DIR_ENV/oem
    is_empty_dir "$OEM_DIR_CONFIG/live_hooks" || cp -vfr $OEM_DIR_CONFIG/live_hooks $DI_INSTALL_TOOLS_DIR_ENV/live_hooks

    ## 设置DI_LC_HOOKS_LIST
    local HOOKS_DIRS=$DI_INSTALL_TOOLS_DIR_ENV/hooks/
    installer_set DI_LC_HOOKS_LIST "$HOOKS_DIRS/live_config"
}

init_hookslist() {
    ## 设置DI_HOOKS_LIST
    local INSTALL_MODE=$(installer_get DI_INSTALL_MODE)
    local HOOKS_DIRS=$DI_INSTALL_TOOLS_DIR_ENV/hooks/
    local HOOKS_STEP="$HOOKS_DIRS/before_chroot;$HOOKS_DIRS/in_chroot;$HOOKS_DIRS/after_chroot;$HOOKS_DIRS/ostree"
    if [ "x$INSTALL_MODE" = "xno-first-boot" ] \
        || [ "x$INSTALL_MODE" = "xauto-no-first-boot" ];then
        installer_set DI_HOOKS_LIST "$HOOKS_STEP;$HOOKS_DIRS/user_config;$HOOKS_DIRS/cleanup"
    else
        installer_set DI_HOOKS_LIST "$HOOKS_STEP;$HOOKS_DIRS/cleanup"
    fi

    ## 设置DI_FB_HOOHS_LIST
    installer_set DI_FB_HOOHS_LIST "$HOOKS_DIRS/user_config;$HOOKS_DIRS/first_boot_cleanup"
}

copy_ghost_files() {
    local uimg_file=$(installer_get DI_UIMG_FILE)
    info "ghost mode install, uimg file:$uimg_file"
    if mount | grep /uimg; then
        echo "/uimg mounted!"
    else
        mount -o loop "$uimg_file" /uimg || error "mount uimg failed!"
    fi
    mkdir -pv /target/ostree/repo
    rsync_file -aHAX  --delete /uimg/localhost/sysroot/ostree/repo/ /target/ostree/repo
}

unsquashfs_filesystem() {
    local is_ghost=$(installer_get DI_IS_GHOST_MODE)
    if [ "x$is_ghost" = "xtrue" ]; then
        local uimg_file=$(installer_get DI_UIMG_FILE)
        echo "ghost mode install, uimg file:$uimg_file"
        if mount |grep /uimg; then
            echo "/uimg mounted!"
        else
            mount -o loop "$uimg_file" /uimg || error "mount uimg failed!"
        fi
        copy_ghost_files
    else
        local LIVE_DIR="$DI_LIVE_DIR_ENV/live"
        local BASE_MODULE="${LIVE_DIR}/filesystem.squashfs"
        unsquashfs_file $BASE_MODULE
        if [[ ${DI_OS_VERSION_ENV} == "V25" ]];then
            unsquashfs_file "${LIVE_DIR}/filesystem-extra.squashfs"
        fi

        # 可变安装删除ostree仓库
        [ -d /target/ostree ] && rm -fr /target/ostree
        [ -d /target/sysroot-checkout-rw ] && rm -fr /target/sysroot-checkout-rw
        [ -d /target/persistent/ostree ] && rm -fr /target/persistent/ostree
    fi
}

unsquashfs_file()
{
    local BASE_MODULE=$1
    local TARGET=$2
    local OBJECT_DIR=$3
    info "unsquashfs filesystem: $BASE_MODULE"
    local CORES=${CORES:-$(lscpu | sed -e 's/^[ ]*//g' | grep "^CPU(s):" | awk '{print $2}')}
    if [ $CORES -gt 1 ];then
        #解压文件系统会导致轮播图动画卡住，减一个核用于动画刷新
        CORES=`expr $CORES - 1`
    fi

    local UNSQUASHFS_LOG="/var/log/deepin-installer/unsquashfs.log"
    local XZ_FAILED_LOG="/var/log/deepin-installer/xzfailed.log"
    local UNSQUASHFS_EXTRA_PARAMS=$(installer_get "DI_UNSQUASHFS_EXTRA_PARAMS")
    unsquashfs -d $TARGET $UNSQUASHFS_EXTRA_PARAMS -f -p $CORES "${BASE_MODULE}" $OBJECT_DIR > "${UNSQUASHFS_LOG}" 2>&1 || error "$(md5sum ${BASE_MODULE})"
    local UNSQUASHFS_RET
    grep "Failed to write.*skipping" "${UNSQUASHFS_LOG}" | grep -oP "(?<=\/target)(.*)(?=,)" > "${XZ_FAILED_LOG}" 2>&1 || UNSQUASHFS_RET="true"

    if [ "x$UNSQUASHFS_RET" != "xtrue" ]; then
        local RET
        xargs -a "${XZ_FAILED_LOG}" -I % unsquashfs -d $TARGET $UNSQUASHFS_EXTRA_PARAMS -f "${BASE_MODULE}" $OBJECT_DIR % 2>&1 | grep "uncompress failed" || RET="true"
        if [ "x$RET" != "xtrue" ]; then
            error "unsquashfs failed: $(md5sum "${BASE_MODULE}")!"
        else
            echo "unsquashfs success with retry!"
        fi
    else
        echo "unsquashfs success"
    fi

    sync
    #echo 3 > /proc/sys/vm/drop_caches
}

setup_mountpoint() {
    mkdir -p /target/dev
    mount --bind -v --bind /dev/ /target/dev || error "Failed to bind /dev"

    mkdir -p /target/dev/pts
    mount -vt devpts devpts /target/dev/pts || error "Failed to bind devpts"

    mkdir -p /target/proc
    mount -vt proc proc /target/proc || error "Failed to bind proc"

    mkdir -p /target/sys
    mount -vt sysfs sysfs /target/sys || error "Failed to bind sysfs"

    if is_ostree_system;then
        # mount /run
        mkdir -p /target/run
        mount --bind /run /target/run || error "Failed to bind /run"
        #mount -t tmpfs -o size=80%,mode=755 tmpfs /target/run
    else
        # Mount /var/run
        mkdir -p /target/var/run
        mount -v --bind /var/run /target/var/run || error "Failed to bind /var/run"
     
    fi

    # Mount /tmp/.X11-unix/
    mkdir -p /target/tmp/.X11-unix
    mount -v --bind /tmp/.X11-unix/ /target/tmp/.X11-unix || error "Failed to bind /tmp/.X11-unix"

    mkdir -p /target/$DI_LIVE_DIR_ENV
    mount -v --bind ${DI_LIVE_DIR_ENV} /target/$DI_LIVE_DIR_ENV || error "Failed to bind ${DI_LIVE_DIR_ENV}"
    mount -v --bind ${DI_LIVE_DIR_ENV}/oem /target/$DI_LIVE_DIR_ENV/oem || error "Failed to bind ${DI_LIVE_DIR_ENV}/oem"
}

# Check whether current platform is loongson or not.
is_mips() {
  case $(uname -m) in
    loongson | mips*)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

is_loongarch64() {
  case $(uname -m) in
    loongarch64)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

is_riscv64() {
  case $(uname -m) in
    riscv64)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}
 
# Check whether current platform is sw or not.
is_sw() {
  case $(uname -m) in
    sw*)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

# Check whether current platform is x86/x86_64 or not.
is_x86() {
  case $(uname -m) in
    i386 | i686 | amd64 | x86 | x86_64)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

# Check whether current platform is arm64 or not.
is_arm64() {
  case $(uname -m) in
    arm64 | aarch64)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

update_local() {
    local DI_LOCALE=$(installer_get "DI_LOCALE")
    DI_LOCALE=${DI_LOCALE:-en_US}

    export LANGUAGE=${DI_LOCALE}
    export LANG=${DI_LOCALE}.UTF-8
    export LC_ALL=
}

is_first_boot() {
    # 获取审核模式的账户
    local cm_user=$(installer_get DI_CM_USER)

    # 判断/home下的目录是用户
    for name in $(ls /home --ignore=$cm_user); do
        id $name
        if [ $? = 0 ]; then
            return 1
        fi
    done

    return 0
}

is_check_mode() {
    # 获取审核模式的账户
    local is_cm=$(installer_get DI_IS_CHECK_MODE)

    # 判断/home下的目录是用户
    if [ "x$is_cm" = "xtrue" ]; then
        return 0
    fi

    return 1
}

# 安装模式：默认安装（有后配置界面=default
# 无人值守安装=auto-install;
# 无first-boot安装=no-first-boot
# 产线无first-boot安装=auto-no-first-boot;
is_have_first_boot() {
    local install_mode=$(installer_get "DI_INSTALL_MODE")
    if [[ $install_mode =~ "no-first-boot" ]]; then
        return 1
    else
        return 0
    fi
}

setup_first_boot_service() {
    info "systemctl disable deepin-installer"
    systemctl disable deepin-installer
    if is_have_first_boot; then
        info "systemctl enable deepin-installer-first-boot"
        systemctl enable deepin-installer-first-boot
        info "systemctl disable deepin-installer-extra"
        systemctl disable deepin-installer-extra
    else
        info "systemctl disable deepin-installer-first-boot"
        systemctl disable deepin-installer-first-boot
        info "systemctl enable deepin-installer-extra"
        systemctl enable deepin-installer-extra
    fi
}

disable_first_boot_service() {
    systemctl disable deepin-installer-first-boot
}

is_service() {
    local type=$(cat /etc/deepin-version | grep Type=)
    type="${type##Type=}"
    if [ "x$type" = "xServer" ]; then
        return 0
    else
        return 1
    fi
}

is_community() {
    local type=$(cat /etc/deepin-version | grep Type=)
    type="${type##Type=}"
    if [ "x$type" = "xDesktop" ]; then
        return 0
    else
        return 1
    fi
}

setup_chroot_hooks() {
    ## 处理chroot环境的hooks执行环境
    if [ ! -d /target/$DI_INSTALL_TOOLS_DIR_ENV ]; then
        mkdir -p /target/$DI_INSTALL_TOOLS_DIR_ENV
    fi

    #if [ ! -d /target/$DI_INSTALL_MODULE_DIR_ENV ]; then
    #    mkdir -p /target/$DI_INSTALL_MODULE_DIR_ENV
    #fi

    #if [ ! -f /target/$DI_INSTALL_DIR_ENV/bin/deepin-installer-config ]; then
    #    cp -vfr $DI_INSTALL_DIR_ENV/bin/deepin-installer-config /target/$DI_INSTALL_DIR_ENV/bin
    #fi

    rsync -aHAXi --delete $DI_INSTALL_TOOLS_DIR_ENV/ /target/$DI_INSTALL_TOOLS_DIR_ENV
    mkdir -p /target/var/lib/deepin-installer/bin || error "mkdir -p /target/var/lib/deepin-installer/bin failed!"
    cp -vfr $DI_HOME_BIN_ENV/deepin-installer-config /target/var/lib/deepin-installer/bin

    #cp -vfr $DI_INSTALL_MODULE_DIR_ENV/* /target/$DI_INSTALL_MODULE_DIR_ENV

    local CONF_DIR=$(get_file_path $DI_CONFIG_FILE_ENV)
    info "CONF_DIR=$CONF_DIR"
    mkdir -vp /target/$CONF_DIR
    ls $CONF_DIR | xargs -I {} cp -vfr $CONF_DIR/{} /target/$CONF_DIR
}

rsync_file() {
    local result=0
    info "rsync $@"
    rsync $@ || result=$?
    if [ "x$result" = "x0" ]; then
        info "rsync finished"
    elif [ "x$result" = "x23" ] || [ "x$result" = "x24" ]; then
        warning "rsync ignore error"
    else
        error "rsync files error"
    fi
}

copy_oem_first_boot() {
    # 将后配置安装的定制包拷贝到目标系统
    local OEM_DIR=$DI_LIVE_DIR_ENV/oem
    if [ -d $OEM_DIR/first_boot_deb ]; then
        cp -vfr $OEM_DIR/first_boot_deb /target/$DI_INSTALL_DIR_ENV
    fi
}

setup_ostree() {
    local data_mount_points=$(installer_get "DI_DATA_MOUNT_POINT_CONFIG" "${DI_OS_VERSION_ENV}")
    local is_init_recovery=$(installer_get "DI_IS_INIT_RECOVERY")
    local curDate=`date +%s`
    local ostreeSubject="--subject={\"SubmissionTime\":\"$curDate\",\"SubmissionType\":2}"
    local ostreeConfig=/usr/share/deepin-installer/configs/ostree_config.yaml

    if [ "x${is_init_recovery}" = "xtrue" ]; then
        # 设置/target中的安装模式为default模式
        deepin-installer-config set /target"${DI_CONFIG_FILE_ENV}" "DI_INSTALL_MODE" "default"

        # 启用后配置服务
        local is_firstboot_service_enabled=$(chroot /target systemctl is-enabled deepin-installer-first-boot)
        info "deepin-installer-first-boot service status in target is : ${is_firstboot_service_enabled}"
        if [ x"${is_firstboot_service_enabled}" = xdisabled ]; then
            chroot /target systemctl enable deepin-installer-first-boot
            info "set deepin-installer-first-boot service status $(chroot /target systemctl is-enabled deepin-installer-first-boot)"
        fi

        # 禁用extra服务置服务
        local is_extra_service_enabled=$(chroot /target systemctl is-enabled deepin-installer-extra)
        info "deepin-installer-extra service status in target is : ${is_extra_service_enabled}"
        if [ x"${is_extra_service_enabled}" = xenabled ]; then
            chroot /target systemctl disable deepin-installer-extra
            info "set deepin-installer-extra service status $(chroot /target systemctl is-enabled deepin-installer-extra)"
        fi

        ##创建原子更新配置文件
        cat > /target/${ostreeConfig} << EOF
target:
    backup_list:
        - "/"
    hold_list:
        - "${data_mount_points}"
EOF

        if [ -n "$data_mount_points" ] && [ -d "/target/${data_mount_points}" ]; then
            info "setup ostree ..."
            if [ -f "/target/$ostreeConfig" ]; then
                chroot /target deepin-upgrade-manager --action=setdefaultconfig --data=${ostreeConfig} || warning "deepin-upgrade-manager --action=setdefaultconfig --data=${ostreeConfig} is failed"
            else
                warning "${ostreeConfig} is not exist"
            fi
            chroot /target deepin-upgrade-manager --action=init $ostreeSubject || warning "deepin-upgrade-manager --action=init $ostreeSubject is failed !"
            # 设置ostree的配置为默认配置
            chroot /target deepin-upgrade-manager --action=setdefaultconfig || warning "deepin-upgrade-manager --action=setdefaultconfig failed !"
        fi

        # 设置/target中的安装模式为当前设置的模式
        local install_mode=$(installer_get "DI_INSTALL_MODE")
        deepin-installer-config set /target"${DI_CONFIG_FILE_ENV}" "DI_INSTALL_MODE" "${install_mode}"

        # 还原/target中的firstboot服务状态
        if [ x"${is_firstboot_service_enabled}" = xenabled ]; then
            chroot /target systemctl enable deepin-installer-first-boot
        else
            chroot /target systemctl disable deepin-installer-first-boot
        fi
        info "restore set deepin-installer-first-boot service status $(chroot /target systemctl is-enabled deepin-installer-first-boot)"

        # 还原/target中的extra服务状态
        if [ x"${is_extra_service_enabled}" = xenabled ]; then
            chroot /target systemctl enable deepin-installer-extra
        else
            chroot /target systemctl disable deepin-installer-extra
        fi
        info "restore set deepin-installer-extra service status $(chroot /target systemctl is-enabled deepin-installer-extra)"
    fi
}

setup_ostree_snapshot() {
    if ! is_ostree_system; then
        return
    fi
    info "setup ostree snapshot ..."

    # 设置/target中的安装模式为default模式
    deepin-installer-config set /target"${DI_CONFIG_FILE_ENV}" "DI_INSTALL_MODE" "default"

    # 设置/target中禁用审核模式
    deepin-installer-config set /target"${DI_CONFIG_FILE_ENV}" "DI_IS_CHECK_MODE" "false"

    # 启用后配置服务
    local is_firstboot_service_enabled=$(chroot /target systemctl is-enabled deepin-installer-first-boot)
    info "deepin-installer-first-boot service status in target is : ${is_firstboot_service_enabled}"
    if [ x"${is_firstboot_service_enabled}" = xdisabled ]; then
        chroot /target systemctl enable deepin-installer-first-boot
        info "set deepin-installer-first-boot service status $(chroot /target systemctl is-enabled deepin-installer-first-boot)"
    fi

    # 禁用extra服务置服务
    local is_extra_service_enabled=$(chroot /target systemctl is-enabled deepin-installer-extra)
    info "deepin-installer-extra service status in target is : ${is_extra_service_enabled}"
    if [ x"${is_extra_service_enabled}" = xenabled ]; then
        chroot /target systemctl disable deepin-installer-extra
        info "set deepin-installer-extra service status $(chroot /target systemctl is-enabled deepin-installer-extra)"
    fi

    # 创建ostree快照
    chroot /target deepin-immutable-ctl snapshot create "First backup" "System initialization backup" || error "Failed to create ostree snapshot"

     # 还原/target中的安装模式为当前设置的模式
    local install_mode=$(installer_get "DI_INSTALL_MODE")
    deepin-installer-config set /target"${DI_CONFIG_FILE_ENV}" "DI_INSTALL_MODE" "${install_mode}"

     # 设置/target中审核模式启用状态为当前设置
    local is_check_mode=$(installer_get "DI_IS_CHECK_MODE")
    deepin-installer-config set /target"${DI_CONFIG_FILE_ENV}" "DI_IS_CHECK_MODE" "${is_check_mode}"
    

    # 还原/target中的firstboot服务状态
    if [ x"${is_firstboot_service_enabled}" = xenabled ]; then
        chroot /target systemctl enable deepin-installer-first-boot
    else
        chroot /target systemctl disable deepin-installer-first-boot
    fi
    info "restore set deepin-installer-first-boot service status $(chroot /target systemctl is-enabled deepin-installer-first-boot)"

    # 还原/target中的extra服务状态
    if [ x"${is_extra_service_enabled}" = xenabled ]; then
        chroot /target systemctl enable deepin-installer-extra
    else
        chroot /target systemctl disable deepin-installer-extra
    fi
    info "restore set deepin-installer-extra service status $(chroot /target systemctl is-enabled deepin-installer-extra)"
}

# 使用deepin-clone生成备份文件
setup_deepin_clone_recovery() {
    ## 判断是否全盘安装模式
    if ! is_full_disk_mode; then
        info "setup_deepin_clone_recovery nothing to do"
        return 0
    fi

    # 查找data分区,(根据挂载点来查找)
    local DATA_MOUNT_POINT_CONFIG=$(installer_get "DI_DATA_MOUNT_POINT_CONFIG" "${DI_OS_VERSION_ENV}")
    local dataPartPath=$(lsblk -lp -o NAME,MOUNTPOINT | grep -w "/target${DATA_MOUNT_POINT_CONFIG}$" | awk '{print $1}')
    info "setup_deepin_clone_recovery find dataPartPath is : ${dataPartPath}"

    # 查找recovery分区,(根据挂载点来查找)
    local recoveryPartPath=$(lsblk -lp -o NAME,MOUNTPOINT | grep -w "/target/recovery$" | awk '{print $1}')
    if [ -z ${recoveryPartPath} ]; then
      return 0
    fi

    info "setup_deepin_clone_recovery find recoveryPartPath is : ${recoveryPartPath}"

    # 查找efi分区,(根据挂载点来查找)
    local efiPartPath=$(lsblk -lp -o NAME,MOUNTPOINT | grep -w "/target/boot/efi$" | awk '{print $1}')
    info "setup_deepin_clone_recovery find efiPartPath is : ${efiPartPath}"

    # 查找boot分区,(根据挂载点来查找)
    local bootPartPath=$(lsblk -lp -o NAME,MOUNTPOINT | grep -w "/target/boot$" | awk '{print $1}')
    info "setup_deepin_clone_recovery find bootPartPath is : ${bootPartPath}"

    # 查找roota分区,(根据挂载点来查找)
    local rootaPartPath=$(lsblk -lp -o NAME,MOUNTPOINT | grep -w "/target$" | awk '{print $1}')
    info "setup_deepin_clone_recovery find rootaPartPath is : ${rootaPartPath}"

    # 记录recovery的UUID
    if [ -n "${recoveryPartPath}" ]; then
      mkdir -p /target/recovery/backup
      local recoveryUUID=$(lsblk -lp -o UUID,NAME | grep -i "${recoveryPartPath}$" | awk '{print $1}')
      echo "UUID=${recoveryUUID}" > /target/etc/deepin/system-recovery.conf
    fi

    # 备份boot的UUID
    if [ -n "${bootPartPath}" ]; then
      local bootUUID=$(lsblk -lp -o UUID,NAME | grep -i "${bootPartPath}$" | awk '{print $1}')
      echo "${bootUUID}" > /target/recovery/backup/boot.info
    fi

    # 备份efi的UUID
    if is_uefi; then
      if [ -n "${efiPartPath}" ]; then
         local efiUUID=$(lsblk -lp -o UUID,NAME | grep -i "${efiPartPath}$" | awk '{print $1}')
         echo "${efiUUID}" > /target/recovery/backup/efi.info
      fi
    fi

    # 备份系统的UUID
    if [ -n "${rootaPartPath}" ]; then
      local rootaUUID=$(lsblk -lp -o UUID,NAME | grep "${rootaPartPath}$" | awk '{print $1}')
      echo "${rootaUUID}" > /target/recovery/backup/system.info
    fi

    # 备份data的UUID
    if [ -n "${dataPartPath}" ]; then
      local dataUUID=$(lsblk -lp -o UUID,NAME | grep "${dataPartPath}$" | awk '{print $1}')
      echo "${dataUUID}" > /target/recovery/backup/_dde_data.info
    fi

    local variable_done_data_var_opt_copy=0
    local is_init_recovery=$(installer_get "DI_IS_INIT_RECOVERY")
    info "get is_init_recovery is : ${is_init_recovery}"
    if [ x"${is_init_recovery}" != xtrue ]; then
      return 0
    fi

    # 设置/target中的安装模式为default模式
    deepin-installer-config set /target"${DI_CONFIG_FILE_ENV}" "DI_INSTALL_MODE" "default"

    # 设置/target中禁用审核模式
    deepin-installer-config set /target"${DI_CONFIG_FILE_ENV}" "DI_IS_CHECK_MODE" "false"

    # 启用后配置服务
    local is_firstboot_service_enabled=$(chroot /target systemctl is-enabled deepin-installer-first-boot)
    info "deepin-installer-first-boot service status in target is : ${is_firstboot_service_enabled}"
    if [ x"${is_firstboot_service_enabled}" = xdisabled ]; then
      chroot /target systemctl enable deepin-installer-first-boot
      info "set deepin-installer-first-boot service status $(chroot /target systemctl is-enabled deepin-installer-first-boot)"
    fi

    # 禁用extra服务置服务
    local is_extra_service_enabled=$(chroot /target systemctl is-enabled deepin-installer-extra)
    info "deepin-installer-extra service status in target is : ${is_extra_service_enabled}"
    if [ x"${is_extra_service_enabled}" = xenabled ]; then
      chroot /target systemctl disable deepin-installer-extra
      info "set deepin-installer-extra service status $(chroot /target systemctl is-enabled deepin-installer-extra)"
    fi

    # 卸载目录
    info "part mountpoints is :"
    info "$(cat /proc/mounts | awk '{print $2}' | grep -e ^/target | sort -r)"
    parted_umount

    # 处理根分区下的/var、/opt目录
    if [ -n "${dataPartPath}" ];then
      mkdir -p /dataVarOpt
      mount "${dataPartPath}" /dataVarOpt
      mount "${rootaPartPath}" /target
      rm -rf /target/var || true
      rm -rf /target/opt || true
      # copy /data/var => /var
      cp -a /dataVarOpt/var /target/
      # copy /data/opt => /opt
      cp -a /dataVarOpt/opt /target/
      umount -Rlf /dataVarOpt
      rm -rf /dataVarOpt
      umount -Rlf /target
      variable_done_data_var_opt_copy=1
    fi

    # 挂载recovery分区
    mkdir -p /target/recovery
    mount "${recoveryPartPath}" /target/recovery

    # 备份boot分区
    deepin-clone --tui "${bootPartPath}" /target/recovery/backup/boot.dim 1>/dev/null || warning "deepin-clone --tui "${bootPartPath}" /target/recovery/backup/boot.dim failed!"
    if [ -e "/target/recovery/backup/boot.dim" ];then
      md5sum /target/recovery/backup/boot.dim > /target/recovery/backup/boot.md5
    else
      info "/target/recovery/backup/boot.dim not exist!"
    fi

    # 备份根分区
    deepin-clone --tui "${rootaPartPath}" /target/recovery/backup/system.dim 1>/dev/null || warning "deepin-clone --tui "${rootaPartPath}" /target/recovery/backup/system.dim failed!"
    if [ -e "/target/recovery/backup/system.dim" ];then
      md5sum /target/recovery/backup/system.dim > /target/recovery/backup/system.md5
    else
      info "/target/recovery/backup/system.dim not exist!"
    fi

    # 清空/var、/opt目录
    if [ ${variable_done_data_var_opt_copy} -eq 1 ];then
      mkdir -p /clearVarOpt
      mount "${rootaPartPath}" /clearVarOpt
      rm -rf /clearVarOpt/var/*
      rm -rf /clearVarOpt/opt/*
      umount -Rlf /clearVarOpt
      rm -rf /clearVarOpt
    fi

    umount /target/recovery

    # 分区挂载
    local mount_points=$(installer_get "DI_MOUNTS_POINTS")
    deepin-installer-parted -m 4 -c "$mount_points"   # 分区挂载
    setup_swap       # 处理swap分区
    bind_data_mount  # 处理data分区内部目录的bind
    setup_efi_parted
    setup_mountpoint # 绑定设备目录位置

    # 设置/target中的安装模式为当前设置的模式
    local install_mode=$(installer_get "DI_INSTALL_MODE")
    deepin-installer-config set /target"${DI_CONFIG_FILE_ENV}" "DI_INSTALL_MODE" "${install_mode}"

    # 设置/target中审核模式启用状态为当前设置
    local is_check_mode=$(installer_get "DI_IS_CHECK_MODE")
    deepin-installer-config set /target"${DI_CONFIG_FILE_ENV}" "DI_IS_CHECK_MODE" "${is_check_mode}"

    # 还原/target中的firstboot服务状态
    if [ x"${is_firstboot_service_enabled}" = xenabled ]; then
      chroot /target systemctl enable deepin-installer-first-boot
    else
      chroot /target systemctl disable deepin-installer-first-boot
    fi
    info "restore set deepin-installer-first-boot service status $(chroot /target systemctl is-enabled deepin-installer-first-boot)"

    # 还原/target中的extra服务状态
    if [ x"${is_extra_service_enabled}" = xenabled ]; then
      chroot /target systemctl enable deepin-installer-extra
    else
      chroot /target systemctl disable deepin-installer-extra
    fi
    info "restore set deepin-installer-extra service status $(chroot /target systemctl is-enabled deepin-installer-extra)"

}

# ghost安装生成初始化备份
setup_deepin_clone_recovery_ghost() {
    ## 判断是否全盘安装模式
    if ! is_full_disk_mode; then
        info "setup_deepin_clone_recovery_ghost nothing to do !"
        return 0
    fi

    # 查找data分区,(根据挂载点来查找)
    local DATA_MOUNT_POINT_CONFIG=$(installer_get "DI_DATA_MOUNT_POINT_CONFIG" "${DI_OS_VERSION_ENV}")
    local dataPartPath=$(lsblk -lp -o NAME,MOUNTPOINT | grep -w "/target${DATA_MOUNT_POINT_CONFIG}$" | awk '{print $1}')
    info "setup_deepin_clone_recovery find dataPartPath is : ${dataPartPath}"

    # 查找recovery分区,(根据挂载点来查找)
    local recoveryPartPath=$(lsblk -lp -o NAME,MOUNTPOINT | grep -w "/target/recovery$" | awk '{print $1}')
    if [ -z ${recoveryPartPath} ]; then
      return 0
    fi

    info "setup_deepin_clone_recovery find recoveryPartPath is : ${recoveryPartPath}"

    # 查找efi分区,(根据挂载点来查找)
    local efiPartPath=$(lsblk -lp -o NAME,MOUNTPOINT | grep -w "/target/boot/efi$" | awk '{print $1}')
    info "setup_deepin_clone_recovery find efiPartPath is : ${efiPartPath}"

    # 查找boot分区,(根据挂载点来查找)
    local bootPartPath=$(lsblk -lp -o NAME,MOUNTPOINT | grep -w "/target/boot$" | awk '{print $1}')
    info "setup_deepin_clone_recovery find bootPartPath is : ${bootPartPath}"

    # 查找roota分区,(根据挂载点来查找)
    local rootaPartPath=$(lsblk -lp -o NAME,MOUNTPOINT | grep -w "/target$" | awk '{print $1}')
    info "setup_deepin_clone_recovery find rootaPartPath is : ${rootaPartPath}"

    # 记录recovery的UUID
    if [ -n "${recoveryPartPath}" ]; then
      mkdir -p /target/recovery/backup
      local recoveryUUID=$(lsblk -lp -o UUID,NAME | grep -i "${recoveryPartPath}$" | awk '{print $1}')
      echo "UUID=${recoveryUUID}" > /target/etc/deepin/system-recovery.conf
    fi

    # 备份boot的UUID
    if [ -n "${bootPartPath}" ]; then
      local bootUUID=$(lsblk -lp -o UUID,NAME | grep -i "${bootPartPath}$" | awk '{print $1}')
      echo "${bootUUID}" > /target/recovery/backup/boot.info
    fi

    # 备份efi的UUID
    if is_uefi; then
      if [ -n "${efiPartPath}" ]; then
         local efiUUID=$(lsblk -lp -o UUID,NAME | grep -i "${efiPartPath}$" | awk '{print $1}')
         echo "${efiUUID}" > /target/recovery/backup/efi.info
      fi
    fi

    # 备份系统的UUID
    if [ -n "${rootaPartPath}" ]; then
      local rootaUUID=$(lsblk -lp -o UUID,NAME | grep "${rootaPartPath}$" | awk '{print $1}')
      echo "${rootaUUID}" > /target/recovery/backup/system.info
    fi

    # 备份data的UUID
    if [ -n "${dataPartPath}" ]; then
      local dataUUID=$(lsblk -lp -o UUID,NAME | grep "${dataPartPath}$" | awk '{print $1}')
      echo "${dataUUID}" > /target/recovery/backup/_dde_data.info
    fi

    local variable_done_data_var_opt_copy=0
    local is_init_recovery=$(installer_get "DI_IS_INIT_RECOVERY")
    info "get is_init_recovery is : ${is_init_recovery}"
    if [ x"${is_init_recovery}" != xtrue ]; then
      return 0
    fi

    # 设置/target中的安装模式为default模式
    deepin-installer-config set /target"${DI_CONFIG_FILE_ENV}" "DI_INSTALL_MODE" "default"

    # 设置/target中禁用审核模式
    deepin-installer-config set /target"${DI_CONFIG_FILE_ENV}" "DI_IS_CHECK_MODE" "false"

    # 启用后配置服务
    local is_firstboot_service_enabled=$(chroot /target systemctl is-enabled deepin-installer-first-boot)
    info "deepin-installer-first-boot service status in target is : ${is_firstboot_service_enabled}"
    if [ x"${is_firstboot_service_enabled}" = xdisabled ]; then
      chroot /target systemctl enable deepin-installer-first-boot
      info "set deepin-installer-first-boot service status $(chroot /target systemctl is-enabled deepin-installer-first-boot)"
    fi

    # 禁用extra服务置服务
    local is_extra_service_enabled=$(chroot /target systemctl is-enabled deepin-installer-extra)
    info "deepin-installer-extra service status in target is : ${is_extra_service_enabled}"
    if [ x"${is_extra_service_enabled}" = xenabled ]; then
      chroot /target systemctl disable deepin-installer-extra
      info "set deepin-installer-extra service status $(chroot /target systemctl is-enabled deepin-installer-extra)"
    fi

    # 卸载目录
    info "part mountpoints is :"
    info "$(cat /proc/mounts | awk '{print $2}' | grep -e ^/target | sort -r)"
    parted_umount

    # 处理根分区下的/var、/opt目录
    if [ -n "${dataPartPath}" ];then
      mkdir -p /dataVarOpt
      mount "${dataPartPath}" /dataVarOpt
      mount "${rootaPartPath}" /target
      rm -rf /target/var || true
      #rm -rf /target/opt || true
      # copy /data/var => /var
      cp -a /dataVarOpt/var /target/
      # copy /data/opt => /opt
      #cp -a /dataVarOpt/opt /target/
      umount -Rlf /dataVarOpt
      rm -rf /dataVarOpt
      umount -Rlf /target
      variable_done_data_var_opt_copy=1
    fi

    # 挂载recovery分区
    mkdir -p /target/recovery
    mount "${recoveryPartPath}" /target/recovery

    # 备份boot分区
    deepin-clone --tui "${bootPartPath}" /target/recovery/backup/boot.dim 1>/dev/null || error "deepin-clone --tui "${bootPartPath}" /target/recovery/backup/boot.dim failed!"
    if [ -e "/target/recovery/backup/boot.dim" ];then
      md5sum /target/recovery/backup/boot.dim > /target/recovery/backup/boot.md5
    else
      info "/target/recovery/backup/boot.dim not exist!"
    fi

    # 备份根分区
    deepin-clone --tui "${rootaPartPath}" /target/recovery/backup/system.dim 1>/dev/null || error "deepin-clone --tui "${rootaPartPath}" /target/recovery/backup/system.dim failed!"
    if [ -e "/target/recovery/backup/system.dim" ];then
      md5sum /target/recovery/backup/system.dim > /target/recovery/backup/system.md5
    else
      info "/target/recovery/backup/system.dim not exist!"
    fi

    # 清空/var、/opt目录
    if [ ${variable_done_data_var_opt_copy} -eq 1 ];then
      mkdir -p /clearVarOpt
      mount "${rootaPartPath}" /clearVarOpt
      rm -rf /clearVarOpt/var/*
      rm -rf /clearVarOpt/opt/*
      umount -Rlf /clearVarOpt
      rm -rf /clearVarOpt
    fi

    umount -Rlf /target/recovery

    # 分区挂载
    local mount_points=$(installer_get "DI_MOUNTS_POINTS")
    deepin-installer-parted -m 4 -c "$mount_points"   # 分区挂载
    setup_swap       # 处理swap分区
    bind_data_mount  # 处理data分区内部目录的bind
    setup_efi_parted
    setup_mountpoint # 绑定设备目录位置
}

# 有后配置情况下替换lightdm配置。
cleanup_lightdm_deepin_installer() {
    local CONF_FILE=/etc/lightdm/lightdm.conf
    local TEMP_CONF_FILE=/etc/lightdm/lightdm.conf.real
    if [ -e "${TEMP_CONF_FILE}" ]; then
        mv "${TEMP_CONF_FILE}" "${CONF_FILE}"
    fi
}

setup_live_install_desktop() {
    local homedir="/home"
    local file=${DI_INSTALL_DIR_ENV}/configs/live-config/deepin-installer.desktop
    if is_community; then
        file=${DI_INSTALL_DIR_ENV}/configs/live-config/deepin-installer_deepin.desktop
    fi
    for user in $(ls $homedir); do
        if [ -f "$file" ]; then
            install -d -o $user -g $user /home/$user/Desktop
            install -D -o $user -g $user $file /home/$user/Desktop/$(basename "$file")
        fi
    done
}

setup_live_workdir() {
    # 光驱安装时，启动盘的挂载点可能会是/run/live/medium
    # 统一启动盘的挂载点为DI_LIVE_DIR_ENV
    if is_empty_dir $DI_LIVE_DIR_ENV; then
        info "$DI_LIVE_DIR_ENV is empty dir"
        mount -v --bind /run/live/medium/ $DI_LIVE_DIR_ENV
    fi

    info "$DI_LIVE_DIR_ENV not empty dir"
}

setup_live_autologin() {
    local username=$(installer_get "LIVE_USERNAME")

    if ! grep -q "^autologin:" /etc/group; then
        groupadd autologin
    fi
    gpasswd -a ${username} autologin
}

setup_live_nopasswdlogin() {
    local username=$(installer_get "LIVE_USERNAME")

    # 如果nopasswdlogin组不存在，则创建
    if ! grep -q "^nopasswdlogin:" /etc/group; then
        groupadd nopasswdlogin
    fi
    # 添加Live用户到nopasswdlogin组
    usermod -aG nopasswdlogin "$username"
    info "set nopasswdlogin on $username"
}

setup_oem_squashfs() {
    if [ -f $DI_LIVE_DIR_ENV/oem.squashfs ]; then
        mount -v $DI_LIVE_DIR_ENV/oem.squashfs $DI_LIVE_DIR_ENV/oem
        info "mount ${DI_LIVE_DIR_ENV}/oem.squashfs success"
    else
        info "${DI_LIVE_DIR_ENV}/oem.squashfs not exist"
    fi
}

enable_quick_login() {
    local enable=$(dde-dconfig  -a org.deepin.dde.lightdm-deepin-greeter -r org.deepin.dde.daemon.accounts -k enableQuickLogin --get 2>/dev/null || echo "false")
    if [ "x$enable" == "xtrue" ]; then
        installer_set DI_QUICK_LOGIN_LOAD "true"
    else
        installer_set DI_QUICK_LOGIN_LOAD "false"
    fi
}

part_to_device() {
    local part_path=$1
    local partDevPath=$(lsblk -lnp -o PKNAME,NAME | grep -w "${part_path}$" | awk '{print $1}' | sed -n 1p)
    local partDevType=$(lsblk -lnp -o TYPE,NAME | grep -w "${partDevPath}$" | awk '{print $1}' | sed -n 1p)
    local deviceTypes=$(installer_get "DI_DEVICETYPES_CONFIG")
    while [[ ! "${deviceTypes}" =~ "${partDevType}" ]] ; do
        partDevPath=$(lsblk -lnp -o PKNAME,NAME | grep -w "${partDevPath}$" | awk '{print $1}' | sed -n 1p)
        partDevType=$(lsblk -lnp -o TYPE,NAME | grep -w "${partDevPath}$" | awk '{print $1}' | sed -n 1p)
    done
    echo -n "${partDevPath}"
}

part_num() {
    local part_path=$1
    echo -n ${part_path##*[a-zA-Z]}
}

fix_lightdm_user_session() {
    local conf_file="/etc/lightdm/lightdm.conf"
    if [ -f $conf_file ]; then
        sed -i 's/user-session=deepin/user-session=dde-x11/g' $conf_file
    else
        warning "lightdm.conf not found!"
    fi
}

setup_lightdm_auto_login() {
    local username=$1
    local conf_file="/etc/lightdm/lightdm.conf"
    if [ -f $conf_file ]; then
        sed -i "s|^[#]*[ ]*autologin-user\([ ]*\)=.*|autologin-user=${username}|g" $conf_file
    else
        warning "lightdm.conf not found!"
    fi
}

init_check_mode_lightdm() {
    local conf_file=/etc/lightdm/lightdm.conf
    local tmp_conf_file=/etc/lightdm/lightdm.conf.real
    if [ -f "${conf_file}" ]; then
      if [ ! -e "${tmp_conf_file}" ]; then
         install -v -m644 "${conf_file}" "${tmp_conf_file}"
      fi
    fi
    local username=$(installer_get DI_CM_USER)

cat > "${conf_file}" <<EOF
[Seat:*]
autologin-user=${username}
user-session=deepin
greeter-session=lightdm-deepin-greeter
display-stopped-script=bash $DI_INSTALL_TOOLS_DIR_ENV/check_hooks/check_mode_cleanup.sh
EOF
}

cleanup_check_mode() {
    if [ -f /etc/xdg/autostart/check_mode_start.desktop ]; then
        rm -f /etc/xdg/autostart/check_mode_start.desktop
    fi

    if [ -f /usr/share/applications/check_mode_start.desktop ]; then
        rm -f /usr/share/applications/check_mode_start.desktop
    fi

    if [ -f /usr/share/applications/check_mode_quit.desktop ]; then
        rm -f /usr/share/applications/check_mode_quit.desktop
    fi

    local username=$(installer_get DI_CM_USER)

    # 仅处理 /media/${username} 下的直接挂载点，避免误删挂载介质内的目录
    # 仅在成功卸载后删除空挂载点目录，最后尝试删除空的用户目录
    if [ -d /media/${username} ]; then
        find /media/"${username}" -mindepth 1 -maxdepth 1 -type d | while read -r dir; do
            if mountpoint -q "${dir}"; then
                if ! umount -Rlf "${dir}"; then
                    warning "umount -Rlf ${dir} failed!"
                    continue
                fi
            fi

            rmdir "${dir}" 2>/dev/null || true
        done

        rmdir /media/"${username}" 2>/dev/null || true
    fi

    delete_user $username
    installer_set DI_IS_CHECK_MODE "false"
}

init_check_mode_workspace() {
    info "init mode workspace..."
    ## 初始化审核模式的日志
    local log_file="/var/log/deepin-installer/deepin-installer-check-mode.log"
    touch $log_file
    chmod 666 $log_file

    local autoStart=$(installer_get DI_AUTO_CHECK_MODE)
    local username=$(installer_get DI_CM_USER)
    local start_file="/home/$username/Desktop/check_mode_start.desktop"
    if [ "x$autoStart" = "xtrue" ]; then
        start_file="/etc/xdg/autostart/check_mode_start.desktop"
    fi

    cat > $start_file << EOF
[Desktop Entry]
Exec=bash $DI_INSTALL_TOOLS_DIR_ENV/check_hooks/check_mode_start.sh
Name=check mode start
StartupNotify=false
Terminal=false
Type=Application
EOF

    cp -v  $start_file /usr/share/applications/check_mode_start.desktop

    cat > /home/$username/Desktop/check_mode_quit.desktop << EOF
[Desktop Entry]
Exec=bash $DI_INSTALL_TOOLS_DIR_ENV/check_hooks/check_mode_quit.sh
Name=check mode quit
StartupNotify=false
Terminal=false
Type=Application
EOF

    cp -v /home/$username/Desktop/check_mode_quit.desktop /usr/share/applications/check_mode_quit.desktop
}

before_check() {
    info "before check..."
    local work_dir="$DI_INSTALL_TOOLS_DIR_ENV/check_hooks/before_check"
    if ! is_empty_dir $work_dir;then
        for job in $(find $work_dir -maxdepth 1 -name "*.job" | sort); do
            bash $job
        done
    fi
    info "before check end!"
}

after_check() {
    info "after check..."
    local work_dir="$DI_INSTALL_TOOLS_DIR_ENV/check_hooks/after_check"
    if ! is_empty_dir $work_dir;then
        for job in $(find $work_dir -maxdepth 1 -name "*.job" | sort); do
            bash $job
        done
    fi
    info "after check end!"
}

in_check() {
    info "in check..."
    local work_dir="$DI_INSTALL_TOOLS_DIR_ENV/check_hooks/in_check"
    if ! is_empty_dir $work_dir;then
        for job in $(find $work_dir -maxdepth 1 -name "*.job" | sort); do
            bash  $job
        done
    fi
    info "in check end!"

    local autoStart=$(installer_get DI_AUTO_CHECK_MODE)
    if [ "x$autoStart" = "xtrue" ]; then
        info "is auto check mode, now to after check!"
        /usr/lib/qt6/bin/qdbus --session --literal  org.deepin.dde.SessionManager1 /org/deepin/dde/SessionManager1  org.deepin.dde.SessionManager1.RequestLogout 

    fi
}

is_base64() {
    local crypt=$1
    local plain=$(echo ${crypt} | base64 -d)

    if [ "x${crypt}" = "x$(echo -n ${plain} | base64)" ]; then
        return 0
    else
        return 1
    fi
}

program_exist() {
    which $1
    return $?
}

tpm2_get_support_pcr_bank() {
    local fields=""
    local keyValue=""
    local bankName=""
    local bankState=""

    tpm2_pcrread > /tmp/tpm2_pcrread.txt

    while read line; do
        local linecontent=(${line}) ##($(echo -n ${line} | tr ':' ' '))

        if [ ${#linecontent[@]} -eq 1 ]; then
            fieldStr="${fields[@]}"

            if [ -n "${fieldStr}" ]; then
                # 解析bank名
                bankName=${fieldStr%%:*}
                bankName=$(echo -n "${bankName}" | tr -d ' ')

                # 记录非空的bank项
                bankState=$(echo -n "${fieldStr}" | tr -d "${bankName}:;" | tr -d ' ')
                if [ -n "${bankState}" ]; then
                    keyValue[${#keyValue[@]}]="${bankName}:1;"
                fi

                fields=()
                bankName=""
                bankState=""

                fields="${line};"

                 continue
            fi
        fi

        # 拼接需要解析的bank项字符串
        fields[${#fields[@]}]="${line};"

    done < /tmp/tpm2_pcrread.txt

    # 处理循环中没有解析的bank项字符串
    fieldStr="${fields[@]}"
    if [ -n "${fieldStr}" ]; then
        bankName=${fieldStr%%:*}
        bankName=$(echo -n "${bankName}" | tr -d ' ')
        bankState=$(echo -n "${fieldStr}" | tr -d "${bankName}:;" | tr -d ' ')
        if [ -n "${bankState}" ]; then
            keyValue[${#keyValue[@]}]="${bankName}:1;"
        fi

        fields=()
        bankName=""
        bankState=""
    fi

    echo -n ${keyValue[@]} | tr -d ' '
}

tpm2_get_support_algs() {
    local fields=""
    local keyValue=""
    local algName=""
    local symmetricState=""

    tpm2_getcap algorithms > /tmp/tpm2_algorithms.txt

    while read line; do
        local linecontent=(${line}) ##($(echo -n ${line} | tr ':' ' '))

        if [ ${#linecontent[@]} -eq 1 ]; then
            fieldStr="${fields[@]}"

            if [ -n "${fieldStr}" ]; then
                # 解析算法名
                algName=${fieldStr%%:*}
                algName=$(echo -n "${algName}" | tr -d ' ')
                # 解析算法标识
                symmetricState=${fieldStr##*symmetric:}
                symmetricState=${symmetricState%%;*}
                symmetricState=$(echo -n "${symmetricState}" | tr -d ' ')
                # 记录是算法的项
                if [ ${symmetricState} -eq 1 ]; then
                    keyValue[${#keyValue[@]}]="${algName}:${symmetricState};"
                fi

                fields=()
                algName=""
                symmetricState=""

                fields="${line};"

                continue
            fi
        fi

        # 拼接需要解析的算法项字符串
        fields[${#fields[@]}]="${line};"

    done < /tmp/tpm2_algorithms.txt

    # 处理循环中没有解析的算法项字符串
    fieldStr="${fields[@]}"
    if [ -n "${fieldStr}" ]; then
        algName=${fieldStr%%:*}
        algName=$(echo -n "${algName}" | tr -d ' ')
        symmetricState=${fieldStr##*symmetric:}
        symmetricState=${symmetricState%%;*}
        symmetricState=$(echo -n "${symmetricState}" | tr -d ' ')
        if [ ${symmetricState} -eq 1 ]; then
            keyValue[${#keyValue[@]}]="${algName}:${symmetricState};"
        fi

        fields=()
        algName=""
        symmetricState=""
    fi

    echo -n ${keyValue[@]} | tr -d ' '
}

init_tpm_check() {
    # 先探测一下dev下是否存在tpm、tcm设备
    local isHaveTpm=$(ls /dev | grep -E "tpm|tcm" || echo "")
    if [ -n "${isHaveTpm}" ]; then
        tpm2-abrmd --allow-root > /dev/null 2>&1 || true &

        local tpmAlgos=$(tpm2_get_support_algs);
        local tpmPcrBanks=$(tpm2_get_support_pcr_bank);

        if [ -n "$tpmAlgos" ] && [ -n "${tpmPcrBanks}" ]; then
            installer_set "DI_TPM_CHECK" "true"
            installer_set "DI_TPM_ALGS" "${tpmAlgos}"
            installer_set "DI_TPM_PCR_BANCKS" "${tpmPcrBanks}"
        else
            installer_set "DI_TPM_CHECK" "false"
            installer_set "DI_TPM_ALGS" ""
            installer_set "DI_TPM_PCR_BANCKS" ""
        fi

        echo "$(ps -aux | grep 'tpm2-abrmd --allow-root$' | awk '{print $2}')" | xargs -I {} kill -9 {} > /dev/null 2>&1 || true
    else
        echo "tpm or tcm device not found"
        installer_set "DI_TPM_CHECK" "false"
        installer_set "DI_TPM_ALGS" ""
        installer_set "DI_TPM_PCR_BANCKS" ""
    fi
}

is_tpm() {
    local tmp_check=$(installer_get DI_TPM_CHECK)
    if [ "x$tmp_check" = "xtrue" ]; then
        return 0
    else
        return 1
    fi
}

deepin_nvidia_installer() {
    if [ -f /usr/sbin/deepin-nvidia-installer ]; then
        #deepin-nvidia-installer 此阶段不需要安装包，不用设置本地仓库
        deepin-nvidia-installer --installer-init  autoinstall || echo "run deepin-nvidia-installer --installer-init  autoinstall error"
    else
        echo "/usr/sbin/deepin-nvidia-installer is not found"
    fi
}

disable_screensaver() {
    xset s off
    xset s noblank
    xset -dpms
    info "disable screen saver done."
}

set_brightness_to_max() {
    local gpu_brignt=$(echo -n "$(ls /sys/class/backlight)")
    info "gpu_brignt=${gpu_brignt}"

    for brigntItem in ${gpu_brignt}; do
        if [ -n "${brigntItem}" ]; then
            info "cat /sys/class/backlight/${brigntItem}/max_brightness | tee /sys/class/backlight/${brigntItem}/brightness"
            cat /sys/class/backlight/${brigntItem}/max_brightness | tee /sys/class/backlight/${brigntItem}/brightness
        fi
    done
}

setup_passwd_validate() {
    local strong_password=$(installer_get DI_PASSWORD_STRONG_CHECK_CONFIG)
    local password_min_len=$(installer_get DI_PASSWORD_MIN_LEN_CONFIG)
    local password_max_len=$(installer_get DI_PASSWORD_MAX_LEN_CONFIG)
    local validate_policy=$(installer_get DI_PASSWORD_VALIDATE_CONFIG)
    local validate_required=$(installer_get DI_PASSWORD_VALIDATE_REQUIRED_CONFIG)
    local palindrome_num=$(installer_get DI_PASSWORD_PALINGROME_LENGTH_CONFIG)
    local consecutive_same_char_num=$(installer_get DI_PASSWORD_CONTINUOUS_LENGTH_CONFIG)
    local monotone_char_num=$(installer_get DI_PASSWORD_MONOTONOUS_LENGTH_CONFIG)

    cat > /etc/deepin/dde.conf << EOF
[Password]
STRONG_PASSWORD = ${strong_password}
PASSWORD_MIN_LENGTH = ${password_min_len}
PASSWORD_MAX_LENGTH = ${password_max_len}
VALIDATE_POLICY = ${validate_policy}
VALIDATE_REQUIRED = ${validate_required}
PALINDROME_NUM = ${palindrome_num}
CONSECUTIVE_SAME_CHARACTER_NUM = ${consecutive_same_char_num}
MONOTONE_CHARACTER_NUM = ${monotone_char_num}
EOF

    /usr/bin/pwd-conf-update || echo "pwd-conf-update failed"
}

pxe_update_oem() {
    # 判断pxe安装0-非pxe安装、1-nfs服务pxe安装、2-文件系统使用http下载本地、远程挂载方式pxe安装
    local pxe_install_type=0
    local cmdline_content=$(cat /proc/cmdline)
    ## 解析网络路径
    local net_path=""
    # 文件系统路径
    local filsystem_path="live/filesystem.squashfs"

    if [[ "${cmdline_content}" =~ "nfsroot=" ]]; then
        pxe_install_type=1
    fi

    if [[ "${cmdline_content}" =~ "httpfs=" ]] && [[ "${cmdline_content}" =~ "${filsystem_path}" ]]; then
        pxe_install_type=2
        net_path=${cmdline_content#*httpfs=}
    fi

    if [[ "${cmdline_content}" =~ "fetch=" ]] && [[ "${cmdline_content}" =~ "${filsystem_path}" ]]; then
        pxe_install_type=2
        net_path=${cmdline_content#*fetch=}
    fi

    info "*** pxe_install_type is : ${pxe_install_type}"

    # 下载oem目录到本地
    if [ ${pxe_install_type} -eq 2 ]; then
        if [ -n "${net_path}" ]; then
            net_path=${net_path%${filsystem_path}*}
            info "*** net_path is : ${net_path}"

            ## 找到文件系统挂载目录
            filesystem_mountpoint=$(lsblk -no MOUNTPOINT | grep filesystem.squashfs)
            info "*** filesystem_mountpoint is : ${filesystem_mountpoint}"

            if [ -n "${filesystem_mountpoint}" ];
            then
                filesystem_mountpoint=${filesystem_mountpoint%rootfs/filesystem.squashfs*}
                info "*** filesystem_mountpoint is : ${filesystem_mountpoint}"

                ## 先下载oem目录
                wget -P ${filesystem_mountpoint}medium/oem/ --cut-dirs=2 -r -c -k -np -nH -R *index.html* ${net_path}oem/
                info "*** download : ${net_path}medium/oem/ to ${filesystem_mountpoint}medium/oem/"

                ## 再下载pxemode目录
                wget -P ${filesystem_mountpoint}medium/pxemode/ --cut-dirs=2 -r -c -k -np -nH -R *index.html* ${net_path}pxemode/
                info "*** download : ${net_path}medium/pxemode/ to ${filesystem_mountpoint}medium/pxemode/"

                ## 合并pxemode和oem
                local pxeModePxeSettings=${filesystem_mountpoint}medium/pxemode/settings.ini-pxe
                local pxeModeSettings=${filesystem_mountpoint}medium/pxemode/settings.ini
                local oemSettings=${filesystem_mountpoint}medium/oem/settings.ini
                if [ -e ${pxeModeSettings} ]; then
                    cp -rvf ${pxeModeSettings} ${oemSettings}
                fi

                if [ -e ${pxeModePxeSettings} ]; then
                    cp -rvf ${pxeModePxeSettings} ${oemSettings}
                fi

            else
                info "filesystem_mountpoint is empty!"
            fi

            info "ls ${filesystem_mountpoint}medium/ : $(ls ${filesystem_mountpoint})medium/"
        else
            info "net_path is empty!"
        fi
    fi
}


# 读取INI配置文件的函数
read_ini_file() {
    key=$1
    section=$2
    configfile=$3
    cat $configfile | grep $key -w >/dev/null 2>&1
    if [[ $? -ne 0 ]]; then
        echo "read $key from ini fail"
        return 1
    fi

    local read_data=$(awk -F '=' '/\['$section'\]/{a=1} a==1&&$1~/'$key'/ {print $2;exit}' $configfile)
    echo "$read_data"
}

# 杀死在chroot阶段开启的进程，因为chroot阶段开启的进程，在退出chroot阶段后，会继续运行。
# 导致分区未卸载干净，影响deepin-clone执行初始化备份
kill_process_opened_by_chroot() {
    info "kill_process_opened_by_chroot"
    find /proc -maxdepth 1 -type d | grep -P '/proc/\d+' | while read -r path; do
        #echo "path=$path"
        if [ ! -f "${path}/cmdline" ]; then
            continue
        fi
        apppid=${path:6}
        exefile=$(readlink "${path}/exe" || echo "")
        #echo "execfile=$exefile"
        rootdir=$(readlink "${path}/root" || echo "")
        #echo "rootdir=$rootdir"
        if [ "${rootdir}" == "/target" ]; then
            info "${exefile} - ${rootdir}"
            kill -9 ${apppid} || echo "kill ${exefile}:${apppid} failed"
        fi
    done
}

copy_locales() {
    mkdir -p /boot/usec-crypt
    cp -vfr /etc/default/locale /boot/usec-crypt
}

init_nvidia_check() {
    if lspci -n | grep -i 10de | egrep -i '.*03(80|0[0-2])' && is_community ;then
        installer_set "DI_NVIDIA_CHECK" "true"
    else
        installer_set "DI_NVIDIA_CHECK" "false"
    fi
}

# generate a file: ab_recovery.json
# 步骤：
# 1.从配置文件取得当前根分区的label。
# 2.假定备用根分区的label为"Rootb"(见full_disk_policy.json)。
# 3.取得两个根分区的uuid。
# 4.将两个uuid写入ab_recovery.json文件中。
do_ab_recovery_generate()
{
    if ! is_full_disk_mode; then
        warning "Maybe not in FULLDISK installation mode!"
        return
    fi

    local ab_recovery_file="/target/etc/deepin/ab-recovery.json"
    local current_mnt="/target"

    info "Generating ${ab_recovery_file}"

    #current_label use KNAME
    local current_label=$(lsblk -lnfp -o NAME,MOUNTPOINT | grep -E ".*\s+${current_mnt}$" | awk '{print $1}')
    local current_pkname=$(lsblk -lnfp -o PKNAME,NAME | grep ${current_label} | awk '{print $1}')
    info "roota pkname:$current_pkname"

    local backup_uuid=$(lsblk -lnfp -o PKNAME,UUID,LABEL | grep Rootb | grep ${current_pkname} | awk '{print $2}')
    local current_uuid="$(lsblk -n -o UUID ${current_label})"
    info "current:{${current_label}} {${current_uuid}}"
    info "backup:{${backup_uuid}}"

    if [ -z ${current_uuid} ]; then
        error "Failed to get current uuid of {${current_label}}"
        return
    fi

    if [ -z ${backup_uuid} ]; then
        error "Failed to get backup uuid of {${backup_uuid}}"
        return
    fi

    mkdir -p $(dirname ${ab_recovery_file})
    cat > ${ab_recovery_file} <<EOF
{
       "Current": "${current_uuid}",
       "Backup" : "${backup_uuid}"
}
EOF

    info "done."
}

# 定义一个函数来检查数组是否包含某项
contains_element()
{
    local element=$1
    # 移除第一个参数（要查找的元素），剩下的参数是数组
    shift
    # 将剩余的参数作为数组
    local array=("$@")

    for item in "${array[@]}"; do
        if [[ "$item" == "$element" ]]; then
          return 0
        fi
    done
    # 没找到，返回 1 (false)
    return 1
}

# 拷贝fstab和lsb-release到根分区设备的根目录下
copy_fstab_lsbrelease_to_rootdev() {
    mkdir /testCopy
    local root_device=$(installer_get DI_ROOT_DEVICE_CONFIG)
    mount $root_device /testCopy || error "mount $root_device /testCopy failed"

    mkdir -p /testCopy/etc
    cp -v /target/etc/fstab /testCopy/etc/
    # 注释掉以"/sysroot/boot /boot"开始的行，否则安装双系统时，第二个系统os-prober会报错
    sed -i  -E '\|^[^#]*/sysroot/boot[[:space:]]+/boot| s|^|#|' /testCopy/etc/fstab
    cp -v /target/etc/lsb-release /testCopy/etc/

    umount /testCopy || error "umount testCopy failed"

    rm -rf /testCopy
}

# 从command line 读取 DI_EDITION_NAME 写到配置文件中
reade_edition_name_from_cmdline() {
    for cmdLineParameter in $(cat /proc/cmdline); do
        if [[ "${cmdLineParameter}" =~ "DI_EDITION_NAME" ]]; then
            local editionName=${cmdLineParameter#*=}
            installer_set "DI_EDITION_NAME" "${editionName}"
        fi
    done
}

enable_developer_mode() {
    [ -d /var/lib/deepin/developer-mode ] || mkdir -pv /var/lib/deepin/developer-mode
    echo -n 1 > /var/lib/deepin/developer-mode/enabled
    # 1075和v25 开发者模式由deepin-elf-verify控制，该服务启动较早，所以这里修改
    #  /var/lib/deepin/developer-mode/enabled并不能马上生效。需要重启服务
    systemctl restart deepin-elf-verify
}

disable_developer_mode() {
    if [ -f /var/lib/deepin/developer-mode/enabled ]; then
        rm -f /var/lib/deepin/developer-mode/enabled
        systemctl restart deepin-elf-verify
    fi
}
