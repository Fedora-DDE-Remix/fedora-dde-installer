cmake_minimum_required(VERSION 3.16)
project(fedora-dde-installer VERSION 0.2.2 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CTest)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(FEDORA_DDE_BUILD_GUI "Build the Qt 6 / DTK 6 installer" ON)
option(FEDORA_DDE_DEBUG_CONSOLE "Build the optional developer terminal plugin" OFF)
set(INSTALL_TARGET "${CMAKE_INSTALL_FULL_DATADIR}/fedora-dde-installer")
# These are private Qt plugins, not system libraries. Keep the path identical on
# x86_64 and aarch64 and to DI_INSTALL_MODULE_DIR_ENV in init_environment.sh.
set(INSTALLER_MODULE_DIR "${CMAKE_INSTALL_PREFIX}/lib/fedora-dde-installer/modules")
set(INSTALLER_BUILD_OSVERSION "Fedora")
set(INSTALLER_BUILD_TYPE "Default")
set(INSTALLER_BUILD_ARCH "${CMAKE_SYSTEM_PROCESSOR}")

if(BUILD_TESTING)
    add_executable(installer-workflow-test src/tests/InstallWorkflowTest.cpp)
    target_include_directories(installer-workflow-test PRIVATE src/commons)
    # The standalone workflow test must retain assertions in Release RPM builds.
    target_compile_options(installer-workflow-test PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-UNDEBUG>)
    add_test(NAME installer-workflow COMMAND installer-workflow-test)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME installer-backend
        COMMAND ${Python3_EXECUTABLE} -m unittest discover
            -s ${CMAKE_SOURCE_DIR}/tests/backend -p "test_*.py" -v)
    set_tests_properties(installer-backend PROPERTIES
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()

# Allow the non-destructive workflow/backend tests to run without a GUI SDK.
if(NOT FEDORA_DDE_BUILD_GUI)
    return()
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Xml DBus Concurrent LinguistTools)
find_package(Dtk6Core REQUIRED)
find_package(Dtk6Gui REQUIRED)
find_package(Dtk6Widget REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(UDisk2 REQUIRED IMPORTED_TARGET udisks2-qt6)
pkg_check_modules(Parted REQUIRED IMPORTED_TARGET libparted)
find_package(ICU REQUIRED COMPONENTS uc i18n)

set(LINK_LIBS
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Xml Qt6::DBus Qt6::Concurrent
    Dtk6::Core Dtk6::Gui Dtk6::Widget
    PkgConfig::UDisk2 PkgConfig::Parted
    ICU::uc ICU::i18n)
add_compile_definitions(DI_PLATFORM_BUILD_ARCH_ENV="${INSTALLER_BUILD_ARCH}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(
        QT_MESSAGELOGCONTEXT
        DI_HOME_DIR_ENV="${CMAKE_SOURCE_DIR}"
        DI_INSTALL_TOOLS_DIR_ENV="${CMAKE_SOURCE_DIR}/tools"
        DI_CONFIG_DIR_ENV="${CMAKE_SOURCE_DIR}/configs"
        DI_SRC_DIR_ENV="${CMAKE_SOURCE_DIR}/src"
        DI_INSTALL_MODULE_DIR_ENV="${CMAKE_BINARY_DIR}/modules"
        DI_CONFIG_FILE_ENV="/run/fedora-dde-installer/installer.conf"
        DI_LIVE_DIR_ENV="/run/initramfs/live"
        DI_I18N_DIR_ENV="${CMAKE_BINARY_DIR}/i18n"
        DI_OS_VERSION_ENV="Fedora")
else()
    # The launcher exports these keys. CommonFunc resolves them through qgetenv.
    foreach(key IN ITEMS DI_HOME_DIR_ENV DI_INSTALL_TOOLS_DIR_ENV DI_CONFIG_DIR_ENV
            DI_SRC_DIR_ENV DI_INSTALL_MODULE_DIR_ENV DI_CONFIG_FILE_ENV DI_LIVE_DIR_ENV
            DI_OS_VERSION_ENV)
        add_compile_definitions(${key}="${key}")
    endforeach()
    add_compile_definitions(DI_I18N_DIR_ENV="${INSTALL_TARGET}/i18n")
endif()

add_subdirectory(i18n)
add_subdirectory(src)
add_subdirectory(configs)
add_subdirectory(tools)
