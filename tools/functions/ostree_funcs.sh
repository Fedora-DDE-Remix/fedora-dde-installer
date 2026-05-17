#!/bin/bash

function is_ostree_system() {
    local immutable_system=$(installer_get "DI_IMMUTABLE_SYSTEM")
    if [[ $immutable_system == "true" ]]; then
        return 0
    else
        return 1
    fi
}

function ostree_get_deploy() {
    local commitID ostree ostree_refspec sysroot
    sysroot=$1
    ostree_refspec=$2
    commitID=$(ostree --repo=$sysroot/ostree/repo log "$ostree_refspec" | grep commit | head -n 1 | awk '{print $2}')
    ostree="/ostree/deploy/deepin/deploy/${commitID}.0"
    echo "$ostree"
}

function ostree_sys_init() {
    mkdir -p /target/boot || error "mkdir /target/boot failed"

    local sysroot=/target
    local os_name=deepin
    local ostree_ref_spec=$DI_REPO_BASE_NAME_ENV
    if [[ x"$(installer_get DI_IS_GHOST_MODE)" == x"true" ]]; then
        ostree_ref_spec=$(cat /uimg/localhost/etc/immutable.conf/base-name)
    fi
    info "ostree_ref_spec=$ostree_ref_spec"
    local sys_dir=$sysroot/ostree/deploy/$os_name/deploy
    mkdir -p $sys_dir || error "mkdir $sys_dir failed"
    # 检查是否存在deployment，格式为deployment/数字/0
    local sys_commit=$(ostree --repo=$sysroot/ostree/repo refs -r | grep -E "^deployment/[0-9]+/0" | head -n 1 | awk '{print $2}' || echo "")
    # 如果sys_commit为空，则使用ostree_ref_spec
    if [ -z "$sys_commit" ]; then
        info "sys_commit is empty, use ostree_ref_spec"
        sys_commit=$(ostree rev-parse --repo=$sysroot/ostree/repo $ostree_ref_spec)
        info "sys_commit=$sys_commit"
        ostree checkout --repo=$sysroot/ostree/repo $ostree_ref_spec "$sysroot/ostree/deploy/$os_name/deploy/$sys_commit.0" || error "ostree_sys_init failed"
    else
        info "sys_commit=$sys_commit"
        ostree checkout --repo=$sysroot/ostree/repo $sys_commit $sys_dir/$sys_commit.0 || error "ostree_sys_init failed"
    fi
   
    info "ostree_sys_init ok"
}

function ostree_ext_mount() {
    local lowerdir=/target$1
    local ostree=$2
    local datadir=/target/persistent

    local mount_root_tool
    if which deepin-mount-root.sh > /dev/null 2>&1; then
        mount_root_tool=deepin-mount-root.sh
    elif which deepin-immutable-mount-root > /dev/null 2>&1; then
        mount_root_tool=deepin-immutable-mount-root
    else
        mount_root_tool=
        info "mount_root_tool not found"
    fi

    # 如果存在mount_root_tool，优先使用mount_root_tool挂载
    if [ -n "$mount_root_tool" ]; then
        local data_device=$(installer_get "DI_DATA_DEVICE_CONFIG")
        info "data_device=$data_device"
    
        local ostree_arg=$2
        mkdir -p $datadir/ostree/deploy/deepin/var
        mkdir -p $lowerdir/var || true
        $mount_root_tool /target --ostree=$ostree --persistent=$data_device || error "$mount_root_tool /target $ostree failed"
        mount -o remount,rw /target/usr /target/usr || error "remount /target/usr failed"
        mount -o remount,rw /target/persistent/ostree /target/persistent/ostree || error "remount /target/persistent/ostree failed"

        # 没有boot分区
        local boot_device=$(installer_get "DI_BOOT_DEVICE_CONFIG")
        if [ -z "$boot_device" ]; then
            mkdir -p "/target/boot" || error "mkdir /target/boot failed"
        fi
    fi
}

function mount_partitions() {
    local mount_points
    mount_points=$(installer_get "DI_MOUNTS_POINTS")
    #mount_points="/dev/nvme0n1p3=/;/dev/nvme0n1p2=/boot;/dev/nvme0n1p1=/boot/efi"
    mount_points=(${mount_points//;/ })
    declare -A mount_map
    for p in "${mount_points[@]}"
    do
        kv=(${p//=/ })
        if [[ "${kv[1]}" != "swap" ]];then
            mount_map["${kv[1]}"]="${kv[0]}"
        fi
    done

    keys=("${!mount_map[@]}")
    sorted_keys=($(printf "%s\n" "${keys[@]}" | sort))
    for key in "${sorted_keys[@]}"; do
        local part="${mount_map[$key]}"
        if ! findmnt $part; then
            mkdir -p "/target/$key" || error "Failed to create mount point /target/$key"
            mount "${mount_map[$key]}" "/target/$key" || error "Failed to mount ${mount_map[$key]} /target/$key"
            echo "mount ${mount_map[$key]} /target$key"
        fi
    done
}

# 删除启动项1，保留启动项0
function delete_boot_1() {
    local ext_commit=$(chroot /target deepin-immutable-ctl admin status | grep ExtCommitIdIdx | head -n 1 | awk '{print $2}')
    echo "ext_commit=$ext_commit"
    local base_commit=$(chroot /target deepin-immutable-ctl admin status | grep BaseCommitIdIdx | head -n 1 | awk '{print $2}')
    echo "base_commit=$base_commit"

    #umount
    umount -lfR /target || error "umount target failed"

    #mount root
    local root_device=$(installer_get DI_ROOT_DEVICE_CONFIG)
    mount $root_device /target
    mount --make-private /target

    # mount data
    local data_device=$(installer_get DI_DATA_DEVICE_CONFIG)
    if [ -n "$data_device" ]; then
        mount $data_device /target/persistent
        mount --make-private /target/persistent
    fi

    ostree_ext_mount /ostree/deploy/deepin/deploy/$base_commit  ostree/data/$ext_commit/checkout 

    mount_partitions
    local boot_loader=$(installer_get "DI_BOOTLOADER")
    if [ -n "$boot_loader" ]; then
        mkdir -vp /target/boot/efi
        mount $boot_loader /target/boot/efi || error "mount $boot_loader /target/boot/efi failed"
    fi
    setup_mountpoint
    local root_device_uuid=$(blkid -o value -s UUID $root_device)
    info "root_device_uuid=$root_device_uuid"
    chroot /target bash -c "GRUB_DEVICE=UUID=$root_device_uuid DDE_DEBUG_LEVEL=debug deepin-immutable-ctl admin undeploy 1 -v" || error "Failed to undeploy 1"

    chroot /target deepin-immutable-ctl admin status
}

function add_persistent_conf()
{
    local data_device=$(installer_get DI_DATA_DEVICE_CONFIG)
    if [ -n "$data_device" ]; then
        mkdir -p /target/usr/share/initramfs-tools/conf.d/
        local data_device_uuid=$(blkid -s UUID -o value $data_device)
        echo "PERSISTENT=UUID=$data_device_uuid" > /target/usr/share/initramfs-tools/conf.d/persistent_uuid.conf
    fi
}

function deploy_persistent()
{
    printf 'sysroot-ro\x00\x00\x00\x00\x00\x00\x01\x00\x62\x0b\x14' > /target/run/ostree-booted
    add_persistent_conf
    chroot /target /usr/sbin/update-initramfs -k all -u
    # 临时解决deploy失败问题，磐石版本更新后去掉
    rm -f /target/etc/grub.d/15_ostree || true

    local ext_commitId=$(ostree --repo=/target/persistent/ostree/repo rev-parse deb-ostree/main)
    if [[ "x$(installer_get DI_IS_GHOST_MODE)" == "xtrue" ]]; then
        ext_commitId=$(cat /uimg/localhost/persistent/ostree/data/status.list | head -1 | awk  -F. '{print $1}')
    fi
    info "ext_commitId=$ext_commitId"
    echo $ext_commitId.0 > /target/persistent/ostree/data/status.list

    local root_device=$(installer_get DI_ROOT_DEVICE_CONFIG)
    local root_device_uuid=$(blkid -o value -s UUID $root_device)
    info "root_device_uuid=$root_device_uuid"
    local language=$(installer_get "DI_LOCALE")
    chroot /target bash -c "LANGUAGE=$language GRUB_DEVICE=UUID=$root_device_uuid DDE_DEBUG_LEVEL=debug deepin-immutable-ctl admin deploy -v"
}

# 系统安装完成并完成所有配置后，执行一次系统备份增加一个回滚启动项
function first_backup()
{
    local language=$(installer_get "DI_LOCALE")
    
    if [ -z "$language" ]; then
        warning "DI_LOCALE is empty"
        language="zh_CN"
    fi
    
    local result=1
    bash -c "LANGUAGE=$language deepin-immutable-ctl admin deploy --backup" && result=0
    if [ $result -eq 0 ]; then
        info "first backup ok"
        # 设置语言环境变量并deploy一次, deploy时会执行update-grub
        bash -c "LANGUAGE=$language LANG=${language}.UTF-8 deepin-immutable-ctl admin deploy --finalize" || error "first finalize failed"
    else
        warning "first backup failed"
    fi
}
