# 工程设置
cmake_minimum_required(VERSION 3.1)
project(deepin-installer C CXX)

set(CMAKE_CXX_STANDARD_REQUIRED 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(INSTALL_TARGET "/usr/share/deepin-installer")
set(ENV{LC_ALL} "C.UTF-8")

enable_testing()

# 开启cppcheck检查
find_program(CPPCHECK_BIND NAMES cppcheck)
if (CPPCHECK_BIND)
    message(status ${CPPCHECK_BIND})
    list(APPEND CMAKE_CXX_CPPCHECK
        "${CPPCHECK_BIND}"
        "--enable=all"
        "--inconclusive"
        "--template=gcc"
        "--language=c++"
        "--std=c++11"
        "${CMAKE_CXX_CPPCHECK}/src"
    )
endif()

# 启用qt moc机制
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    # 默认优化编译参数
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O0 ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O0 -Wno-unused-result -Wno-switch-default -Wno-padded")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-reorder -Werror")
    # 安全编译参数
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -z noexecstack -pie -fPIC -z relro -z now -fstack-protector-all")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -z noexecstack -pie -fPIC -z relro -z now -fstack-protector-all")

    # Enable Qt builtin debug mode
    add_definitions("-DQT_MESSAGELOGCONTEXT")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -z noexecstack -pie -fPIC -z relro -z now -fstack-protector-all")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -z noexecstack -pie -fPIC -z relro -z now -fstack-protector-all")

endif()

set(INSTALLER_BUILD_ARCH "")
execute_process(COMMAND uname -m
                OUTPUT_VARIABLE INSTALLER_BUILD_ARCH)
string(REGEX REPLACE "\n$" "" INSTALLER_BUILD_ARCH "${INSTALLER_BUILD_ARCH}")
message("build arch: ${INSTALLER_BUILD_ARCH}")

# 调试时指定INSTALLER_BUILD_OSVERSION的版本，如V20、V23
set(INSTALLER_BUILD_OSVERSION "")
if (NOT INSTALLER_BUILD_OSVERSION)
    execute_process(COMMAND bash -c "cat /usr/lib/os-release | grep VERSION_ID | awk -F= '{print $2}'"
        OUTPUT_VARIABLE INSTALLER_BUILD_OSVERSION
    )
    string(REGEX REPLACE "\n$" "" INSTALLER_BUILD_OSVERSION "${INSTALLER_BUILD_OSVERSION}")
    string(REGEX REPLACE "\"" "" INSTALLER_BUILD_OSVERSION "${INSTALLER_BUILD_OSVERSION}")
    set(INSTALLER_BUILD_OSVERSION "V${INSTALLER_BUILD_OSVERSION}")
endif()


message("OSVERSION: ${INSTALLER_BUILD_OSVERSION}")


set(INSTALLER_BUILD_TYPE "")
execute_process(COMMAND bash -c "cat /etc/deepin-version | grep Type= | awk -F= '{print $2}'"
                OUTPUT_VARIABLE INSTALLER_BUILD_TYPE)
string(REGEX REPLACE "\n$" "" INSTALLER_BUILD_TYPE "${INSTALLER_BUILD_TYPE}")
message("OSType: ${INSTALLER_BUILD_TYPE}")

add_definitions("-DDI_PLATFORM_BUILD_ARCH_ENV=\"${INSTALLER_BUILD_ARCH}\"")
if (CMAKE_BUILD_TYPE MATCHES Debug)
    # 调式
    set(DI_CONFIG_FILE "/tmp/deepin-installer/deepin-installer.conf")
    add_definitions("-DDI_HOME_DIR_ENV=\"${CMAKE_SOURCE_DIR}/\"")
    add_definitions("-DDI_INSTALL_TOOLS_DIR_ENV=\"${CMAKE_SOURCE_DIR}/tools/\"")
    add_definitions("-DDI_CONFIG_DIR_ENV=\"${CMAKE_SOURCE_DIR}/configs/\"")
    add_definitions("-DDI_SRC_DIR_ENV=\"${CMAKE_SOURCE_DIR}/src/\"")
    add_definitions("-DDI_INSTALL_MODULE_DIR_ENV=\"${CMAKE_BINARY_DIR}/modules/\"")
    add_definitions("-DDI_CONFIG_FILE_ENV=\"${DI_CONFIG_FILE}\"")
    add_definitions("-DDI_LIVE_DIR_ENV=\"/tmp/test\"")
    add_definitions("-DDI_I18N_DIR_ENV=\"${CMAKE_BINARY_DIR}/i18n\"")
    add_definitions("-DDI_OS_VERSION_ENV=\"${INSTALLER_BUILD_OSVERSION}\"")

    message("${CMAKE_SOURCE_DIR}  ${CMAKE_BINARY_DIR}")
else()
    # 占位宏
    add_definitions("-DDI_HOME_DIR_ENV=\"DI_HOME_DIR_ENV\"")
    add_definitions("-DDI_INSTALL_TOOLS_DIR_ENV=\"DI_INSTALL_TOOLS_DIR_ENV\"")
    add_definitions("-DDI_CONFIG_DIR_ENV=\"DI_CONFIG_DIR_ENV\"")
    add_definitions("-DDI_SRC_DIR_ENV=\"DI_SRC_DIR_ENV\"")
    add_definitions("-DDI_INSTALL_MODULE_DIR_ENV=\"DI_INSTALL_MODULE_DIR_ENV\"")
    add_definitions("-DDI_CONFIG_FILE_ENV=\"DI_CONFIG_FILE_ENV\"")
    add_definitions("-DDI_LIVE_DIR_ENV=\"DI_LIVE_DIR_ENV\"")
    add_definitions("-DDI_I18N_DIR_ENV=\"${INSTALL_TARGET}/i18n\"")
    add_definitions("-DDI_OS_VERSION_ENV=\"DI_OS_VERSION_ENV\"")
endif()

# 库设置
find_package(Qt6Core REQUIRED)
find_package(Qt6Gui REQUIRED)
find_package(Dtk6CMake REQUIRED)
find_package(Dtk6Widget REQUIRED)
find_package(Dtk6Gui REQUIRED)
find_package(Qt6Widgets REQUIRED)

#find_package(Qt6Network REQUIRED)
#find_package(Qt6X11Extras REQUIRED)
find_package(Qt6Xml REQUIRED)
#find_package(Qt6LinguistTools)
#find_package(Qt6Svg REQUIRED)
find_package(Qt6DBus REQUIRED)
#find_package(QTermWidget5 REQUIRED)
find_package(Qt6Test REQUIRED)
find_package(Qt6Concurrent REQUIRED)

find_package(PkgConfig REQUIRED) # 使用pkg_search_module需要先加载PkgConfig
pkg_search_module(UDisk2 REQUIRED udisks2-qt6)
include_directories(AFTER ${UDisk2_INCLUDE_DIRS})
pkg_search_module(Parted REQUIRED libparted)
include_directories(AFTER ${Parted_INCLUDE_DIRS})
pkg_search_module(DeepinPwCheck REQUIRED libdeepin_pw_check)
include_directories(AFTER ${DeepinPwCheck_INCLUDE_DIRS})

set(QT_LINK_LIBS
    Qt6::Core
    Qt6::Gui
#    Qt6::Network
    Qt6::Widgets
#    Qt6::X11Extras
    Qt6::Xml
#    Qt6::Svg
    Qt6::DBus
    Qt6::Concurrent
)

set(LINK_LIBS
    ${QT_LINK_LIBS}
    ${UDisk2_LIBRARIES}
    ${DtkWidget_LIBRARIES}
    ${Parted_LIBRARIES}
    ${DeepinPwCheck_LIBRARIES}
)


# Import folders
add_subdirectory(i18n)
add_subdirectory(src)
add_subdirectory(configs)
add_subdirectory(tools)
