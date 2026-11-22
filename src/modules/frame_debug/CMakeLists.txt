set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/modules)
# 增加源文件

file(GLOB SRC_FILES *.h *.cpp)
find_package(QTermWidget6)
if (QTermWidget6_FOUND)
    message("QTermWidget6: ${QTermWidget6_FOUND} ${QTERMWIDGET_INCLUDE_DIRS}")
    add_library(DebugShellFrame SHARED "${SRC_FILES}" "${INTERFACE_FILES}")
    target_include_directories(DebugShellFrame PUBLIC ${QTERMWIDGET_INCLUDE_DIRS})
    target_link_libraries(DebugShellFrame ${LINK_LIBS} qtermwidget6)

    install(TARGETS DebugShellFrame
            LIBRARY DESTINATION ${INSTALLER_MODULE_DIR}
    )
endif()
