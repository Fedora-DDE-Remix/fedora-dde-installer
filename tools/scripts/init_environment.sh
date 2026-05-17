#!/bin/bash

# 安装器中使用的最基本的文件变量，其他的配置选项请使用安装器的配置配置实现
## 安装器包安装的资源目录
export DI_HOME_BIN_ENV=/usr/bin/
export DI_HOME_DIR_ENV=/usr/share/deepin-installer
export DI_TOOLS_DIR_ENV=$DI_HOME_DIR_ENV/tools
export DI_CONFIG_DIR_ENV=$DI_HOME_DIR_ENV/configs
export DI_SRC_DIR_ENV=$DI_HOME_DIR_ENV/src
export DI_MODULE_DIR_ENV=$DI_HOME_DIR_ENV/modules
export DI_CONFIG_FILE_ENV=/etc/deepin-installer/deepin-installer.conf
# 兼容以前定制的脚本，保留以下变量
export DI_INSTALL_DIR_ENV=$DI_HOME_DIR_ENV
export DI_INSTALL_TOOLS_DIR_ENV=$DI_HOME_DIR_ENV/tools
export DI_INSTALL_MODULE_DIR_ENV=/usr/lib/deepin-installer/modules

# LIVE下u盘目录
export DI_LIVE_DIR_ENV=/usr/lib/live/mount/medium/
export DI_OEM_DIR=$DI_LIVE_DIR_ENV/oem/
export DI_TARGET_LIVE_DIR_ENV=/media/live/mount/medium

export DI_OS_VERSION_ENV="V$(cat /etc/deepin-version | grep Version | awk -F= '{print $2}')"
export DI_REPO_BASE_NAME_ENV=$(cat /etc/immutable.conf/base-name)

echo "DI_REPO_BASE_NAME_ENV=$DI_REPO_BASE_NAME_ENV"
echo "DI_HOME_BIN_ENV=$DI_HOME_BIN_ENV"
echo "DI_HOME_DIR_ENV=$DI_HOME_DIR_ENV"
echo "DI_TOOLS_DIR_ENV=$DI_TOOLS_DIR_ENV"
echo "DI_CONFIG_DIR_ENV=$DI_CONFIG_DIR_ENV"
echo "DI_SRC_DIR_ENV=$DI_SRC_DIR_ENV"
echo "DI_MODULE_DIR_ENV=$DI_MODULE_DIR_ENV"
echo "DI_CONFIG_FILE_ENV=$DI_CONFIG_FILE_ENV"
echo "DI_INSTALL_DIR_ENV=$DI_INSTALL_DIR_ENV"
echo "DI_INSTALL_TOOLS_DIR_ENV=$DI_INSTALL_TOOLS_DIR_ENV"
echo "DI_INSTALL_MODULE_DIR_ENV=$DI_INSTALL_MODULE_DIR_ENV"
echo "DI_LIVE_DIR_ENV=$DI_LIVE_DIR_ENV"
echo "DI_OEM_DIR=$DI_OEM_DIR"
echo "DI_OS_VERSION_ENV=$DI_OS_VERSION_ENV"
echo "DI_TARGET_LIVE_DIR_ENV=$DI_TARGET_LIVE_DIR_ENV"
