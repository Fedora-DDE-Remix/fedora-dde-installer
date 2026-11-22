# Compile translations without mutating source TS files during configure/build.
# Accept the upstream filenames while giving every installed catalog our name.
file(GLOB INSTALLER_TRANSLATION_TS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/deepin-installer-*.ts"
    "${CMAKE_CURRENT_SOURCE_DIR}/fedora-dde-installer-*.ts")
set(INSTALLER_TRANSLATION_QM)
foreach(ts IN LISTS INSTALLER_TRANSLATION_TS)
    get_filename_component(stem "${ts}" NAME_WE)
    string(REGEX REPLACE "^deepin-installer-" "fedora-dde-installer-" stem "${stem}")
    set(qm "${CMAKE_CURRENT_BINARY_DIR}/${stem}.qm")
    add_custom_command(OUTPUT "${qm}"
        COMMAND Qt6::lrelease "${ts}" -qm "${qm}"
        DEPENDS "${ts}" VERBATIM)
    list(APPEND INSTALLER_TRANSLATION_QM "${qm}")
endforeach()
add_custom_target(installer-qm-update ALL DEPENDS ${INSTALLER_TRANSLATION_QM})
install(FILES ${INSTALLER_TRANSLATION_QM} DESTINATION "${INSTALL_TARGET}/i18n")
