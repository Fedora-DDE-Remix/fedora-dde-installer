install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/org.deepin.installer"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dsg/configs/
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN ".gitkeep" EXCLUDE
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/settings"
    DESTINATION ${INSTALL_TARGET}/configs
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN ".gitkeep" EXCLUDE
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/slides/${INSTALLER_BUILD_OSVERSION}/"
    DESTINATION ${INSTALL_TARGET}/configs/slides
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN ".gitkeep" EXCLUDE
)

install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/slides/uos-installtool.svg"
    DESTINATION ${INSTALL_TARGET}/configs/slides
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/live-boot/
    DESTINATION /lib/live/boot
    FILES_MATCHING PATTERN "*"
    PERMISSIONS OWNER_READ OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
)

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/live-config/deepin-installer-live.conf
    DESTINATION /etc/live/config.conf.d/
)

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/live-config/deepin-installer.desktop
          ${CMAKE_CURRENT_SOURCE_DIR}/live-config/deepin-installer_deepin.desktop
          ${CMAKE_CURRENT_SOURCE_DIR}/live-config/deepin-installer-live
    DESTINATION ${INSTALL_TARGET}/configs/live-config
    PERMISSIONS OWNER_READ OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/os"
    DESTINATION ${INSTALL_TARGET}/configs/
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN ".gitkeep" EXCLUDE
)
