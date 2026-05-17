#!/bin/bash

set -e # 开启脚本错误校验，脚本出错后立即结束

#导入函数库
source $DI_INSTALL_TOOLS_DIR_ENV/scripts/function_include.sh

# hooks manager
hooks_manager() {
    local in_file=$1
    local start_time=$(date +%s)

    echo "run $in_file..."
    case ${in_file} in
      */*in_chroot/*|*/user_config/*)
        if [ ! -d "/target" ]; then
            . "${in_file}"
        elif [ -f "$in_file" ]; then
            chroot /target /bin/bash -e "${in_file}"
        fi
        ;;
      *)
        # Run normal hooks.
        [ -f "$in_file" ] && . "${in_file}"
        ;;
    esac

    ## 打印job执行日志
    local end_time=$(date +%s)
    local cost_time=$((end_time-$start_time))
    echo "run ${in_file} time is $(($cost_time/60))m $(($cost_time%60))s"
}

# Run hook file
hooks_manager $1
