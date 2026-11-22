foreach(directory IN ITEMS functions scripts hooks)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${directory}"
        DESTINATION "${INSTALL_TARGET}/tools"
        USE_SOURCE_PERMISSIONS
        PATTERN "CMakeLists.txt" EXCLUDE PATTERN "__pycache__" EXCLUDE
        PATTERN "*.pyc" EXCLUDE PATTERN ".gitkeep" EXCLUDE)
endforeach()
install(PROGRAMS
    "${CMAKE_CURRENT_SOURCE_DIR}/fedora-dde-installer-core"
    "${CMAKE_CURRENT_SOURCE_DIR}/fedora-dde-installer-init"
    "${CMAKE_CURRENT_SOURCE_DIR}/fedora-dde-installer-preinit"
    DESTINATION "${INSTALL_TARGET}/tools")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/systemd/fedora-dde-installer.service"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/systemd/system")
