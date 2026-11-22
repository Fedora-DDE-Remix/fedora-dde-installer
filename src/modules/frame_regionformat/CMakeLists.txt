set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/modules)
# 增加源文件
file(GLOB SRC_FILES *.h *.cpp)

add_library(DeepinInstallerRegionFormatFrame SHARED "${SRC_FILES}" "${INTERFACE_FILES}")
target_link_libraries(DeepinInstallerRegionFormatFrame ${LINK_LIBS})

install(TARGETS DeepinInstallerRegionFormatFrame
    LIBRARY DESTINATION ${INSTALLER_MODULE_DIR}
)
