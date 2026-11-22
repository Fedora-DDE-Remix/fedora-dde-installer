file(GLOB BASES_FILES CONFIGURE_DEPENDS bases/*.h bases/*.cpp)
file(GLOB COMMONS_FILES CONFIGURE_DEPENDS commons/*.h commons/*.cpp)
file(GLOB SERVICES_FILES CONFIGURE_DEPENDS services/*.h services/*.cpp services/inner/*.h services/inner/*.cpp)
file(GLOB WIDGETS_FILES CONFIGURE_DEPENDS widgets/*.h widgets/*.cpp)
file(GLOB INTERFACES_FILES CONFIGURE_DEPENDS interfaces/*.h interfaces/*.cpp)

# Share the same manager instances and symbols between the application and its
# dynamically loaded plugins. Do not rely on symbols exported by an executable.
add_library(FedoraDDEInstallerCore SHARED
    ${BASES_FILES} ${COMMONS_FILES} ${SERVICES_FILES} ${WIDGETS_FILES}
    ${INTERFACES_FILES} resources/images/images.qrc)
set_target_properties(FedoraDDEInstallerCore PROPERTIES
    OUTPUT_NAME fedora-dde-installer-common VERSION ${PROJECT_VERSION} SOVERSION 0)
target_include_directories(FedoraDDEInstallerCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/bases ${CMAKE_CURRENT_SOURCE_DIR}/commons
    ${CMAKE_CURRENT_SOURCE_DIR}/services ${CMAKE_CURRENT_SOURCE_DIR}/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaces)
target_link_libraries(FedoraDDEInstallerCore PUBLIC ${LINK_LIBS})

# Release builds resolve settings through environment variables, so the real
# planner can be exercised with an isolated temporary configuration. Debug
# builds intentionally pin the live installer configuration path instead.
if(BUILD_TESTING AND CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
    add_executable(installer-partition-layout-test tests/PartitionLayoutTest.cpp
        modules/task_partitions/PartitionTaskManager.cpp)
    target_include_directories(installer-partition-layout-test PRIVATE modules/task_partitions)
    target_link_libraries(installer-partition-layout-test PRIVATE FedoraDDEInstallerCore)
    add_test(NAME installer-partition-layout
        COMMAND installer-partition-layout-test ${CMAKE_SOURCE_DIR})
endif()

install(TARGETS FedoraDDEInstallerCore
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} NAMELINK_SKIP)
install(FILES resources/timezone_alias
    DESTINATION "${INSTALL_TARGET}/src/resources")

function(installer_application suffix source)
    if(suffix STREQUAL "main")
        set(target fedora-dde-installer)
    else()
        set(target fedora-dde-installer-${suffix})
    endif()
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE FedoraDDEInstallerCore)
    install(TARGETS ${target} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()

installer_application(main apps/deepin_installer.cpp)
installer_application(parted apps/deepin_installer_parted.cpp)
installer_application(parted-util apps/deepin_installer_parted_util.cpp)

add_executable(fedora-dde-installer-config apps/deepin_installer_config.cpp)
target_include_directories(fedora-dde-installer-config PRIVATE commons)
target_link_libraries(fedora-dde-installer-config PRIVATE Qt6::Core)
install(TARGETS fedora-dde-installer-config RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
add_dependencies(fedora-dde-installer installer-qm-update fedora-dde-installer-config
    fedora-dde-installer-parted fedora-dde-installer-parted-util)

# All modules link to the shared core; their established names are the plugin ABI.
set(LINK_LIBS FedoraDDEInstallerCore)
add_subdirectory(modules)
add_dependencies(fedora-dde-installer
    DeepinConfigProgressFrame DeepinInstallerLanguage DeepinInstallerPartition
    DeepinInstallerPartitionFrame DeepinInstallerProgressFrame
    DeepinInstallerUserInfoFrame DeepinInstallerVerifyFrame
    DeepinInstallerPrepareFrame DeepinInstallerRegionFormatFrame)
