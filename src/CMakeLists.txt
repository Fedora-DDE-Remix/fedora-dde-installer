# 增加源文件
file(GLOB BASES_FILES bases/*.h bases/*.cpp)
file(GLOB RESOURCE_FILES resources/images/images.qrc)
file(GLOB COMMONS_FILES commons/*.h commons/*.cpp)
file(GLOB SERVICES_FILES services/*.h services/*.cpp services/inner/*.h services/inner/*.cpp)
file(GLOB WIDGETS_FILES widgets/*.h widgets/*.cpp)
file(GLOB INTERFACES_FILES interfaces/*.h interfaces/*.cpp)
file(GLOB TESTS_FILES tests/*.h tests/*.cpp)

# 增加头文件和库
include_directories(bases commons services widgets interfaces tests)


#　前配置安装模块
add_executable(deepin-installer
    apps/deepin_installer.cpp
    "${COMMONS_FILES}"
    "${SERVICES_FILES}"
    "${RESOURCE_FILES}"
    "${WIDGETS_FILES}"
    "${INTERFACES_FILES}"
    "${BASES_FILES}")
target_link_libraries(deepin-installer ${LINK_LIBS})
add_dependencies(deepin-installer installer-qm-update)

## 试用模式配置模块
add_executable(deepin-installer-live-config
        apps/deepin_installer_live_config.cpp
        "${COMMONS_FILES}"
        "${SERVICES_FILES}"
        "${RESOURCE_FILES}"
        "${WIDGETS_FILES}"
        "${INTERFACES_FILES}"
        "${BASES_FILES}")
target_link_libraries(deepin-installer-live-config ${LINK_LIBS})
add_dependencies(deepin-installer-live-config installer-qm-update)

## 后配置模块
add_executable(deepin-installer-first-boot
    apps/deepin_installer_first_boot.cpp
    "${COMMONS_FILES}"
    "${SERVICES_FILES}"
    "${RESOURCE_FILES}"
    "${WIDGETS_FILES}"
    "${INTERFACES_FILES}"
    "${BASES_FILES}")
target_link_libraries(deepin-installer-first-boot ${LINK_LIBS})
add_dependencies(deepin-installer-first-boot installer-qm-update)

## 分区管理工具
add_executable(deepin-installer-parted
    apps/deepin_installer_parted.cpp
    "${COMMONS_FILES}"
    "${SERVICES_FILES}"
    "${INTERFACES_FILES}"
    "${BASES_FILES}")
target_link_libraries(deepin-installer-parted ${LINK_LIBS})

## 配置管理工具
add_executable(deepin-installer-config
    apps/deepin_installer_config.cpp)
target_link_libraries(deepin-installer-config Qt6::Core)

add_executable(deepin-installer-parted-util
    apps/deepin_installer_parted_util.cpp
    "${COMMONS_FILES}"
    "${BASES_FILES}"
    "${SERVICES_FILES}"
)
target_link_libraries(deepin-installer-parted-util ${LINK_LIBS})

## 单元测试
add_executable(deepin-installer-test
    apps/deepin_installer_test.cpp
    "${COMMONS_FILES}"
    "${SERVICES_FILES}"
    "${RESOURCE_FILES}"
    "${WIDGETS_FILES}"
    "${INTERFACES_FILES}"
    "${BASES_FILES}"
    "${TESTS_FILES}")
target_link_libraries(deepin-installer-test ${LINK_LIBS} Qt6::Test)
add_test(deepin-installer-test deepin-installer-test)

add_executable(deepin-installer-command-agent
    apps/deepin_installer_command_agent.cpp
    "${BASES_FILES}"
    "${COMMONS_FILES}"
    "${SERVICES_FILES}"
)
target_link_libraries(deepin-installer-command-agent ${LINK_LIBS})

## 安装目标文件
install(TARGETS
    deepin-installer
    deepin-installer-first-boot
    deepin-installer-live-config
    deepin-installer-parted
    deepin-installer-config
    deepin-installer-parted-util
    deepin-installer-command-agent
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/
)

# 增加子目录
add_subdirectory(modules)
add_dependencies(deepin-installer
        deepin-installer-parted
        deepin-installer-config
        deepin-installer-parted-util
        DeepinConfigProgressFrame
        DeepinInstallerComponentFrame
        DeepinInstallerLanguage
        DeepinInstallerPartition
        DeepinInstallerPartitionFrame
        DeepinInstallerProgressFrame
        DeepinInstallerUserInfoFrame
        DeepinInstallerVerifyFrame
        DeepinInstallerInstallApplicationFrame
        DeepinInstallerPrepareFrame
        DeepinInstallerRegionFormatFrame
        DeepinInstallerSelectOSVersionFrame
        DebugShellFrame
        )

add_dependencies(deepin-installer-first-boot
        deepin-installer-parted
        deepin-installer-config
        deepin-installer-parted-util
        DeepinConfigProgressFrame
        DeepinInstallerComponentFrame
        DeepinInstallerLanguage
        DeepinInstallerPartition
        DeepinInstallerPartitionFrame
        DeepinInstallerProgressFrame
        DeepinInstallerUserInfoFrame
        DeepinInstallerVerifyFrame
        DeepinInstallerInstallApplicationFrame
        DeepinInstallerRegionFormatFrame
        #DebugShellFrame
        )

