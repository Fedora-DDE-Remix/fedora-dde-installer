#!/bin/bash

## 初始化环境变量
source /usr/share/deepin-installer/tools/scripts/init_environment.sh

## 导入函数库
source $DI_INSTALL_TOOLS_DIR_ENV/scripts/function_include.sh

## 初始化日志
LOG_FILE="/var/log/deepin-installer/deepin-installer-check-mode.log"



USER_PASSWORD=$(installer_get "DI_CM_PASSWORD")
echo ${USER_PASSWORD} | sudo -S deepin-installer-config set "${DI_CONFIG_FILE_ENV}"  "DI_CHECK_MODE_HOLD" "false"

qdbus --session --literal  org.deepin.dde.SessionManager1 /org/deepin/dde/SessionManager1  org.deepin.dde.SessionManager1.RequestLogout 
