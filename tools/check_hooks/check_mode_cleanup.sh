#!/bin/bash

## 初始化环境变量
source /usr/share/deepin-installer/tools/scripts/init_environment.sh

## 导入函数库
source $DI_INSTALL_TOOLS_DIR_ENV/scripts/function_include.sh

## 初始化日志
LOG_FILE="/var/log/deepin-installer/deepin-installer-check-mode.log"

main() {
    if is_check_mode; then
        ## 执行after_check脚本
        after_check

        if [ x"$(installer_get DI_CHECK_MODE_HOLD)" != "xtrue" ]; then
            ## 执行审核模式的清理动作
            cleanup_check_mode
        fi

        ## 重启
        reboot
    fi
}

main >> $LOG_FILE 2>&1
