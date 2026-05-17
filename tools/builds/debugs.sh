#!/bin/bash

# 定义辅助工程构建脚本的函数

function add_env() {
    local key="$1"
    if [ $(grep -c "$key" ~/.bashrc) -eq 0 ]; then
        echo "export $key" >>  ~/.bashrc
    fi
}

function debug_config() {
    echo "debug_config..."

    local SRC_DIR=$1
    local BUILD_DIR=$2
    local CONFIG_FILE=$3

    rm -vfr $CONFIG_FILE
    $BUILD_DIR/src/deepin-installer-config init $CONFIG_FILE
}

function setup_git_hooks() {
    echo "setup_git_hooks..."

    local SRC_DIR=$1
    local TOOL_DIR=$1/tools/git-hooks
    ls $TOOL_DIR/ | xargs -I {} cp -vrf $TOOL_DIR/{} $SRC_DIR/.git/hooks/
    chmod a+x $SRC_DIR/.git/hook
}

echo $@

#setup_git_hooks $@ # 当前通过CMAKE_CXX_CPPCHECK的方式检查
debug_config $@


