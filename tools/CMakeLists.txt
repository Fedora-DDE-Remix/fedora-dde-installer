install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/functions"
    DESTINATION ${INSTALL_TARGET}/tools
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN ".gitkeep" EXCLUDE
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scripts"
    DESTINATION ${INSTALL_TARGET}/tools
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN ".gitkeep" EXCLUDE
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/hooks"
    DESTINATION ${INSTALL_TARGET}/tools
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN ".gitkeep" EXCLUDE
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/check_hooks"
    DESTINATION ${INSTALL_TARGET}/tools
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN ".gitkeep" EXCLUDE
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/systemd/"
    DESTINATION /lib/systemd/system/
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN ".gitkeep" EXCLUDE
)

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/deepin-installer-core
          ${CMAKE_CURRENT_SOURCE_DIR}/deepin-installer-first-boot-preinit
          ${CMAKE_CURRENT_SOURCE_DIR}/deepin-installer-init
          ${CMAKE_CURRENT_SOURCE_DIR}/deepin-installer-preinit
          ${CMAKE_CURRENT_SOURCE_DIR}/deepin-installer-extra
        ${CMAKE_CURRENT_SOURCE_DIR}/deepin-repairtool-launch
    DESTINATION ${INSTALL_TARGET}/tools
    PERMISSIONS OWNER_READ OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
)
