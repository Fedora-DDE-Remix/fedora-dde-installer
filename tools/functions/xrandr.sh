#!/bin/bash

# 获取活动屏幕输出个数
monitors_cnt() {
    local out=`xrandr --listactivemonitors | grep ^Monitors:`
    out=${out##Monitors:}
    echo ${out:-0}
}

# 获取主屏幕
monitors_primary() {
    ## 通过前后空格精准匹配
    local out=`xrandr | grep ' connected primary ' | awk '{print $1}'`
    echo -n $out
}

# 获取其他屏幕数据列表
monitors_othe_list() {
    local out1=`xrandr | grep ' connected ' | grep -v 'connected primary' | awk '{print $1}' | grep -v "DisplayPort-"`
    local out2=`xrandr | grep ' connected ' | awk '{print $1}' | grep "DisplayPort-" | awk 'END {print}' | grep -v $(monitors_primary)`
    [ -n "$out1" ] || out2=`xrandr | grep ' connected ' | awk '{print $1}' | grep "DisplayPort-" | grep -v $(monitors_primary)`
    echo -n $out1 $out2
}


# 获取屏幕数据列表
monitors_list() {
    local out=`xrandr | grep ' connected ' | awk '{print $1}'`
    echo -n $out
}

# 处理1k情况，安装器最高以1k显示
monitors_mode_1k() {
    local mode="1920x1080"
    local cnt=`xrandr -q | awk '{print $1}' | grep -E [0-9]+x[0-9]+ | grep -E "^$mode$" | wc -l`
    if [ $cnt -eq $(monitors_cnt) ]; then
        echo $mode
    else
        echo ""
    fi
}

# 获取所有屏幕共同支持的显示模式
monitors_mode() {
   local mode_1k=$(monitors_mode_1k)
   ## 如果支持1k，最以1k屏显示
   if [ -n "$mode_1k" ]; then
       echo $mode_1k
   else
       local list=`xrandr -q | awk '{print $1}' | grep -E [0-9]+x[0-9]+`
       for line in $list; do
           local cnt=`xrandr -q | awk '{print $1}' | grep -E [0-9]+x[0-9]+ | grep -E "^$line$" | wc -l`
           if [ $cnt -eq $(monitors_cnt) ]; then
               echo $line
               return;
           fi
       done

       # 最低的默认分辨率，防止如果所有分辨率都不满足的情况下使用800x600
       echo "800x600"
   fi
}

print_log() {
    xrandr
    monitors_list | xargs -d" " -I {} echo "xrandr --output {} --auto --primary"
    monitors_primary | xargs -d" " -I {} echo "xrandr --output {} --mode $(monitors_mode)"
    monitors_othe_list | xargs -d" " -I {} echo "xrandr --output {} --same-as $(monitors_primary) --mode $(monitors_mode)"
    monitors_othe_list | xargs -d" " -I {} echo "xrandr --output {} --mode $(monitors_mode)"
}

# 获取最高分辨率
max_mode() {
    local mode=`xrandr -q | awk '{print $1}' | grep -E '[0-9]+x[0-9]+$' | sed -n 1p`
    echo ${mode}
}

# 获取最优分辨率
optimal_mode() {
    local mode=`xrandr -q | grep + | awk '{print $1}' | grep -E [0-9]+[0-9] | sed -n 1p`
    echo ${mode}
}

start_screen_manager() {
    info "start screen manager..."

    #acquireManager
    local dconfig_resource_path=$(dbus-send --system --type=method_call --print-reply=literal --dest=org.desktopspec.ConfigManager / \
    org.desktopspec.ConfigManager.acquireManager \
    string:'org.deepin.installer' string:'org.deepin.installer' string:'')'' || true
    echo "dconfig_resource_path=${dconfig_resource_path}"


    local restartMonitor=$(dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager \
    $dconfig_resource_path org.desktopspec.ConfigManager.Manager.value string:'restartMonitor') || true
    echo "restartMonitor=${restartMonitor}" 

    local pri_mon=$(monitors_primary)
    if [[ $restartMonitor =~ "true" ]]; then
        if [[ "${pri_mon}" =~ "VGA" ]]; then
            echo "nothing to do with VGA"
        else
            echo "restart monitor!"
            xrandr --output $pri_mon --off
            xrandr --output $pri_mon --auto
            chvt 2
            chvt 1
	    fi
    fi

#    if [ ! -n "$pri_mon" ];then
#        # 设置主屏幕，兼容都是dpi输出且输出屏幕位置重合的情况,兼容无主屏输出的情况
#        monitors_list | xargs -d" " -I {} xrandr --output {} --auto --primary || xrandr
#    fi
#
#    local cnt=$(monitors_cnt)
#    if [ $cnt -gt 1 ]; then
#        # 设置主屏分辨率
#        monitors_primary | xargs -d" " -I {} xrandr --output {} --mode $(monitors_mode)
#        # 克隆模式
#        monitors_othe_list | xargs -d" " -I {} xrandr --output {} --same-as $(monitors_primary) --mode $(monitors_mode)
#        # 设置多屏共同支持的最大分辨率
#        monitors_othe_list | xargs -d" " -I {} xrandr --output {} --mode $(monitors_mode)
#
#        ## 输出调试日志
#        print_log
#    else
#        info "Single output"
#        local opt_mode=$(optimal_mode)
#        if [ -n "${opt_mode}" ]; then
#            # 设置屏幕的最优分辨率
#            info "optimal mode:${opt_mode}"
#            xrandr --output ${pri_mon} --mode ${opt_mode}
#        else
#            # 设置屏幕的最高分辨率
#            info "max mode:$(max_mode)"
#            xrandr --output ${pri_mon} --mode $(max_mode)
#        fi
#    fi
}

setup_kwin_env() {
    export HOME=/root
    export XDG_CURRENT_DESKTOP=Deepin
    #识别到景嘉微显卡屏蔽窗口特效
    #lshw -c video | grep "driver=mwv206"
    #if [ $? -eq 0 ]; then
    #    export KWIN_COMPOSE=N
    #fi

    env
}

setup_kwin_blur() {
    while true; do
        /usr/lib/qt6/bin/qdbus org.kde.KWin /Effects unloadEffect blur
        if [ $? -eq 0 ]; then
            /usr/lib/qt6/bin/qdbus org.kde.KWin /Effects loadEffect blur
            break;
        fi
    done
}

start_kwin() {
    if [ -f /usr/bin/kwin_no_scale ]; then
        /usr/bin/kwin_no_scale
    elif [ -f /usr/bin/deepin-kwin_x11 ]; then
        /usr/bin/deepin-kwin_x11
    else
        /usr/bin/kwin_x11
    fi
}

setup_kwin() {
    setup_kwin_env
    start_kwin || true &
    #setup_kwin_blur || true &
}
