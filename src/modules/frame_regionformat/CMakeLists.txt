# 增加源文件
file(GLOB SRC_FILES *.h *.cpp)

add_library(DeepinInstallerRegionFormatFrame SHARED "${SRC_FILES}" "${INTERFACE_FILES}")
target_link_libraries(DeepinInstallerRegionFormatFrame ${LINK_LIBS})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/modules)

install(TARGETS DeepinInstallerRegionFormatFrame
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/deepin-installer/modules
)
