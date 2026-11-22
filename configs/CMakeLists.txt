install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/org.fedoradde.installer"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/dsg/configs")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/settings"
    DESTINATION "${INSTALL_TARGET}/configs"
    PATTERN "CMakeLists.txt" EXCLUDE PATTERN ".gitkeep" EXCLUDE)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/slides/Fedora/"
    DESTINATION "${INSTALL_TARGET}/configs/slides/Fedora")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/branding/fedora-dde-installer.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps")
# Fedora KIWI/dracut manages the Live boot. Do not install Debian live-boot hooks,
# deepin-version overrides, display-manager replacement units or first-boot jobs.
install(FILES "${PROJECT_SOURCE_DIR}/packaging/fedora-dde-installer.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
install(FILES "${PROJECT_SOURCE_DIR}/packaging/org.fedoradde.installer.policy"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/polkit-1/actions")
install(PROGRAMS "${PROJECT_SOURCE_DIR}/packaging/fedora-dde-installer-launch"
    DESTINATION "${CMAKE_INSTALL_BINDIR}")
