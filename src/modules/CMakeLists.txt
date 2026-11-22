# Explicitly select the modules used by the Fedora installer. Neither first-boot
# configuration nor Deepin edition/application selection is part of this flow.
foreach(module IN ITEMS
        frame_languages frame_kblayouts frame_timezone frame_regionformat
        frame_partitions task_partitions frame_prepare frame_verify
        frame_progress frame_userinfo frame_config_progress)
    add_subdirectory(${module})
endforeach()
if(FEDORA_DDE_DEBUG_CONSOLE)
    add_subdirectory(frame_debug)
endif()
