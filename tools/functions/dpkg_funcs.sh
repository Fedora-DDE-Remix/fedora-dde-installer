#!/bin/bash

setup_repo_sources() {
    local cmdline_content=$(cat /proc/cmdline)
    # 判断pxe安装0-非pxe安装、1-nfs服务pxe安装、2-文件系统使用http下载本地、远程挂载方式pxe安装
    local pxe_install_type=0
    # 解析网络路径
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

    echo "net_path is : ${net_path}"

    if [ -e "/etc/apt/sources.list.d" ]; then
        mv /etc/apt/sources.list.d /etc/apt/sources.list.d.bak
        mkdir -p /etc/apt/sources.list.d
    fi

    if [ ${pxe_install_type} -eq 2 ];
    then
        if [ -n "${net_path}" ]; then
            # 网络仓库的路径
            net_path=${net_path%${filsystem_path}*}
            echo "net_path is : ${net_path}"

            # 设置远程的镜像仓库
            echo "deb [trusted=yes] ${net_path} eagle main" >> /etc/apt/sources.list.d/deepin-installer.list

            apt-get update || true
        else
            echo "net_path is empty!"
        fi
    else
        # 本地仓库的路径
        local live_dir=$DI_LIVE_DIR_ENV
        local local_repo=$live_dir/dists
        if [ ! -d $DI_LIVE_DIR_ENV/dists ]; then
            live_dir=$DI_LIVE_DIR_ENV
            local_repo=$live_dir/dists
        fi

        # add cdrom to sources.list
        if [ -d $local_repo ]; then
            dir=$(find $local_repo -name "Release" | xargs awk -F '[ :]+' '/Codename/{print $2}')
            for name in $dir; do
                # ident the cdrom first.
                echo "deb [trusted=yes] file:$live_dir ${name} main" >> /etc/apt/sources.list.d/deepin-installer.list
            done

            apt-get update || true
        else
            warning "$local_repo is not a debian repository. Skipped."
        fi
    fi

    # 设置版本仓库
    setup_edition_repo_sources
}

install_package_apt() {
    for i in $@; do
        DEBIAN_FRONTEND="noninteractive" apt-get -y -o Dpkg::Options::="--force-confdef" \
            -o Dpkg::Options::="--force-confold" --no-install-recommends --allow-unauthenticated install $i

        if [ $? -eq 0 ]; then
            info "Install $i succeed !"
        else
            warning "Install $i failed !"
            return 1
        fi
    done
    return 0
}

install_all_package_apt() {
	local DEB_DIR=$1
	if ! is_empty_dir $DEB_DIR; then
		ls "${DEB_DIR}"
		local result=0
		DEBIAN_FRONTEND="noninteractive" apt-get -y --allow-downgrades install -o Dpkg::Options::="--force-confnew" ${DEB_DIR}/*.deb || result=$?
		#如果result不等于0，说明安装失败
		if [ $result -eq 0 ]; then
			info "install package succeed:${DEB_DIR}/*.deb"
		else
			warning "install package failed : ${DEB_DIR}/*.deb"
		fi
		return $result
	fi
}

install_package_by_immutable() {
    local pkgs="$*"

    if [ -z "$pkgs" ]; then
        return 0
    fi

    local persistentDir=/target/persistent
    local sysRootDir=/target

    local commitId sysCommitIdIdx
    commitId=$(ostree rev-parse --repo=$sysRootDir/ostree/repo $DI_REPO_BASE_NAME_ENV)
    sysCommitIdIdx="$commitId.0"

    local repo_dir=$DI_LIVE_DIR_ENV
    if [ ! -d "$repo_dir/dists" ]; then
        repo_dir=""
    fi

    local pkgs_list=($pkgs)
    echo "deepin-immutable-ctl ext install  --sysroot $sysRootDir --persistent $persistentDir  --sysbase $sysCommitIdIdx \
                     --local-repo \"$repo_dir\" \"${pkgs_list[@]}\""
    deepin-immutable-ctl ext install  --sysroot $sysRootDir --persistent $persistentDir  --sysbase "$sysCommitIdIdx" \
            --local-repo "$repo_dir" "${pkgs_list[@]}"
    deepin-immutable-ctl admin status --persistent $persistentDir

}

install_all_package_by_immutable() {
    local deb_dir=$1
    if ! is_empty_dir $deb_dir; then
        local pkgs=$(echo ${deb_dir}/*.deb)
        #如果pkgs为空，则返回
        if [ "$pkgs" = "${deb_dir}/*.deb" ]; then
            return 0
        fi

        local persistentDir=/target/persistent
        local sysRootDir=/target

        local commitId sysCommitIdIdx
        commitId=$(ostree rev-parse --repo=$sysRootDir/ostree/repo $DI_REPO_BASE_NAME_ENV)
        sysCommitIdIdx="$commitId.0"

        local repo_dir=$DI_LIVE_DIR_ENV
        if [ ! -d "$repo_dir/dists" ]; then
            repo_dir=""
        fi
        local pkgs_list=($pkgs)
        echo "deepin-immutable-ctl ext install  --sysroot $sysRootDir --persistent $persistentDir  --sysbase $sysCommitIdIdx \
                                  --local-repo \"$repo_dir\" \"${pkgs_list[@]}\""
        deepin-immutable-ctl ext install  --sysroot $sysRootDir --persistent $persistentDir  --sysbase "$sysCommitIdIdx" \
                    --local-repo "$repo_dir" "${pkgs_list[@]}"
        deepin-immutable-ctl admin status --persistent $persistentDir
    fi
}

install_package() {
    #if is_ostree_system;then
    #    install_package_by_immutable $@
    #else
        install_package_apt $@
    #fi
}

install_all_package() {
    #if is_ostree_system;then
    #    install_all_package_by_immutable $@
    #else
        install_all_package_apt $@
    #fi
}

install_oem_deb_package() {
    info "install_oem_deb_package"
    local  OEM_DEB=$DI_LIVE_DIR_ENV/oem/deb
    info "OEM_DEB=$OEM_DEB"
    install_all_package $OEM_DEB
}

install_oem_uab_package() {
    info "install_oem_uab_package"
    local  OEM_DEB=$DI_LIVE_DIR_ENV/oem/deb
    install_all_uab_package $OEM_DEB
}

#安装组件包
install_component_package() {
    info "install_component_package"
    local DI_COMPONENT_PACKAGES=$(installer_get "DI_COMPONENT_PACKAGES")
    local DI_COMPONENT_LANGUAGE=$(installer_get "DI_COMPONENT_LANGUAGE")

    if [ -n "${DI_COMPONENT_PACKAGES}" ];then
        # install component
        info "component packages: $DI_COMPONENT_PACKAGES"
        install_package ${DI_COMPONENT_PACKAGES} || \
            warning "Failed to install packages: ${DI_COMPONENT_PACKAGES}"
    fi

    if [ -n "${DI_COMPONENT_LANGUAGE}" ];then
        info "component language: $DI_COMPONENT_LANGUAGE"
        install_package ${DI_COMPONENT_LANGUAGE} || \
            warning "Failed to install packages: " ${DI_COMPONENT_LANGUAGE}
    fi
}

install_app_deb_package() {
    info "install_app_deb_package"
    ## 安装自定义包
    local APP_INFO="/etc/deepin-installer/app-info.ini"
    local COUNT=$(deepin-installer-config get "${APP_INFO}" "count" 2>/dev/null)
    [ -z "$COUNT" ] && COUNT=0
    info "COUNT=$COUNT"
    for ((i=0;i<${COUNT};i++))
    do
        APP_NAME=$(deepin-installer-config get "${APP_INFO}" "App${i}" "appName" 2>/dev/null)
        APP_PATH=$(deepin-installer-config get "${APP_INFO}" "App${i}" "appPath" 2>/dev/null)
        info "APP_NAME=$APP_NAME"
        info "APP_PATH=$APP_PATH"

        if [ -z "$APP_PATH" ]; then
        	warning "app path is empty"
            continue
        fi

        # 如果APP_PATH目录不存在则跳过
        if [ ! -d "$APP_PATH" ]; then
        	warning "app path is not exist:$APP_PATH"
            continue
        fi

        # 判断目录下是否存在deb包
        local DEB_PKGS=$(echo ${APP_PATH}/*.deb)
        if [ "${DEB_PKGS}" = "${APP_PATH}/*.deb" ]; then
            continue
        fi

        if install_all_package $APP_PATH; then
        	deepin-installer-config set ${APP_INFO} "App${i}" "installed" "true"
        else
        	deepin-installer-config set ${APP_INFO} "App${i}" "installed" "false"
        fi
    done
}

install_app_uab_package() {
    info "install_app_uab_package"
    ## 安装自定义包
    local APP_INFO="/etc/deepin-installer/app-info.ini"
    local COUNT=$(deepin-installer-config get "${APP_INFO}" "count" 2>/dev/null)
    [ -z "$COUNT" ] && COUNT=0
    info "COUNT=$COUNT"
    for ((i=0;i<${COUNT};i++))
    do
        APP_NAME=$(deepin-installer-config get "${APP_INFO}" "App${i}" "appName" 2>/dev/null)
        APP_PATH=$(deepin-installer-config get "${APP_INFO}" "App${i}" "appPath" 2>/dev/null)
        info "APP_NAME=$APP_NAME"
        info "APP_PATH=$APP_PATH"

        if [ -z "$APP_PATH" ]; then
        	  warning "app path is empty"
            continue
        fi

        # 如果APP_PATH目录不存在则跳过
        if [ ! -d "$APP_PATH" ]; then
        	  warning "app path is not exist:$APP_PATH"
            continue
        fi

        # 判断目录下是否存在uab包
        local UAB_PKGS=$(echo ${APP_PATH}/*.uab)
        if [ "${UAB_PKGS}" = "${APP_PATH}/*.uab" ]; then
            continue
        fi

        if install_all_uab_package $APP_PATH; then
        	deepin-installer-config set ${APP_INFO} "App${i}" "installed" "true"
        else
        	deepin-installer-config set ${APP_INFO} "App${i}" "installed" "false"
        fi
    done
}


install_all_uab_package() {
    info "install_all_uab_package"
    local oem_dir=$1
    if ! is_empty_dir $oem_dir; then
        local pkgs=$(echo ${oem_dir}/*.uab)
        if [ "${pkgs}" = "${oem_dir}/*.uab" ]; then
            return 0
        fi
        #遍历pkgs
        for pkg in $pkgs ; do
            echo "pkg=${pkg}"
            ll-cli --no-dbus install "$pkg" || warning "Failed to install packages: ${pkg}"
        done
    fi
}


setup_apt_conf() {
    info "manage /etc/apt/apt.conf 00deepin-installer"
    mkdir -vp /etc/apt/apt.conf.d/
    cat > /etc/apt/apt.conf.d/00deepin-installer <<EOF
APT::Install-Recommends "false";
APT::Get::AllowUnauthenticated "true";
Acquire::AllowInsecureRepositories "true";
Acquire::Check-Date "false";
EOF
}

remove_apt_conf() {
    rm -f /target/etc/apt/apt.conf.d/00deepin-installer
}

remove_local_repository() {
    if [ -e "/target/etc/apt/sources.list.d.bak" ]; then
        if [ -e "/target/etc/apt/sources.list.d" ]; then
            rm -rvf /target/etc/apt/sources.list.d
            mv /target/etc/apt/sources.list.d.bak /target/etc/apt/sources.list.d
        fi
    fi

    if [ -e "/target/etc/apt/sources.list.d/deepin-installer.list" ]; then
        rm -f /target/etc/apt/sources.list.d/deepin-installer.list
    fi
}


setup_edition_repo_sources() {
    ## 创建授权切换版本需要的环境
    local  pro_edu_ppa_etc_path="/etc/deepin/lastore-daemon/pro-edu-ppa"
    local  pro_edu_ppa_lists_path="/var/lib/pro-edu-ppa/lists"
    local  pro_edu_ppa_cache_path="/var/cache/pro-edu-ppa"

    mkdir -p ${pro_edu_ppa_etc_path} || warning "mkdir -p ${pro_edu_ppa_etc_path} failed!"
    mkdir -p ${pro_edu_ppa_lists_path} || warning "mkdir -p ${pro_edu_ppa_lists_path} failed!"
    mkdir -p ${pro_edu_ppa_cache_path} || warning "mkdir -p ${pro_edu_ppa_cache_path} failed!"

    local EDU_REPO_PATH="/media/pro-edu-ppa"
    local EDU_REPO_DIST_PATH=${EDU_REPO_PATH}/dists
    if [ -d ${EDU_REPO_DIST_PATH} ]; then
        dir=$(find ${EDU_REPO_DIST_PATH} -name "Release" | xargs awk -F '[ :]+' '/Codename/{print $2}')
        for name in $dir; do
            # 创建授权切版本需要的sourcelist
            echo "deb [trusted=yes] file:${EDU_REPO_PATH} ${name} main contrib non-free" >> /etc/deepin/lastore-daemon/pro-edu-ppa/sources.list
        done
        # 更新授权切换版本需要的apt索引
        apt-get update -o Dir::Etc::SourceParts=/dev/null -o Dir::Etc::SourceList=${pro_edu_ppa_etc_path}/sources.list -o Dir::State::lists=${pro_edu_ppa_lists_path} -o Dir::Cache::archives=${pro_edu_ppa_cache_path} || true
    else
        warning "${EDU_REPO_PATH} is not a debian repository. Skipped."
    fi
}

copy_edition_repository() {
    local eduRepositoryName="pro-edu-ppa"
    local mediaPath="/target/media"

    # 同步版本切换仓库到目标系统
    local repositoryPath="${DI_LIVE_DIR_ENV}/${eduRepositoryName}"
    if [ -e "${repositoryPath}" ]; then
        # 本地不存在版本切换仓库目录则创建
        if [ ! -e "${mediaPath}/${eduRepositoryName}" ]; then
            mkdir -p "${mediaPath}/${eduRepositoryName}" || error "mkdir -p ${mediaPath}/${eduRepositoryName} failed!"
        fi
        rsync -aHXA "${repositoryPath}" "${mediaPath}" || error "rsync -aHXA ${repositoryPath} ${mediaPath} failed!"
    else
        warning "${repositoryPath} no found!"
    fi
}

install_Edition_Package() {
    local osVersionType=$(installer_get "DI_EDITION_NAME")
    local versionDeb=""
    if [ "x$osVersionType" = "x" ]; then
        error "DI_EDITION_NAME:${osVersionType} should not be empty!"
    elif [ "x$osVersionType" = "xE" ]; then
        versionDeb="deepin-edu-base"
        info "DI_EDITION_NAME:${osVersionType}, deepin-edu-base will be install!"
    elif [ "x$osVersionType" = "xProfessional" ]; then
        info "DI_EDITION_NAME:${osVersionType}, nothing to install!"
    else
        warning "DI_EDITION_NAME:${osVersionType} not support, nothing to install!"
        return
    fi

    if [ -n "${versionDeb}" ]; then
        # 版本仓库的环境
        local pro_edu_ppa_etc_path="/etc/deepin/lastore-daemon/pro-edu-ppa"
        local pro_edu_ppa_lists_path="/var/lib/pro-edu-ppa/lists"
        local pro_edu_ppa_cache_path="/var/cache/pro-edu-ppa"
        local result=0
        DEBIAN_FRONTEND="noninteractive" apt-get -y install ${versionDeb}\
            --no-install-recommends \
            --allow-unauthenticated \
            -o Dpkg::Options::="--force-confdef" \
            -o Dpkg::Options::="--force-confold" \
            -o Dir::Etc::SourceParts=/dev/null \
            -o Dir::Etc::SourceList=${pro_edu_ppa_etc_path}/sources.list \
            -o Dir::State::lists=${pro_edu_ppa_lists_path} \
            -o Dir::Cache::archives=${pro_edu_ppa_cache_path} || result=$?

        if [ ${result} -eq 0 ]; then
            info "Install ${versionDeb} Succeed !"
        else
            error "Install ${versionDeb} Failed !"
        fi
    fi
}