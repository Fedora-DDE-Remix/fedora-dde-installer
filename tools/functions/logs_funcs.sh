#!/bin/bash

init_log() {
    local file=$1
    mkdir -p $(get_file_path $file)
    [ -f $file ] && mv $file $file-$(date '+%Y-%m-%d-%H-%M-%S')
    touch $file
}

 # 定义日志输出函数
info() {
    local str=$@
    echo [$(date '+%Y-%m-%d %H:%M:%S')][info] "$str"
}

error() {
    local str=$@
    echo [$(date '+%Y-%m-%d %H:%M:%S')][error] "$str"
    exit 1
}

warning() {
    local str=$@
    echo [$(date '+%Y-%m-%d %H:%M:%S')][warning] "$str"
}

print_system_info() {
    ## 打印环境变量
    info "$(env)"
    # 打印系统版本信息
    info "$(cat /etc/deepin-version)"
    # 打印系统架构信息
    info "$(arch)"
    # 打印分区相关信息
    #info "$(echo $(lsblk -OJ) | jq '.')"
}

install_log() {
    local LOG_DIR="/var/log/deepin-installer/"
    mkdir -p /target/$LOG_DIR
    cp -vrf $LOG_DIR/* /target/$LOG_DIR
}

crypt_file() {
    local file=$1
    touch $file.tmp && chmod 666 $file.tmp
    base64 $file > $file.tmp
    mv $file.tmp $file && chmod 600 $file
}

crypt_log() {
    local logDir=$1
    if [ ! -d ${logDir} ]; then
        return 0
    fi

    local logs=$(ls -d "${logDir}"/*)
    logs=$(echo -n "${logs}" | tr -s '\n' ' ')

    for file in ${logs} ; do {
        crypt_file ${file}
    }
    done
}

install_status() {
    local state=$1
    installer_set DI_INSTALLER_STATUS "$state"
}
