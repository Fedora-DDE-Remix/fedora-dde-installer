#!/bin/bash

## 初始化环境变量
source /usr/share/deepin-installer/tools/scripts/init_environment.sh

## 导入函数库
source $DI_INSTALL_TOOLS_DIR_ENV/scripts/function_include.sh

## 初始化日志
LOG_FILE="/var/log/deepin-installer/deepin-installer-check-mode.log"

main() {
    ## 执行in_check脚本
    in_check
}

main >> $LOG_FILE 2>&1
