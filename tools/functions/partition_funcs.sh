#!/bin/bash

parted_manager() {
    local parted_type=$(installer_get "DI_PARTITION_TYPE")
    local parted_conf=$(installer_get "DI_PARTITION_CONFIG")
    deepin-installer-parted -m $parted_type -c $parted_conf   # 分区管理
}

parted_mount() {
    local mount_points=$(installer_get "DI_MOUNTS_POINTS")
    if ! is_ostree_system; then
        deepin-installer-parted -m 4 -c $mount_points   # 分区挂载
        bind_data_mount # 处理data分区内部目录的bind
        setup_data_info
        setup_efi_parted
    else
        # 此处只需要挂载root和和swap即可，其他分区需要等ostree加载完成后再进行挂载。
        mount_points=(${mount_points//;/ })
        for p in "${mount_points[@]}"
        do
            kv=(${p//=/ })
            if [ "${kv[1]}" == "/" ]; then
                [ -d /target ] || mkdir /target
                mount "${kv[0]}" "/target" || error "mount ${kv[0]} to /target failed !"
            fi
        done

        # 处理保留用户数据场景数据清理
        delete_data_without_home
    fi

    setup_swap # 处理swap分区
}

parted_umount() {
    if mount |grep /target; then
        echo "====umount /target===="
        umount -Rlf /target
    fi

    swapoff -a
    rm /target -fr
}

# 调整文件系统缩小32M
shrink_filesystem() {
    local mount_points=$(installer_get "DI_MOUNTS_POINTS")
    mount_points=(${mount_points//;/ })

    local resizeStep=35 # 单位是Mib

    for p in "${mount_points[@]}"; do
        kv=(${p//=/ })
        local device="${kv[0]}"
        local mount_point="${kv[1]}"

        # 获取文件系统类型
        local fstype=$(lsblk -no FSTYPE "${device}")

        # 检查文件系统类型并缩小文件系统
        if [[ "${mount_point}" != "/boot/efi" && "${mount_point}" != "/boot" ]]; then
            case "${fstype}" in
                ext4|ext3)
                    # 使用tune2fs -l $device，查询Block count和Block size，计算文件系统总大小
                    local blockCount=$(tune2fs -l ${device} | grep "Block count" | awk '{print $NF}')
                    local blockSize=$(tune2fs -l ${device} | grep "Block size" | awk '{print $NF}')

                    # 计算文件系统缩小后的大小
                    local resizeValue=$(echo "(${blockCount} * ${blockSize} - ${resizeStep} * 1024 * 1024) / ${blockSize}" | bc)

                    # 执行调整之前需要先e2fsck检查一下文件系统
                    info "e2fsck -fy ${device}"
                    e2fsck -fy ${device}

                    # 执行调整
                    info "Resizing ${fstype} filesystem on ${device}: resize2fs "${device}" ${resizeValue}"
                    resize2fs "${device}" ${resizeValue}
                    ;;
                btrfs)
                    # 执行调整之前需要先btrfs check检查一下文件系统
                    info "btrfs check ${device}"
                    btrfs check ${device}

                    # 挂载分区，获取文件系统总大小
                    local mountPath="/testuos"
                    mkdir -p ${mountPath}
                    mount ${device} ${mountPath}
                    local fileSystemSize=$(df -B 1 ${mountPath} | sed -n 2p | grep "${mountPath}" | awk '{print $2}')

                    # 计算文件系统缩小后的大小
                    local resizeValue=$(echo "(${fileSystemSize} - ${resizeStep} * 1024 * 1024) / 1" | bc)

                    # 执行调整
                    info "Resizing btrfs filesystem on ${device}: btrfs filesystem resize ${resizeValue} ${mountPath}"
                    btrfs filesystem resize ${resizeValue} "${mountPath}"

                    umount ${mountPath}
                    rm -rf ${mountPath}
                    ;;
                *)
                    info "Skipping $device with filesystem type $fstype"
                    ;;
            esac
        else
            info "Skipping $device mounted on $mount_point"
        fi
    done
}

clear_active_part() {
    # 读取分区配置文件中/etc/deepin-installer/partition_policy.json的设备信息，根据设备信息遍历找出加密卷信息，对其进行卸载、vg反激活、关闭加密卷操作
    info "clear_active_part umount points"
    local device_array=($(echo -n "$(cat /etc/deepin-installer/partition_policy.json | grep -w 'device' | tr -d ',' | awk '{print $NF}' | tr -d '\"' | sort -u)"))
    for deviceItem in ${device_array[@]}; do
        # 清理设备下的挂载信息
        local mountArray=($(echo -n "$(lsblk ${deviceItem} -lpno MOUNTPOINT | awk '{print $1}')"))
        while [ ${#mountArray[@]} -gt 1 ]; do
            for mountItem in ${mountArray[@]}; do
                if [ -n "$mountItem" ]; then
                    info "umount -Rlf ${mountItem}"
                    umount -Rlf "${mountItem}" || warning "umount -Rlf ${mountItem} failed !"
                fi
            done
            mountArray=($(echo -n "$(lsblk ${deviceItem} -lpno MOUNTPOINT | awk '{print $1}')"))
        done

        # 反激活设备下的vg信息
        info "clear_active_part vgchange inactive"
        local vgArray=($(echo -n "$(vgdisplay | grep "VG Name" | awk '{print $NF}')"))
        for vgItem in ${vgArray[@]}; do
            if [ -n "${vgItem}" ]; then
                # 判断设备中包含对应的vg
                local devInVg=$(echo -n "$(lsblk ${deviceItem} -npl | grep ${vgItem} || echo \"\")")
                if [ -n "${devInVg}" ]; then
                    info "vgchange -an ${vgItem}"
                    vgchange -an ${vgItem} || warning "vgchange -an ${vgItem} failed !"
                fi
            fi
        done

        # 关闭加密卷
        info "clear_active_part close crypt parts"
        local cryptArray=($(echo -n "$(lsblk ${deviceItem} -plfno NAME,TYPE | grep crypt$ | awk '{print $1}')"))
        for cryptItem in ${cryptArray[@]}; do
            info "cryptsetup luksClose ${cryptItem}"
            cryptsetup luksClose ${cryptItem} || warning "cryptsetup luksClose ${cryptItem} failed !"
        done

    done
}

setup_esp_parted() {
    local part=$1
    local dev=$(part_to_device $1)
    local num=$(part_num $1)
    parted -s "$dev" set "$num" esp on
}

setup_efi_parted() {
    if is_uefi; then

        local start_dev=$(installer_get "DI_START_DEV")
        if [ -z "${start_dev}" ]; then
            start_dev="null"
        fi
        local efi_part=$(lsblk -lf -o NAME,MOUNTPOINT | grep -E "/target/boot/efi$" | awk '{print $1}')
        local old_efi_part=$(fdisk -l -o Device,Type | grep EFI | awk '{print $1}' | sed -n '1p')
        if [ -z "$old_efi_part" ]; then
            # 如果fdisk没有找到efi分区，可能是EFI分区的type没有被指定，尝试使用lsblk按label和文件系统类型查找
            old_efi_part=$(lsblk -lfp -o NAME,FSTYPE,LABEL,TRAN | grep -v "${start_dev}" | grep "vfat" | grep "EFI" | awk '{print $1}' | sed -n '1p')
        fi
        local roota_part=$(lsblk -lf -o NAME,MOUNTPOINT | grep -E "/target$" | awk '{print $1}')

        if [ -n "$efi_part" ]; then
            setup_esp_parted "/dev/$efi_part"
            installer_set DI_BOOTLOADER "/dev/$efi_part"

        elif [ -n "$old_efi_part" ]; then
            setup_esp_parted "$old_efi_part"
            # 判断esp分区是否有被挂载
            local mp=$(lsblk -nlf $old_efi_part -o MOUNTPOINT)
            if [ ! -n "$mp" ];then
                # 复用磁盘中的efi分区
                mkdir -vp /target/boot/efi
                mount "$old_efi_part" /target/boot/efi
                installer_set DI_BOOTLOADER "$old_efi_part"
            fi
        fi
    fi
}

lv_to_part() {
    local in=$1
    echo -n "$in" | sed 's/-/\//g'
}

setup_swap() {
    # 如果当前用户没有新建swap分区
    local new_swap=$(swapon | sed -n "2p" | awk '{print $1}')
    if [ ! -n "$new_swap" ];then
        # 如果系统内有已存在的swap分区，则复用该分区，除了lvm中的swap分区
        local old_swap=$(lsblk -lfno PATH,FSTYPE,TYPE,MOUNTPOINT | grep "part" | grep "swap" | grep -v "SWAP"| awk '{print $1}')
        info "old_swap=$old_swap"
        if [ -n "$old_swap" ];then
            swapon $(lv_to_part "$old_swap")
        fi
    fi
}

exists_data_part() {
    local mount_points=$(installer_get "DI_MOUNTS_POINTS")
    local data_mount_points=$(installer_get "DI_DATA_MOUNT_POINT_CONFIG" "${DI_OS_VERSION_ENV}")
    echo "$mount_points" | grep -wq "$data_mount_points"
    return $?
}

exist_home_part() {
    local mount_points=$(installer_get "DI_MOUNTS_POINTS")
    echo "$mount_points" | grep -wq "/home"
    return $?
}

bind_data_mount() {
    if exists_data_part; then
        local data_mount_points=$(installer_get "DI_DATA_MOUNT_POINT_CONFIG" "${DI_OS_VERSION_ENV}")

        mkdir -p /target/${data_mount_points}/home
        mkdir -p /target/home
        mount --bind /target/${data_mount_points}/home /target/home || error "Faild to mount /target/home"

        mkdir -p /target/${data_mount_points}/opt
        mkdir -p /target/opt
        mount --bind /target/${data_mount_points}/opt /target/opt || error "Faild to mount /target/opt"

        mkdir -p /target/root
        mkdir -p /target/${data_mount_points}/root
        mount --bind /target/${data_mount_points}/root /target/root || error "Faild to mount /target/root"

        mkdir -p /target/var
        mkdir -p /target/${data_mount_points}/var
        mount --bind /target/${data_mount_points}/var /target/var || error "Failed to mount /target/var"
    fi
}

clean_data() {
    local in=$1
    local ignore=$2

    ls -a $in | grep -Ev $ignore | xargs -I {} chattr -iR ${in}/{} || true
    ls -a $in | grep -Ev $ignore | xargs -I {} rm -vfr "${in}/{}" || true
}

setup_data_info() {
    if exists_data_part; then
        local data_mount_points=$(installer_get "DI_DATA_MOUNT_POINT_CONFIG" "${DI_OS_VERSION_ENV}")

        find /target/home -depth | while read -r file; do                                                                                                                                                                                     
            chown -h "root:root" "${file}" || warning "chown -h root:root ${file} failed!"
        done
        [ -d /target/${data_mount_points} ] && clean_data /target/${data_mount_points} '^\.\.$|^\.$|^var$|^home$|^root$|^opt$|^lost\+found$'
        [ -d /target/root ] && clean_data /target/root '^\.\.$|^\.$|^lost\+found$'
        [ -d /target/var ] && clean_data /target/var '^\.\.$|^\.$|^lost\+found$'
        [ -d /target/opt ] && clean_data /target/opt '^\.\.$|^\.$|^lost\+found$'
        # 删除原来用户的密钥环，防止弹出密钥环验证提示. 系统版本不同keyrings的路径不一样。全部删除
        ls /target/home | xargs -I {} rm -vrf "/target/home/{}/.local/share/keyrings"
        ls /target/home | xargs -I {} rm -vrf "/target/home/{}/.local/share/deepin-keyrings-wb"
        # 创建dpkg-db的链接
        mkdir -p /target/usr/lib/dpkg-db
        mkdir -p /target/var/lib
        ln -sf ../../usr/lib/dpkg-db /target/var/lib/dpkg
    fi
}

generate_fstab() {
    # 生成fstab
    readonly GENFSTAB="$DI_INSTALL_TOOLS_DIR_ENV/scripts/genfstab.sh"
    umount -lv /target/$DI_LIVE_DIR_ENV
    bash "${GENFSTAB}" -p -U -r / /target > /target/etc/fstab
    mount -v --bind ${DI_LIVE_DIR_ENV} /target/$DI_LIVE_DIR_ENV || error "Failed to bind ${DI_LIVE_DIR_ENV}"

    # 处理swap分区的挂载
    lsblk -f -o FSTYPE,UUID |
    while read -r type uuid; do
        if [ "x$type" = "xswap" ] && [ $(grep -c swap /target/etc/fstab) -eq 0 ]; then
            printf '%s\t%-10s\t%-10s\t%-10s\t0 0\n\n' "UUID=${uuid}" 'none' 'swap' 'defaults,pri=-2' >> /target/etc/fstab
        fi
    done

    local data_mount_points=$(installer_get "DI_DATA_MOUNT_POINT_CONFIG" "${DI_OS_VERSION_ENV}")
    if ! is_ostree_system; then
        if exists_data_part; then
            sed -i '$!N;/\n.*opt/!P;D' /target/etc/fstab
            sed -i '/opt/d' /target/etc/fstab

            sed -i '$!N;/\n.*var/!P;D' /target/etc/fstab
            sed -i '/var/d' /target/etc/fstab

            echo "${data_mount_points}/opt /opt none defaults,bind 0 0" >> /target/etc/fstab
            echo "${data_mount_points}/var /var none defaults,bind 0 0" >> /target/etc/fstab
        fi
    else
        if exists_data_part  && ! exist_home_part; then
            mkdir -pv /target/${data_mount_points}/home
            mkdir -pv /target/${data_mount_points}/root
            echo "${data_mount_points}/home /home none defaults,bind 0 0" >> /target/etc/fstab
            echo "${data_mount_points}/root /root none defaults,bind 0 0" >> /target/etc/fstab
        fi
    fi
}

setup_cryttab() {
    local autoDecrypt=$(installer_get "DI_IS_AUTO_DECRYPT")
    if [ "x$autoDecrypt" = "xtrue" ]; then
        setup_tpm_decrypt
    else
        setup_multidisk_one_key_decrypt
    fi

    generate_cryttab
    cat /target/etc/crypttab
}

generate_cryttab() {
    info "generate_cryttab start..."
    local file="/target/etc/crypttab"
    local luks_parts=$(installer_get "DI_CRYPT_DEVICE")
    local CRYPT_INFO
    for lp in $luks_parts;do
        local part="$lp"
        local luks_name=${part##*/}
        local crypt_part=$(cryptsetup status ${part} | grep "device" | awk '{print $2}')
        local partMountPoint=$(lsblk "${part}" -lnpfo PATH,MOUNTPOINT | awk '{print $2}')
        local luks_uuid=""
        if [ -n "${crypt_part}" ]; then
            luks_uuid=$(lsblk -lpfo UUID,PATH | grep "${crypt_part}" | awk '{print $1}')
        fi

        if [ -n "${luks_uuid}" ] && [ -n "${partMountPoint}" ]; then
            local autoDecrypt=$(installer_get "DI_IS_AUTO_DECRYPT")

            # 如果设置了自动解密，则设置tpm2-tpm2-device=auto
            local tpm2_auto=""
            if [ "x$autoDecrypt" = "xtrue" ]; then
                tpm2_auto="tpm2-device=auto,"
            fi

            # 如果是全盘或者手动分区根分区加密，则设置initramfs
            local use_initramfs=""
            if is_full_disk_mode; then
                use_initramfs="initramfs,"
            elif [ -n "$(echo -n \"$(lsblk ${crypt_part} -no LABEL | grep 'Roota')\" | tr -d ' ' | tr -d '"')" ]; then
                use_initramfs="initramfs,"
            fi

            # 更新crypttab
            echo "${luks_name} UUID=${luks_uuid}  none luks,${use_initramfs}${tpm2_auto}keyscript=/lib/usec-crypt-kit/usec-askpass" >> "$file"
        fi
        [[ -n ${CRYPT_INFO} ]] && CRYPT_INFO+=";"
        CRYPT_INFO+="$crypt_part:$luks_name"
    done
    deepin-installer-config set /target"${DI_CONFIG_FILE_ENV}" "DI_CRYPT_INFO" "${CRYPT_INFO}"

    local useHideCryptHeader=$(installer_get "DI_USE_HIDE_CRYPT_HEADER")
    if [ "x${useHideCryptHeader}" = "xtrue" ]; then
        # 添加隐藏crypt header lvm分区到/etc/crypttab
        local lvmPartNames=$(lsblk -lno NAME,TYPE,UUID | grep " lvm " | grep "usec-" | awk '{print $1}' | tr -s '\n' ' ' | tr -s '--' '-')
        if [ -n "${lvmPartNames}" ]; then
            # 获取相关设备里的所有partuuid
            local deviceList=$(installer_get "DI_DEVICE_LIST")
            deviceList=$(echo ${deviceList} | tr -s ';' '|')
            local allPartUuids=$(lsblk -lpno NAME,PARTUUID | grep -E "${deviceList}" | awk '{print $NF}' | tr -s '\n' ' ')

            for nameItem in ${lvmPartNames[@]};do
                local lvmPartUuid=${nameItem##*lv-}
                local lvmVolumeName=$(echo lv${lvmPartUuid} | tr -d '-')
                # 排除root的partuuid和没有被涉及的磁盘上的分区的partuuid
                local rootPartUuid=$(lsblk -lno PARTUUID,MOUNTPOINT | grep -E "/target$" | awk '{print $1}')
                if [ "${rootPartUuid}" != "${lvmPartUuid}" ] && [[ "${allPartUuids}" =~ "${lvmPartUuid}" ]]; then
                    echo "${lvmVolumeName} PARTUUID=${lvmPartUuid} none luks"
                    echo "${lvmVolumeName} PARTUUID=${lvmPartUuid} none luks" >> "$file"
                fi
            done
        fi
    fi


    info "generate_cryttab end!"
}

setup_tpm_decrypt() {
    info "setup_tpm_decrypt start..."

    # 获取支持的tpm2算法列表
    local tpm2_algs=$(installer_get "DI_TPM_ALGS")
    local tpm2_pcr_banks=$(installer_get "DI_TPM_PCR_BANCKS")
    echo "tpm2_algs is : ${tpm2_algs}"
    echo "tpm2_pcr_banks is : ${tpm2_pcr_banks}"

    local tpm2Alg=""
    local primaryKeyAlg="rsa"
    local primaryHashAlg=""
    local pcr="23"
    local pcrBank=""
    local pinKey=""
    local crypt_password=$(installer_get "DI_CRYPT_PASSWORD")
    crypt_password=`echo -n "$crypt_password" | base64 -d`

    # 使用aes算法
    if [[ "${tpm2_algs}" =~ "aes:1" ]]; then
        tpm2Alg="aes"
    # 使用国密算法
    elif [[ "${tpm2_algs}" =~ "sm4:1" ]]; then
        tpm2Alg="sm4"
        primaryKeyAlg="sm4"
    else
        tpm2Alg=${tpm2_algs%%:*}
        tpm2Alg=$(echo -n "${tpm2Alg}" | tr -d ' ')
    fi

    # 设置pcrbank
    if [[ "${tpm2_pcr_banks}" =~ "sha256:1" ]]; then
        primaryHashAlg="sha256"
        pcrBank="sha256"
    elif [[ "${tpm2_pcr_banks}" =~ "sm3_256:1" ]]; then
        primaryHashAlg="sm3_256"
        pcrBank="sm3_256"
    else
        pcrBank=${tpm2_pcr_banks%%:*}
        pcrBank=$(echo -n "${pcrBank}" | tr -d ' ')
        primaryHashAlg=${pcrBank}
    fi

    echo "${crypt_password}" > secret.dat
    openssl rand  -out iv.bin 16

    tpm2-abrmd --allow-root > /dev/null 2>&1 || true &

    # 清除tpm2保存的认证信息，双系统加密情况下会清除之前安装系统设置的认证信息
    # tpm2_clear -c p || echo "[$(date '+%Y-%m-%d %H:%M:%S')][info] tpm2_clear -c p failed !!"
    # 清除tpm2的lockout状态
    tpm2_dictionarylockout -c || echo "[$(date '+%Y-%m-%d %H:%M:%S')][info] tpm2_dictionarylockout -c failed !"
    tpm2_startauthsession -S session.dat -g ${primaryHashAlg} -G ${tpm2Alg}
    tpm2_pcrread ${pcrBank}:${pcr} -o pcr_val.bin
    tpm2_policypcr -S session.dat -l ${pcrBank}:${pcr}  -L policy.dat -f pcr_val.bin

    # 如果pinkey不为空则做策略
    if [ -n "$pinKey" ]; then
        info "use pinkey, now set policy"
        tpm2_policypassword -S session.dat -L policy.dat
        tpm2_flushcontext session.dat
    fi

    tpm2_createprimary -C o -g ${primaryHashAlg} -G ${primaryKeyAlg} -c primary.ctx
    if [ -n "$pinKey" ]; then
        tpm2_create -g ${primaryHashAlg} -G ${tpm2Alg} -u key.pub -r key.priv -C primary.ctx -L policy.dat -p "${pinKey}"
    else
        tpm2_create -g ${primaryHashAlg} -G ${tpm2Alg} -u key.pub -r key.priv -C primary.ctx -L policy.dat
    fi
    tpm2_load -C primary.ctx -u key.pub -r key.priv -n key.name -c key.ctx

    tpm2_startauthsession --policy-session -S session.dat -g ${primaryHashAlg} -G ${tpm2Alg}
    tpm2_pcrread ${pcrBank}:${pcr} -o pcr_val.bin
    tpm2_policypcr -S session.dat -l ${pcrBank}:${pcr} -f pcr_val.bin

    if [ -n "$pinKey" ]; then
        tpm2_policypassword -S session.dat -L policy.dat
        tpm2_encryptdecrypt -Q --iv iv.bin -c key.ctx -o cipher.out secret.dat -p"session:session.dat+${pinKey}"
    else
        tpm2_encryptdecrypt -Q --iv iv.bin -c key.ctx -o cipher.out secret.dat -p"session:session.dat"
    fi
    tpm2_flushcontext session.dat

    echo "$(ps -aux | grep 'tpm2-abrmd --allow-root$' | awk '{print $2}')" | xargs -I {} kill -9 {} > /dev/null 2>&1 || true

    # token json
    local token_type="usec-tpm2"
    local token_pin="0"
    local token_keySlots="[\"7\"]"
    local token_keyPriv="$(cat key.priv | base64 -w 0)"
    local token_keyPub="$(cat key.pub | base64 -w 0)"
    local token_keyAlg="${primaryKeyAlg}"
    local token_hashAlg="${primaryHashAlg}"
    local token_iv="$(cat iv.bin | base64 -w 0)"
    local token_enc="$(cat cipher.out | base64 -w 0)"
    local token_pcr="${pcr}"
    local token_bank="${pcrBank}"
    local token_sessionKeyAlg=""
    local token_sessionHashAlg=""

cat > /tmp/token.json << EOF
{
    "type":"${token_type}",
    "pin":"${token_pin}",
    "keyslots":${token_keySlots},
    "kek-priv":"${token_keyPriv}",
    "kek-pub":"${token_keyPub}",
    "primary-key-alg":"${token_keyAlg}",
    "primary-hash-alg":"${token_hashAlg}",
    "session-key-alg":"${tpm2Alg}",
    "session-hash-alg":"${token_hashAlg}",
    "iv":"${token_iv}",
    "enc":"${token_enc}",
    "pcr":"${token_pcr}",
    "pcr-bank":"${token_bank}"
}
EOF

    local cryptParts=$(installer_get "DI_CRYPT_DEVICE")
    for lp in ${cryptParts};do
        local part="$lp"
        local cryptPart=$(cryptsetup status ${part} | grep "device" | awk '{print $2}')
        if [ -n "${cryptPart}" ]; then
            echo "cryptsetup token remove \"${cryptPart}\" --token-id 0"
            cryptsetup token remove "${cryptPart}" --token-id 0 || true
            echo "cryptsetup token import ${cryptPart} --token-id 0 --json-file=/tmp/token.json"
            cryptsetup token import "${cryptPart}" --token-id 0 --json-file=/tmp/token.json
        fi
    done

    info "setup_tpm_decrypt end!"
}

setup_kefile_decrypt() {
    info "setup_kefile_decrypt start..."
    local crypt_password=$(installer_get "DI_CRYPT_PASSWORD")
    local crypt_keyfile=$(installer_get "DI_DISK_KEY_FILE_CONFIG")
    mkdir -vp $(get_file_path "${crypt_keyfile}")
    echo -n "$crypt_password" | base64 -d > ${crypt_keyfile}
    info "setup_kefile_decrypt end!"
}

fstab_crypt_part_option() {
    local fstabPath="/target/etc/fstab"
    local cryptParts=$(installer_get "DI_CRYPT_DEVICE")
    local partUuids=""

    for lp in ${cryptParts};do
        local cryptPart="$lp"
        partUuids=$(lsblk -lnpo UUID,FSTYPE "${cryptPart}" | grep -vE 'LVM2_member|crypto_LUKS' | awk '{print $1}')
    done

    local useHideCryptHeader=$(installer_get "DI_USE_HIDE_CRYPT_HEADER")
    if [ "x${useHideCryptHeader}" = "xtrue" ]; then
        # 考虑usec-vg中设置的分区的uuid，根据lvm名称中的partuuid找到对应分区的uuid，合并到partUuids中
        local lvmPartNames=$(lsblk -lno NAME,TYPE,UUID | grep " lvm " | grep "usec-" | awk '{print $1}' | tr -s '\n' ' ' | tr -s '--' '-')
        if [ -n "${lvmPartNames}" ]; then
            for nameItem in ${lvmPartNames[@]};do
                local lvmToPartUuid=${nameItem##*lv-}
                local partToUuid=$(lsblk -lno UUID,PARTUUID | grep "${lvmToPartUuid}" | awk '{print $1}')
                if [ -n "${partToUuid}" ]; then
                    partUuids="${partUuids} ${partToUuid}"
                fi
            done
        fi
    fi

    for lpUuid in ${partUuids};do
        local testUuid="${lpUuid}"
        local fstabLine=$(cat ${fstabPath} | grep "${testUuid}" || echo "")

        if [ -z "${fstabLine}" ]; then
            continue
        fi

        local fstab4Colum=$(cat ${fstabPath} | grep "${testUuid}" | awk '{print $4}')
        local fstabLineNew=$(echo "${fstabLine}" | sed 's/'${fstab4Colum}'/'${fstab4Colum},x-systemd.device-timeout=0'/g' || echo "")

        if [ -z "${fstabLineNew}" ]; then
            warning "sed ${fstab4Colum} to ${fstab4Colum},x-systemd.device-timeout=0 with '/' failed, now try '|'"
            fstabLineNew=$(echo "${fstabLine}" | sed 's|'${fstab4Colum}'|'${fstab4Colum},x-systemd.device-timeout=0'|g' || echo "")
        fi

        if [ -n "${fstabLineNew}" ]; then
            local fstabLineNum=$(grep -rn "${testUuid}" "${fstabPath}")
            fstabLineNum=${fstabLineNum%%:*}
            sed -i '/'"${testUuid}"'/d' ${fstabPath}
            sed -i "${fstabLineNum}i\\${fstabLineNew}" ${fstabPath}
        else
            warning "nothing to do with ${testUuid}"
        fi
    done
}

setup_fstab() {
    generate_fstab
    fstab_crypt_part_option
    setup_cryttab
}

# 修复/target下特殊目录的权限
# 手动分区安装时，如果/tmp被挂载为独立分区，新文件系统根目录的默认权限为0755，
# 而Linux FHS规范要求/tmp权限为1777（所有用户可读写 + sticky bit）。
# 全盘安装时squashfs镜像中已预设正确权限，此处统一修正以确保一致性。
fix_target_tmp_permissions() {
    if [ -d /target/tmp ]; then
        chmod 1777 /target/tmp
        info "fix /target/tmp permissions to 1777"
    fi
}

## 判断是否全盘安装模式
is_full_disk_mode() {
    #FullDiskMode = 0, 全盘安装
    #AdvancedMode, 高级安装
    #Cryptsetup, 全盘加密
    #SaveData, 保留用户数据
    #MountMode = 4, 挂载分区
    #GhostMode, ghost安装
    #CustomMode, 自定义安装
    #LvmFullDisk 全盘lvm安装

    local parted_type=$(installer_get "DI_PARTITION_TYPE")
    local install_mode="02367"
    if [[ "${install_mode}" =~ ${parted_type} ]]; then
        return 0
    else
        return 1
    fi
}

setup_udisks_rules() {
    local UDISK_FILE="/target/etc/udev/rules.d/80-udisks-installer.rules"
    local usedUuids=""
    # 隐藏efi分区
    local EFI_ID=$(lsblk -o UUID,MOUNTPOINT -f | grep -i /target/boot/efi$ | awk '{print $1}')
    local EFI_PART_PATH=$(lsblk -o PATH,MOUNTPOINT -f | grep -i /target/boot/efi$ | awk '{print $1}')
    if [ -n "$EFI_ID" ]; then
        echo "# hide ${EFI_PART_PATH} efi" >> ${UDISK_FILE}
        echo "ENV{ID_FS_UUID}==\"${EFI_ID}\", ENV{UDISKS_IGNORE}=\"1\"" >> ${UDISK_FILE}
        usedUuids="${EFI_ID}"
    fi

    # 隐藏boot分区
    local BOOT_ID=$(lsblk -o UUID,MOUNTPOINT -f | grep -i /target/boot$ | awk '{print $1}')
    local BOOT_PART_PATH=$(lsblk -o PATH,MOUNTPOINT -f | grep -i /target/boot$ | awk '{print $1}')
    if [ -n "$BOOT_ID" ]; then
        echo "# hide ${BOOT_PART_PATH} boot" >> ${UDISK_FILE}
        echo "ENV{ID_FS_UUID}==\"${BOOT_ID}\", ENV{UDISKS_IGNORE}=\"1\"" >> ${UDISK_FILE}
        usedUuids="${usedUuids};${BOOT_ID}"
    fi

    # persistent和home分区同时存在时隐藏persistent分区
    if exists_data_part  && exist_home_part; then
        local data_device=$(installer_get DI_DATA_DEVICE_CONFIG)
        if [ -n "$data_device" ]; then
            local data_device_uuid=$(blkid -s UUID -o value $data_device)
            echo "# hide ${data_device} persistent" >> ${UDISK_FILE}
            echo "ENV{ID_FS_UUID}==\"${data_device_uuid}\", ENV{UDISKS_IGNORE}=\"1\"" >> ${UDISK_FILE}
        fi
    fi

    # 隐藏swap分区
    echo "# hide swap" >> ${UDISK_FILE}
    echo "ENV{ID_FS_TYPE}==\"swap\", ENV{UDISKS_IGNORE}=\"1\"" >> ${UDISK_FILE}

    # 隐藏使用到的加密分区
    if is_full_disk_mode; then
        local luks_parts=$(installer_get "DI_CRYPT_DEVICE")
        for lp in $luks_parts;do
            local part="$lp"
            local luks_name=${part##*/}
            local crypt_part=$(cryptsetup status ${part} | grep "device" | awk '{print $2}')
            local luks_uuid=""
            if [ -n "${crypt_part}" ]; then
                luks_uuid=$(lsblk -lpfo UUID,PATH | grep "${crypt_part}" | awk '{print $1}')
                if [ -n "${luks_uuid}" ]; then
                    echo "# hide ${part}" >> ${UDISK_FILE}
                    echo "ENV{ID_FS_UUID}==\"${luks_uuid}\", ENV{UDISKS_IGNORE}=\"1\"" >> ${UDISK_FILE}
                    usedUuids="${usedUuids};${luks_uuid}"
                fi
            fi
        done
    fi

    local useHideCryptHeader=$(installer_get "DI_USE_HIDE_CRYPT_HEADER")
    if [ "x${useHideCryptHeader}" = "xtrue" ]; then
        # 隐藏crypt header lvm分区
        echo "# hide all crypt header part"
        echo "ENV{DM_VG_NAME}==\"usec-vg-*\", ENV{DM_LV_NAME}==\"lv-*\", ENV{UDISKS_IGNORE}=\"1\""
        echo "ENV{DM_VG_NAME}==\"usec-vg-*\", ENV{DM_LV_NAME}==\"lv-*\", ENV{UDISKS_IGNORE}=\"1\"" >> ${UDISK_FILE}
    fi
}

# 保留用户数据安装场景，清理/persistent挂载点对应分区下除home目录之外的其他数据
delete_data_without_home() {
    local partition_type=$(installer_get "DI_PARTITION_TYPE")

    if [ ${partition_type} -eq 3 ]; then
        local mount_points=$(installer_get "DI_MOUNTS_POINTS")
        mount_points=(${mount_points//;/ })
        local data_device=""
        for p in "${mount_points[@]}"
        do
            kv=(${p//=/ })
            if [ "${kv[1]}" == "/persistent" ];then
                data_device="${kv[0]}"
                break
            fi
        done

        # 清理/persistent挂载点对应分区下的除home目录之外的其他目录
        if [ -n "${data_device}" ]; then
            local work_dir="/saveHomeDir"
            [ -d "${work_dir}" ] || mkdir "${work_dir}"

            mount "${data_device}" "${work_dir}" || error "mount ${data_device} to ${work_dir} failed !"

            # 清理除home目录之外的其他目录
            clean_data ${work_dir} '^\.\.$|^\.$|^home$|^lost\+found$'
            sync

            umount "${work_dir}" || error "umount ${work_dir} failed !"
        fi
    fi
}

set_boot_readonly()
{
    local boot_device
    boot_device=$(installer_get "DI_BOOT_DEVICE_CONFIG")
    if [ -n "$boot_device" ]; then
        local boot_uuid
        boot_uuid=$(blkid -o value -s UUID "$boot_device")
        info "boot_uuid=${boot_uuid}"
        local old
        old=$(cat /target/etc/fstab |grep UUID="${boot_uuid}")
        local new
        new=$(echo "$old" | sed 's/rw/ro/')
        sed -i "s|^$old$|$new|" /target/etc/fstab

    fi
}

# 设置多磁盘一键解密tokens
setup_multidisk_one_key_decrypt()
{
    info "setup_tpm_dsetup_multidisk_one_key_decryptecrypt start..."

cat > /tmp/multidisk_one_key_decrypt_token.json << EOF
{
    "type":"usec-full-crypt",
    "keyslots":["0"],
}
EOF

    local cryptParts=$(installer_get "DI_CRYPT_DEVICE")
    for lp in ${cryptParts};do
        local part="$lp"
        local cryptPart=$(cryptsetup status ${part} | grep "device" | awk '{print $2}')
        if [ -n "${cryptPart}" ]; then
            echo "cryptsetup token remove \"${cryptPart}\" --token-id 0"
            cryptsetup token remove "${cryptPart}" --token-id 0 || true
            echo "cryptsetup token import ${cryptPart} --token-id 0 --json-file=/tmp/multidisk_one_key_decrypt_token.json"
            cryptsetup token import "${cryptPart}" --token-id 0 --json-file=/tmp/multidisk_one_key_decrypt_token.json
        fi
    done

    info "setup_tpm_dsetup_multidisk_one_key_decryptecrypt end!"
}

# 保存授权数据到分区表
save_authorized_data_to_parttable()
{
    # 获取选中磁盘上的roota分区
    local output=$(installer_get "DI_DEVICE_LIST")";"
    local device_array=$(echo ${output} | tr -s ';')
    local allRootaPaths=""
    local diskPathToSaveAuthorizedData=""
    local sectorSize=512
    while [ -n "${device_array}" ]; do
        local diskPath=${device_array%%;*}
        # 第一个用于分区表保存授权数据
        if [ -z "${diskPathToSaveAuthorizedData}" ]; then
            diskPathToSaveAuthorizedData=${diskPath}
            # 获取磁盘的物理扇区大小
            sectorSize=$(lsblk -pnlo path,phy-sec | grep -w ${diskPath} | awk '{print $2}')
        fi
        device_array=${device_array#*;}
        local rootaPaths=$(lsblk -lpno NAME,LABEL $dev | grep "Roota" | awk '{print $1}' | tr -s '\n' ';')
        if [ -n "${rootaPaths}" ]; then
            allRootaPaths="${allRootaPaths}${rootaPaths};"
        fi
    done

    # 计算写入的扇区数量，减65是为了避免读取到分区表信息
    local writeSectorCount=$(( 1024 * 1024 / ${sectorSize} - 65 ))

    # 在开始备份授权数据前，先尝试从分区表中读取授权数据
    if [ -n "${diskPathToSaveAuthorizedData}" ]; then
        ## 从分区表后面的空间读取授权数据
        dd if=${diskPathToSaveAuthorizedData} of=/tmp/licenceFilesDir.tar bs=${sectorSize} skip=64 count=${writeSectorCount} || warning "dd if=${diskPathToSaveAuthorizedData} of=/tmp/licenceFilesDir.tar bs=${sectorSize} skip=64 count=${writeSectorCount} failed !"
        ## 解压授权数据
        tar -xvf /tmp/licenceFilesDir.tar -C /tmp/ || warning "tar -xvf /tmp/licenceFilesDir.tar -C /tmp/ failed !"
        if is_empty_dir "/tmp/var"; then
            warning "Not found authorized data in partition table area!"
        fi
    fi

    # 遍历${allRootaPaths}所有roota分区路径挂载到/mnt,读取/etc/fstab找到var分区（目录），
    ## 拷贝授权数据/var/uos/.license.xkey到/tmp/var/uos/uos1
    ## 拷贝授权数据/var/lib/uos-license/.license.json到/tmp/var/lib/uos-license/uos1
    local rootaPathArray=$(echo ${allRootaPaths} | tr -s ';')
    while [ -n "${rootaPathArray}" ]; do
        local rootaPath=${rootaPathArray%%;*}
        rootaPathArray=${rootaPathArray#*;}
        if [ ! -e "/target" ]; then
            mkdir -p /target
        fi
        mount ${rootaPath} /target || continue
        local rootaUUID=$(cat /target/etc/fstab | grep "UUID=" | grep -w "/" | awk '{print $1}')
        rootaUUID=${rootaUUID#UUID=*}

        local varUUID=$(cat /target/etc/fstab | grep "UUID=" | grep -w "/var" | awk '{print $1}')
        varUUID=${varUUID#UUID=*}
        local varPath=""
        if [ -n "${varUUID}" ]; then
            varPath="/target/var"
            mount --uuid ${varUUID} ${varPath} || varPath=""
            if [ -z "${varPath}" ]; then
                warning "mount ${varUUID} to ${varPath} failed !"
            fi
        else
            local persistentUUID=$(cat /target/etc/fstab | grep "UUID=" | grep "/persistent" | awk '{print $1}')
            persistentUUID=${persistentUUID#UUID=*}
            local persistentPath="/target/persistent"
            varPath="${persistentPath}/ostree/deploy/deepin/var"
            if [ -n "${persistentUUID}" ]; then
                mount --uuid ${persistentUUID} ${persistentPath} || varPath=""
                if [ -z "${varPath}" ]; then
                    warning "mount ${persistentUUID} to ${persistentPath} failed !"
                fi
            fi

            if [ ! -d "${varPath}" ]; then
                varPath=""
            fi
        fi

        if [ -n "${varPath}" ]; then
            [ -e /tmp/var/uos/uos-${rootaUUID} ] || mkdir -p /tmp/var/uos/uos-${rootaUUID}
            [ -e /tmp/var/lib/uos-license/uos-${rootaUUID} ] || mkdir -p /tmp/var/lib/uos-license/uos-${rootaUUID}
            if [ ! -f /tmp/var/uos/uos-${rootaUUID}/.license.xkey ]; then
                cp -avf ${varPath}/uos/.license.xkey /tmp/var/uos/uos-${rootaUUID} || warning "cp ${varPath}/uos/.license.xkey to /tmp/var/uos/uos-${rootaUUID} failed !"
            fi
            if [ ! -f /tmp/var/lib/uos-license/uos-${rootaUUID}/.license.json ]; then
                cp -avf ${varPath}/lib/uos-license/.license.json /tmp/var/lib/uos-license/uos-${rootaUUID} || warning "cp ${varPath}/lib/uos-license/.license.json to /tmp/var/lib/uos-license/uos-${rootaUUID} failed !"
            fi
        fi

        umount -Rlf /target/ || warning "umount -Rlf /target/ failed!"
    done


    if ! is_empty_dir "/tmp/var/uos"; then
        # 在/tmp/var/uos下列出所有包含uos-的子目录
        local authedDataDirs=$(ls /tmp/var/uos | grep "uos-" | tr -s '\n' ';')
        authedDataDirs=$(echo ${authedDataDirs} | tr -s ';')
        authedDataDirs=${authedDataDirs%;}

        mkdir -p /tmp/var/tmp
        cat > /tmp/var/tmp/deepin_installer_authed_data_dir_config.ini << EOF
[General]
;; 如果有多个原系统的权数据被保留，则原系统的授权文件保存在这个字段指定的子目录中，这些子目录分别在/var/uos/ 和 /var/lib/uos-licence/目录下
;; 如果只有一个原系统的权数据被保留则这个字段是空的
;; 目录的命名规则是uos-${rootaUUID}
authedDataDirs="${authedDataDirs}"
EOF

        tar -cvf /tmp/licenceFilesDir.tar -C /tmp var || warning "tar -cvf /tmp/licenceFilesDir.tar -C /tmp var failed !"

        if [ -n "${diskPathToSaveAuthorizedData}" ]; then
            if [ -e "/tmp/licenceFilesDir.tar" ]; then
                # 向分区表后面的空间写入授权数据
                dd if=/tmp/licenceFilesDir.tar of=${diskPathToSaveAuthorizedData} bs=${sectorSize} seek=64 count=${writeSectorCount} conv=sync || warning "dd if=/tmp/licenceFilesDir.tar of=${diskPathToSaveAuthorizedData} bs=${sectorSize} seek=64 count=${writeSectorCount} conv=sync failed !"
                # 清除临时文件
                rm -rvf /tmp/licenceFilesDir.tar /tmp/var || warning "rm /tmp/licenceFilesDir.tar /tmp/var failed !"
            fi
        fi
    fi
}

# 从分区表后面的空间读取授权数据到目标磁盘
save_authorized_data_from_parttable_to_target()
{
    # 根据DI_DEVICE_LIST获取用户选中的系统盘，用于读取保存在分区表后面空间的授权数据
    local output=$(installer_get "DI_DEVICE_LIST")";"
    local device_array=$(echo ${output} | tr -s ';')
    local diskPathToSaveAuthorizedData=${device_array%%;*}

    if [ -n "${diskPathToSaveAuthorizedData}" ]; then
        # 获取磁盘的物理扇区大小
        local sectorSize=$(lsblk -pnlo path,phy-sec | grep -w ${diskPathToSaveAuthorizedData} | awk '{print $2}')
        # 计算读取的扇区数量，减65是为了避免读取到分区表信息
        local writeSectorCount=$(( 1024 * 1024 / ${sectorSize} - 65 ))
        # 从分区表后面的空间读取授权数据
        dd if=${diskPathToSaveAuthorizedData} of=/tmp/licenceFilesDir.tar bs=${sectorSize} skip=64 count=${writeSectorCount} || warning "dd if=${diskPathToSaveAuthorizedData} of=/tmp/licenceFilesDir.tar bs=${sectorSize} skip=64 count=${writeSectorCount} failed!"
        # 解压授权数据
        tar -xvf /tmp/licenceFilesDir.tar -C /tmp/ || warning "tar -xvf /tmp/licenceFilesDir.tar -C /tmp/ failed !"
        if ! is_empty_dir "/tmp/var"; then
            # 拷贝授权数据到目标磁盘分区
            cp -rvf /tmp/var/* /target/var/
            # 清除分区表后面的空间中的授权数据
            dd if=/dev/zero of=${diskPathToSaveAuthorizedData} bs=${sectorSize} seek=64 count=${writeSectorCount} conv=sync || warning "dd if=/dev/zero of=${diskPathToSaveAuthorizedData} bs=${sectorSize} seek=64 count=${writeSectorCount} conv=sync failed !"
        fi
    fi
}
