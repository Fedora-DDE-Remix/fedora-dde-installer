#!/bin/bash

prepare_customize_user() {
    local oem_dir=$DI_LIVE_DIR_ENV/oem
    local oem_desk_background="$oem_dir/background.jpg"
    local sys_desk_background=/usr/share/backgrounds/default_background.jpg

    if [ -f "${oem_desk_background}" ]; then
        install -Dm644 -v "${oem_desk_background}" "${sys_desk_background}" || \
        warning "Failed to copy wallpaper to system"
    fi

    local oem_lightdm_background="${oem_dir}/lightdm.png"
    local sys_lightdm_background=/usr/share/backgrounds/oem-lightdm.png

    if [ -f "${oem_lightdm_background}" ]; then
        install -Dm644 -v "${oem_lightdm_background}" "${sys_lightdm_background}" || \
        warning "Failed to copy lightdm background to system"
    fi
}

reload_conf() {
    local allow=$1
    dbus-send --system --type=method_call --print-reply --dest=org.deepin.dde.Accounts1 /org/deepin/dde/Accounts1 org.deepin.dde.Accounts1.EnablePasswdChangedHandler boolean:$allow
}

# 覆盖用户的默认设置。
customize_user() {
    local username=$(installer_get "DI_USERNAME")
    local conf_dir="/var/lib/AccountsService/deepin/users"
    local user_conf="${conf_dir}/${username}"
    local root_conf="${conf_dir}/root"
    local root_password=$(installer_get "DI_ROOT_PASSWORD")
    local locale=$(installer_get "DI_LOCALE")
    local xlayout=$(installer_get "DI_LAYOUT")
    local xlayout_variant=$(installer_get "DI_LAYOUT_VARIANT")

    mkdir -pv ${conf_dir} && touch ${user_conf}
    deepin-installer-config set "${user_conf}" "User" "XSession" "deepin"
    info "deepin-installer-config set \"${user_conf}\" \"User\" \"XSession\" \"deepin\""

    deepin-installer-config set "${user_conf}" "User" "SystemAccount" "false"
    info "deepin-installer-config set \"${user_conf}\" \"User\" \"SystemAccount\" \"false\""

    #把键盘和语言的配置写到/var/lib/AccountsService/deepin/users/下
    deepin-installer-config set "${user_conf}" "User" "Layout" "${xlayout};${xlayout_variant}"
    info "deepin-installer-config set \"${user_conf}\" \"User\" \"Layout\" \"${xlayout};${xlayout_variant}\""

    deepin-installer-config set "${user_conf}" "User" "Locale" "${locale}.UTF-8"
    info "deepin-installer-config set \"${user_conf}\" \"User\" \"Locale\" \"${locale}.UTF-8\""

    #如果启用root账号
    if [ -n "${root_password}" ]; then
        touch ${root_conf}
        deepin-installer-config set "${root_conf}" "User" "Layout" "${xlayout};${xlayout_variant}"
        deepin-installer-config set "${root_conf}" "User" "Locale" "${locale}.UTF-8"

        #去掉双引号
        sed -i 's/\"'"${xlayout};${xlayout_variant}"'\"/'"${xlayout};${xlayout_variant}"'/g' ${root_conf}

        info "cat ${root_conf}"
        cat ${root_conf}
    fi

    local avatar=$(installer_get "DI_AVATAR")
    if [ -f "${avatar}" ]; then
        deepin-installer-config set "${user_conf}" "User" "Icon" "file://${avatar}"
        info "deepin-installer-config set \"${user_conf}\" \"User\" \"Icon\" \"file://${avatar}\""
    fi

    local sys_lightdm_background=/usr/share/backgrounds/oem-lightdm.png
    if [ -f "${sys_lightdm_background}" ]; then
        deepin-installer-config set "${user_conf}" "User" "GreeterBackground" "file://${sys_lightdm_background}"
        info "deepin-installer-config set \"${user_conf}\" \"User\" \"GreeterBackground\" \"file://${sys_lightdm_background}\""
    fi

    #去掉双引号
    sed -i 's/\"'"${xlayout};${xlayout_variant}"'\"/'"${xlayout};${xlayout_variant}"'/g' ${user_conf}

    info "cat ${user_conf}"
    cat ${user_conf}
}

# 给用户添加组，相关的组都在/etc/group文件里。
add_user_to_existed_group() {
    local user=$1
    local group=$2

    if grep -q "^${group}:" /etc/group 2>/dev/null; then
        gpasswd -a "${user}" "${group}"
    else
        warning "No group found ${group}"
    fi
}

add_user() {
    local username=$1
    local uid=${2:-}
    local cmd_argc="-U -m --skel /etc/skel --shell /bin/bash"
    if [[ -d /home/${username} ]]; then
        cmd_argc="-U --shell /bin/bash"
    fi
    if [ -n "${uid}" ]; then
        cmd_argc="${cmd_argc} -u ${uid}"
    fi

    info "Start create user: ${username}"
    useradd $cmd_argc ${username}
    info "useradd $cmd_argc ${username}"
}

# 从指定数字开始，找下一个未被使用的 UID
next_available_uid() {
    local uid=$1

    while getent passwd "${uid}" >/dev/null 2>&1; do
        uid=$((uid + 1))
    done

    echo "${uid}"
}

# 设置用户
setup_username() {
    local username=$(installer_get "DI_USERNAME")
    local preferred_uid=1000
    local uid=${preferred_uid}
    [ -z "${username}" ] && error "Variable DI_USERNAME not set. Abort!"
    if getent passwd "${preferred_uid}" >/dev/null 2>&1; then
        uid=$(next_available_uid "${preferred_uid}")
        warning "UID ${preferred_uid} already exists. Use UID ${uid} for user ${username}."
    fi

    # 先修改home目录下的权限
    if [ -n "$(ls /home)" ]; then
        #删除指定文件
        local clean_list=$(installer_get "DI_SAVE_USER_DATA_CLEAN_LIST")
        echo "clean_list=$clean_list"
        IFS=";" read -ra file_array <<< "${clean_list}"
        for element in "${file_array[@]}"
        do
            echo "delete $element"
            rm -rf $element || echo "delete file ${element} failed!"
        done

        ls /home | xargs -I {} rm -vrf "/home/{}/.local/share/keyrings"
        ls /home | xargs -I {} rm -vrf "/home/{}/.local/share/deepin-keyrings-wb"
        ls /home | grep -v "$username" | xargs -I {} chown -hR root:root /home/{} || warning "chown -hR root:root /home failed!"
    fi

    add_user $username $uid

    # 补全原用户目录框架和权限等属性
    cp -vfr -upP /etc/skel/* /home/${username}
    # 遍历 /home/${username} 下的所有文件和目录，逐个设置所有者，设置失败的则输出警告信息
    find /home/${username} -depth | while read -r file; do
        chown -h "${username}:${username}" "${file}" || warning "chown -h ${username}:${username} ${file} failed!"
    done

    # 添加到组里
    set +e
    add_user_to_existed_group "${username}" lp
    add_user_to_existed_group "${username}" lpadmin
    add_user_to_existed_group "${username}" netdev
    add_user_to_existed_group "${username}" network
    add_user_to_existed_group "${username}" sambashare
    add_user_to_existed_group "${username}" scanner
    add_user_to_existed_group "${username}" storage
    add_user_to_existed_group "${username}" sudo
    add_user_to_existed_group "${username}" users
    add_user_to_existed_group "${username}" wheel
    set -e
}

add_password() {
    local username=$1
    local password=$2
    echo "${username}:${password}" | chpasswd
}

setup_password_proc() {
    local username=$1
    local password=$2

    if [ "x$(installer_get "DI_PASSWORD_ENCRYPTION_CONFIG")" = "xtrue" ]; then
        password=$(deepin-installer-command-agent -d -p ${password})
    fi
    add_password ${username} ${password}
}

# 设置密码
setup_password() {
    info "Start setup password"

    local username=$(installer_get "DI_USERNAME")
    local password=$(installer_get "DI_PASSWORD")
    local rootpassword=$(installer_get "DI_ROOT_PASSWORD")

    # 更新密码。
    if [ -z "${password}" ]; then
        passwd -d "${username}"
        info "Set password to empty"
    else
        setup_password_proc ${username} ${password}
        info "Set ${username} password"
    fi

    # 更新root用户密码。
    if [ -n "${rootpassword}" ]; then
        setup_password_proc root ${rootpassword}
        info "Set root password"
    fi
}

#清除密码
clean_password() {
    # 将配置里的密码清空。
    installer_set "DI_PASSWORD" ""
    installer_set "DI_ROOT_PASSWORD" ""
}

add_hostname() {
    local hostname=$1
    echo "${hostname}" | tee /etc/hostname

    cat > /etc/hosts << EOF
127.0.0.1 localhost

# The following lines are desirable for IPv6 capable hosts
::1     localhost ip6-localhost ip6-loopback
fe00::0 ip6-localnet
ff00::0 ip6-mcastprefix
ff02::1 ip6-allnodes
ff02::2 ip6-allrouters
EOF
}

# 设置主机名。
setup_hostname() {
    info "Start setup hostname"

    local hostname=$(installer_get "DI_HOSTNAME")
    hostname="${hostname:-deepin}"
    add_hostname $hostname
}

# 设置快速登录。
setup_quick_login() {
    local quick_login=$(installer_get "DI_QUICK_LOGIN")
    if [ "${quick_login}" = "true" ]; then
        local lightdm_conf="/etc/lightdm/lightdm.conf"
        if is_have_first_boot; then
            lightdm_conf="/etc/lightdm/lightdm.conf.real"
        fi

        if [ -f "${lightdm_conf}" ]; then
            #  增加快速登录配置项
            sed -i '/^\[Seat:\*\]$/a quicklogin-enabled=true' $lightdm_conf
        else
            error "${lightdm_conf} not found"
        fi

        local state_user_path="/var/lib/lightdm/lightdm-deepin-greeter"
        local username=$(installer_get "DI_USERNAME")
        local uid=$(id -u "$username") || error "Failed to resolve UID for user ${username}"
        if [ ! -d ${state_user_path} ]; then
            mkdir -p  ${state_user_path}
        fi

        cat > "${state_user_path}/state_user" <<EOF
        [General]
        last-user={"AuthType":1,"Name":"$username","Type":0,"Uid":${uid}}
        quicklogin-users=$username;
EOF
        chown -R lightdm:lightdm ${state_user_path}/state_user
    fi
}

# 更新键盘。
setup_keyboard() {
    local xkblayout=$(installer_get "DI_LAYOUT")
    local xkbvariant=$(installer_get "DI_LAYOUT_VARIANT")
    local xkbmodel=$(installer_get "DI_KEYBOARD_MODEL")
    local xkboptions=$(installer_get "DI_KEYBOARD_OPTIONS")

    xkbmodel=${xkbmodel:-pc105}
    xkblayout=${xkblayout:-us}

    cat > /etc/default/keyboard << EOF
# Check /usr/share/doc/keyboard-configuration/README.Debian for
# documentation on what to do after having modified this file.

# The following variables describe your keyboard and can have the same
# values as the XkbModel, XkbLayout, XkbVariant and XkbOptions options
# in /etc/X11/xorg.conf.

XKBMODEL="${xkbmodel}"
XKBLAYOUT="${xkblayout}"
XKBVARIANT="${xkbvariant}"
XKBOPTIONS="${xkboptions}"

# If you don't want to use the XKB layout on the console, you can
# specify an alternative keymap.  Make sure it will be accessible
# before /usr is mounted.
# KMAP=/etc/console-setup/defkeymap.kmap.gz
EOF
}

setup_locale() {
    local tmp_locale=$(installer_get "DI_LOCALE")
    tmp_locale=${tmp_locale:-en_US}

    local locale=${tmp_locale%.*}

    # Disable all locales first.
    sed -i 's/^\([^#].*\)$/# \1/g' /etc/locale.gen

    # Always generate en_US locale.
    info "Generating locale: en_US ${locale}"
    sed -i "s/# \(en_US\.UTF-8.*$\)/\1/g" /etc/locale.gen

    # generate user selected locale.
    sed -i "s/# \(${locale}[\. ]UTF-8.*$\)/\1/g" /etc/locale.gen

    # 获取/etc/locale.gen locale值
    local lang=${locale}.UTF-8
    if [ $(grep -c "${locale}\.UTF-8" /etc/locale.gen) -eq 0 ]; then
        lang=${locale}
    fi

    cat > /etc/default/locale << EOF
LANG=${lang}
LANGUAGE=${locale}
EOF

    # Re-generate localisation files.
    /usr/sbin/locale-gen

    # Enable extral zh_CN charsets.
    if test x${locale} = xzh_CN; then
        local LOCALE_DIR=/usr/lib/locale
        sed -i "s/# \(${locale}\.GBK.*$\)/\1/g" /etc/locale.gen
        sed -i "s/# \(${locale}\.GB18030.*$\)/\1/g" /etc/locale.gen
        sed -i "s/# \(${locale}\ GB2312.*$\)/\1/g" /etc/locale.gen
        # Use pre-compiled locale archives
        #for P_LOCALE in ${LOCALE_DIR}/{zh_CN,zh_CN.gb18030,zh_CN.gbk}; do
        #localedef --add-to-archive ${P_LOCALE}
        #done
    #else
        #rm -rf ${LOCALE_DIR}/{zh_CN,zh_CN.gb18030,zh_CN.gbk}
    fi
    # Check locale
    if ls /usr/share/i18n/locales | grep -q "\<${locale}\>"; then
        info "${locale} is available"
    else
        info "${locale} is not available, Fallback to en_US"
        unset LOCALE
    fi
}

# 设置时区。
setup_timezone() {
    local timezone=$(installer_get "DI_TIMEZONE")

#    info "Check timezone ${timezone}"
#    if cat /usr/share/zoneinfo/zone.tab | grep -v '^#' | awk '{print $3}' | \
#        grep -q "^${timezone}$"; then
#        info "${timezone} is available"
#    else
#        if [ "${{timezone}}" = "" ]; then}"]
#        timezone="Asia/Beijing"
#        info "Timezone is not available, Fall back to Asia/Beijing"
#    fi

    echo "${timezone}" > /etc/timezone
    info "Set timezone to ${timezone}"

    local cn_extra_timezones=("Asia/Beijing" "Asia/Chengdu" "Asia/Nanjing" "Asia/Wuhan" "Asia/Xian")
    if contains_element "${timezone}" "${cn_extra_timezones[@]}"; then
        ln -nsf /usr/share/zoneinfo/Asia/Shanghai /etc/localtime
    else
        ln -nsf /usr/share/zoneinfo/${timezone} /etc/localtime
    fi

    # 如果启用的RTC，则本地时间改成硬件时间
    if [ -n "$(cat /etc/adjtime | grep "LOCAL" || echo "")" ]; then
        info "before set systime from rtc : $(timedatectl)"
        local isSetHcToSysOk=$(hwclock --hctosys || echo "false")
        if [[ "${isSetHcToSysOk}" == "false" ]]; then
            warning "hwclock --hctosys failed try use utc time"
            local utcTime=$(date -u "+%Y-%m-%d %H:%M:%S" || echo "false")
            if [[ "${utcTime}" == "false" ]]; then
                warning "get utc time failed"
            else
                info "set system time to ${utcTime}"
                local isSetSysTimeOk=$(date -s "${utcTime}" || echo "false")
                if [[ "${isSetSysTimeOk}" == "false" ]]; then
                    warning "set system time failed"
                fi
            fi
        else
            info "hwclock --hctosys success"
        fi
        info "after set systime from rtc : $(timedatectl)"
    fi
    # 更新系统应用的时间戳
    #touch -c /*
}

# 设置系统的运行级别
setup_default_target() {
    info "TARGET:before={$(systemctl get-default)}"
    local DI_COMPONENT_UNINSTALL=$(installer_get "DI_COMPONENT_UNINSTALL")
    if [[ "${DI_COMPONENT_UNINSTALL}" =~ ^(.* )?lightdm( .*)?$ ]]; then
         info "TARGET:lightdm will be removed:{${DI_COMPONENT_UNINSTALL}}"
         systemctl set-default -f multi-user.target
    elif [ -f /usr/sbin/lightdm ]; then
         info "TARGET:lightdm program found."
        systemctl set-default -f graphical.target
    else
        info "TARGET:lightdm program NOT found."
        systemctl set-default -f multi-user.target
    fi
    info "TARGET:after={$(systemctl get-default)}"
}

# 配置蓝牙服务
setup_bluetooth_service() {
    if [ -f /lib/systemd/system/bluetooth.service ]; then
        systemctl restart bluetooth.service
    fi
}

add_uninstall_package() {
    local pkg="$1"
    if dpkg -l | grep -q "^ii  $pkg"; then
	    local list=$(installer_get "DI_UNINSTALL_PACKAGES")
	    list="${list} $pkg"
        installer_set "DI_UNINSTALL_PACKAGES" "${list}"
        info "list:${list}"
    else
        info "not installed $pkg"
    fi
}

# Remove component packages
remove_component_packages() {
    local DI_COMPONENT_UNINSTALL=$(installer_get "DI_COMPONENT_UNINSTALL")
    if [ ! -z "${DI_COMPONENT_UNINSTALL}" ];then
        local list=$(installer_get "DI_UNINSTALL_PACKAGES")
        list="${list} ${DI_COMPONENT_UNINSTALL}"
        installer_set "DI_UNINSTALL_PACKAGES" "${list}"
    fi
}

# 检查系统是否使用到了btrfs文件系统。
detect_btrfs() {
  for i in $(lsblk -o FSTYPE | sed '/^$/d' | uniq); do
    [ "${i}" = "btrfs" ] && return 0
  done
  return 1
}

# Check whether current machine is virtualbox.
detect_vbox() {
  if dmidecode | grep -q "Product Name: VirtualBox"; then
    return 0
  fi
  return 1
}

# Check whether current machine is vmware.
detect_vmware() {
  if dmidecode | grep -q "Product Name: VMware"; then
    return 0
  fi
  return 1
}

is_loongson_integrated_graphics() {
    local ret=$(lshw -c video | grep "driver=loongson-drm")
    if [ ! -z "${ret}" ]; then
        return 0
    else
        return 1
    fi
}

# 卸载包括安装器等一些包, 需要再chroot环境执行
uninstall_packages() {
    local list=$(installer_get "DI_UNINSTALL_PACKAGES")
    if [ -n "${list}" ]; then
        apt-get -y purge $list
    fi
}

# 更新/etc/apt/sources.list
update_apt_sources_list() {
    readonly DEB_ENTRY=$(installer_get "DI_APT_SOURCE_DEB")
    readonly DEB_SRC_ENTRY=$(installer_get "DI_APT_SOURCE_DEB_SRC")
    cat > /etc/apt/sources.list <<EOF
## Generated by deepin-installer
${DEB_ENTRY}
${DEB_SRC_ENTRY}
EOF
}

is_create_check_mode_user() {
    local username=$(installer_get DI_CM_USER)
    if ! id $username; then
        return 0
    fi

    return 1
}

init_check_mode_user() {
    local username=$(installer_get DI_CM_USER)
    local password=$(installer_get DI_CM_PASSWORD)
    local check_mode_uid=$(next_available_uid 1999)
    if [ -d /home/$username ]; then
        #生成一个唯一的审核模式用户，防止和保留用户功能保留的用户目录名重复
        username=$username-$(head -c 16 /dev/random | base64 | sed 's/[^a-z0-9]//g')
        installer_set DI_CM_USER $username
    fi
    add_user $username $check_mode_uid
    add_user_to_existed_group "$username" "sudo"
    add_password $username $password
    info "init check mode user: <$username>"

    # 如果nopasswdlogin组不存在，则创建
    if ! grep -q "^nopasswdlogin:" /etc/group; then
        info "groupadd nopasswdlogin"
        groupadd nopasswdlogin
    fi
    # 添加审核模式用户到nopasswdlogin组
    info "usermod -aG nopasswdlogin ${username}"
    usermod -aG nopasswdlogin "${username}"

    # 如果autologin组不存在，则创建
    if ! grep -q "^autologin:" /etc/group; then
        info "groupadd autologin"
        groupadd autologin
    fi
    # 添加审核模式用户到autologin组
    info "usermod -aG autologin ${username}"
    usermod -aG autologin "${username}"
    info "gpasswd -a ${username} autologin"
    gpasswd -a ${username} autologin
}

delete_user() {
    local username=$1

    # 删除账户之前先清理一遍用户进程
    ps -aux | grep ${username}
    echo -n "$(ps -aux | grep ${username} | awk '{print $2}')" | xargs -I {} bash -c "kill -9 {} || echo \"kill {} failed!\""
    ps -aux | grep ${username}

    userdel -rf $username
}

enable_local_rtc() {
    info "before rtc set : $(timedatectl)"
    local DI_USE_RTC_TIME_CONFIG=$(installer_get "DI_USE_RTC_TIME_CONFIG")
    if [ "x${DI_USE_RTC_TIME_CONFIG}" = "xtrue" ]; then
        # 参考 /etc/init.d/hwclock.sh
        printf "0.0 0 0.0\n0\nLOCAL\n" > /etc/adjtime
        info "set LOCAL to /etc/adjtime"
    else
        rm /etc/adjtime || true
    fi
    info "after rtc set : $(timedatectl)"
}

enable_ntp() {
    info "before ntp set : $(timedatectl)"
    local DI_IS_ENABLE_NTP=$(installer_get "DI_IS_ENABLE_NTP")
    if [[ x"${DI_IS_ENABLE_NTP}" = xtrue ]]; then
        # 启动ntp服务，设置开启时间同步属性
        rm -fr /usr/lib/systemd/system/systemd-timesyncd.service.d/disable-timesyncd-with-installer.conf
        timedatectl set-ntp true
        info "timedatectl set-ntp true"
    else
        # 手动设置时间时，关闭ntp服务
        timedatectl set-ntp false
        info "timedatectl set-ntp false"
    fi
    info "after ntp set : $(timedatectl)"
}

set_timedate() {
    # 格式为 2023-01-01 10:34:36 可以这样获取 ：
    # local timeDate=$(hwclock --show)
    # timeDate=${timeDate%.*}

    local timeDate=$1
    if [ ! -z "${timeDate}" ]; then
        info "before time set : $(timedatectl)"
        # 先禁止时间同步服务， 防止时间设置不生效
        timedatectl set-ntp false
        info "timedatectl set-ntp false"
        date -s "${timeDate}"
        info "date -s ${timeDate}"
        info "after time set : $(timedatectl)"
    fi
}

set_systime_to_hc() {
    local DI_SYSTIME_TO_HC=$(installer_get "DI_SYSTIME_TO_HC")
    if [[ x"${DI_SYSTIME_TO_HC}" = xtrue ]]; then
        info "before set systime to hc : $(timedatectl)"
        hwclock --systohc
        info "hwclock --systohc"
        info "after set systime to hc : $(timedatectl)"
    fi
}

# 用dde-dconfig设置org.deepin.region-format下的配置项，区域格式的大部分配置项在这个路径下
dde_dconfig_deepin_region_format_set() {
    local username=$1
    local regionFormatKey=$2
    local regionFormatValue=$3
    if [ -n "${username}" ] && [ -n "${regionFormatKey}" ] && [ -n "${regionFormatValue}" ]; then
        sudo -u "${username}" dde-dconfig set -a "" -r "org.deepin.region-format" -k "${regionFormatKey}" -v "${regionFormatValue}" || true
        local resultRegionFormatValue=$(sudo -u "${username}" dde-dconfig get "" -r "org.deepin.region-format" -k "${regionFormatKey}" | tr -d "\""|| true)
        if [ "${resultRegionFormatValue}" != "${regionFormatValue}" ]; then
            warning "${regionFormatKey} set failed !"
        fi
    else
        warning "dde_dconfig_deepin_region_format_set invalid param ! username=${username} regionFormatKey=${regionFormatKey} regionFormatValue=${regionFormatValue}"
    fi
}

# 用dde-dconfig设置org.deepin.dde.daemon.format下的配置项，部分格式配置项在这个路径下，需要单独设置
dde_dconfig_deepin_dde_daemon_format_set() {
    local username=$1
    local regionFormatKey=$2
    local regionFormatValue=$3
    if [ -n "${username}" ] && [ -n "${regionFormatKey}" ] && [ -n "${regionFormatValue}" ]; then
        sudo -u "${username}" dde-dconfig set org.deepin.dde.daemon -r org.deepin.dde.daemon.format -k "${regionFormatKey}" -v "${regionFormatValue}" || true
        local resultRegionFormatValue=$(sudo -u "${username}" dde-dconfig get org.deepin.dde.daemon -r org.deepin.dde.daemon.format -k "${regionFormatKey}" | tr -d "\""|| true)
        if [ "${resultRegionFormatValue}" != "${regionFormatValue}" ]; then
            warning "${regionFormatKey} set failed !"
        fi
    else
        warning "dde_dconfig_deepin_dde_daemon_format_set invalid param ! username=${username} regionFormatKey=${regionFormatKey} regionFormatValue=${regionFormatValue}"
    fi
}

# 设置货币格式比较特殊，除了org.deepin.region-format路径下的currencyFormat配置项外，还需要设置org.deepin.dde.daemon.format路径下的positiveCurrencyFormat和negativeCurrencyFormat这两个配置项，把其中的货币符号替换成新的货币符号
dde_dconfig_currency_format_set() {
    local username=$1
    local newCurrencyFormat=$2

    # 新建的账户没有登录的情况下获取的货币配置可能是空的，后面针对空值情况设置默认值
    local resultCurrencyFormatValue=$(sudo -u "${username}" dde-dconfig get "" -r "org.deepin.region-format" -k "currencyFormat" | tr -d "\""|| true)
    if [ -z "${resultCurrencyFormatValue}" ]; then
        warning "get currencyFormat failed !"
    fi

    local resultPositiveCurrencyFormatValue=$(sudo -u "${username}" dde-dconfig get org.deepin.dde.daemon -r org.deepin.dde.daemon.format -k "positiveCurrencyFormat" | tr -d "\""|| true)
    if [ -z "${resultPositiveCurrencyFormatValue}" ]; then
        warning "get positiveCurrencyFormat failed !"
    fi

    local resultNegativeCurrencyFormatValue=$(sudo -u "${username}" dde-dconfig get org.deepin.dde.daemon -r org.deepin.dde.daemon.format -k "negativeCurrencyFormat" | tr -d "\""|| true)
    if [ -z "${resultNegativeCurrencyFormatValue}" ]; then
        warning "get negativeCurrencyFormat failed !"
    fi

    # 替换resultPositiveCurrencyFormatValue和resultNegativeCurrencyFormatValue中的货币符号为新的货币符号
    local newPositiveCurrencyFormatValue=""
    if [ -n "${resultPositiveCurrencyFormatValue}" ] && [ -n "${resultCurrencyFormatValue}" ]; then
        newPositiveCurrencyFormatValue=$(echo "${resultPositiveCurrencyFormatValue}" | sed "s/${resultCurrencyFormatValue}/${newCurrencyFormat}/g")
    else
        newPositiveCurrencyFormatValue="${newCurrencyFormat}1.1"
    fi

    local newNegativeCurrencyFormatValue=""
    if [ -n "${resultNegativeCurrencyFormatValue}" ] && [ -n "${resultCurrencyFormatValue}" ]; then
        newNegativeCurrencyFormatValue=$(echo "${resultNegativeCurrencyFormatValue}" | sed "s/${resultCurrencyFormatValue}/${newCurrencyFormat}/g")
    else
        newNegativeCurrencyFormatValue="${newCurrencyFormat}-1.1"
    fi

    # 设置新的货币格式
    dde_dconfig_deepin_dde_daemon_format_set "${username}" "positiveCurrencyFormat" "${newPositiveCurrencyFormatValue}"
    dde_dconfig_deepin_dde_daemon_format_set "${username}" "negativeCurrencyFormat" "${newNegativeCurrencyFormatValue}"
    dde_dconfig_deepin_region_format_set "${username}" "currencyFormat" "${newCurrencyFormat}"
}

set_region_fomat() {
    local username=""

    ## 判断内核参数，确定是进入安装器还是live系统桌面
    if is_livecd; then
        username=$(installer_get "LIVE_USERNAME")
    else
        username=$(installer_get "DI_USERNAME")
    fi

    # 设置localname
    local localeConfig=$(installer_get "DI_REGION_FORMAT_LOCALE")
    dde_dconfig_deepin_region_format_set "${username}" "localeName" "${localeConfig}"

    # 设置地区
    local countryConfig=$(installer_get "DI_REGION")
    dde_dconfig_deepin_region_format_set "${username}" "country" "${countryConfig}"

    # 设置区域格式
    local languageRegionConfig=$(installer_get "DI_REGION_FORMAT")
    dde_dconfig_deepin_region_format_set "${username}" "languageRegion" "${languageRegionConfig}"

    # 设置一周第一天
    local firstDayOfWeekConfig=$(installer_get "DI_REGION_FORMAT_FIRST_DAY_OF_WEEK_FORMAT")
    dde_dconfig_deepin_region_format_set "${username}" "firstDayOfWeek" "${firstDayOfWeekConfig}"

    # 设置短日期格式
    local shortDateFormatConfig=$(installer_get "DI_REGION_FORMAT_SHORT_DATE_FORMAT")
    dde_dconfig_deepin_region_format_set "${username}" "shortDateFormat" "${shortDateFormatConfig}"

    # 设置长日期格式
    local longDateFormatConfig=$(installer_get "DI_REGION_FORMAT_LONG_DATE_FORMAT")
    dde_dconfig_deepin_region_format_set "${username}" "longDateFormat" "${longDateFormatConfig}"

    # 设置短时间格式
    local shortTimeFormatConfig=$(installer_get "DI_REGION_FORMAT_SHORT_TIME_FORMAT")
    dde_dconfig_deepin_region_format_set "${username}" "shortTimeFormat" "${shortTimeFormatConfig}"

    # 设置长时间格式
    local longTimeFormatConfig=$(installer_get "DI_REGION_FORMAT_LONG_TIME_FORMAT")
    dde_dconfig_deepin_region_format_set "${username}" "longTimeFormat" "${longTimeFormatConfig}"

    # 设置数字格式
    local numberFormatConfig=$(installer_get "DI_REGION_FORMAT_NUMBER_FORMAT")
    dde_dconfig_deepin_region_format_set "${username}" "numberFormat" "${numberFormatConfig}"

    # 设置纸张格式
    local paperFormatConfig=$(installer_get "DI_REGION_FORMAT_PAPER_SIZE")
    dde_dconfig_deepin_region_format_set "${username}" "paperFormat" "${paperFormatConfig}"

    # 设置小数点符号
    local decimalSymbol=$(installer_get "DI_REGION_FORMAT_DECIMAL_SYMBOL")
    dde_dconfig_deepin_dde_daemon_format_set "${username}" "decimalSymbol" "${decimalSymbol}"

    # 设置千位分隔符
    local digitGroupingSymbol=$(installer_get "DI_REGION_FORMAT_DIGIT_GROUP_SEPARATOR")
    dde_dconfig_deepin_dde_daemon_format_set "${username}" "digitGroupingSymbol" "${digitGroupingSymbol}"

    # 设置数字分组
    dde_dconfig_deepin_dde_daemon_format_set "${username}" "digitGrouping" "${numberFormatConfig}"

    # 设置货币格式
    local currencySymbolConfig=$(installer_get "DI_REGION_FORMAT_CURRENCY_SYMBOL")
    dde_dconfig_currency_format_set "${username}" "${currencySymbolConfig}"


    # 设置locale.conf
    local region_format_local_set=$(installer_get "DI_REGION_FORMAT_LOCALE_SET")
    local user_locale_gen_path="/home/${username}/.config/locale.conf"
    local system_locale_gen_path="/etc/locale.conf"

    ## 设置用户locale.conf
    touch ${user_locale_gen_path} || warning "touch ${user_locale_gen_path} failed !"
    deepin-installer-config set ${user_locale_gen_path} "LC_NUMERIC" "${region_format_local_set}"
    deepin-installer-config set ${user_locale_gen_path} "LC_MONETARY" "${region_format_local_set}"
    deepin-installer-config set ${user_locale_gen_path} "LC_TIME" "${region_format_local_set}"
    deepin-installer-config set ${user_locale_gen_path} "LC_PAPER" "${region_format_local_set}"
    deepin-installer-config set ${user_locale_gen_path} "LC_NAME" "${region_format_local_set}"
    deepin-installer-config set ${user_locale_gen_path} "LC_ADDRESS" "${region_format_local_set}"
    deepin-installer-config set ${user_locale_gen_path} "LC_TELEPHONE" "${region_format_local_set}"
    deepin-installer-config set ${user_locale_gen_path} "LC_MEASUREMENT" "${region_format_local_set}"
    chown ${username}:${username} ${user_locale_gen_path}

    ## 设置系统locale.conf
    cat >> ${system_locale_gen_path} << EOF
LC_NUMERIC=${region_format_local_set}
LC_MONETARY=${region_format_local_set}
LC_TIME=${region_format_local_set}
LC_PAPER=${region_format_local_set}
LC_NAME=${region_format_local_set}
LC_ADDRESS=${region_format_local_set}
LC_TELEPHONE=${region_format_local_set}
LC_MEASUREMENT=${region_format_local_set}
EOF

    # 设置/etc/locale.gen
    local tmp_locale=${localeConfig}
    tmp_locale=${tmp_locale:-en_US}

    local tmp_locale_all=${tmp_locale%.*}

    # generate user selected locale.
    sed -i "s/# \(${tmp_locale_all}[\. ]UTF-8.*$\)/\1/g" /etc/locale.gen

    # Re-generate localisation files.
    deepin-immutable-ctl admin exec /usr/sbin/locale-gen

    # Check locale
    if ls /usr/share/i18n/locales | grep -q "\<${tmp_locale_all}\>"; then
        info "${tmp_locale_all} is available"
    fi
}

## 设置冰点还原
enable_ice_restore() {
    local enable_restore
    local restore_white_list
    enable_restore=$(installer_get "DI_ENABLE_ICE_RESTORE")
    if [ "${enable_restore}" = "true" ]; then
        deepin-immutable-writable enable -a -c -y || warning "ice restore enable failed"
        restore_white_list=$(installer_get "DI_ICE_RESTORE_WHITELIST")
        if [ -n "${restore_white_list}" ]; then
            deepin-immutable-writable set whitelist  "${restore_white_list}" || warning "ice restore set whitelist failed"
        fi
    fi
}
