# 增加源文件
file(GLOB SRC_FILES *.h *.cpp)

add_library(DeepinInstallerProgressFrame SHARED "${SRC_FILES}" "${INTERFACE_FILES}")
target_link_libraries(DeepinInstallerProgressFrame ${LINK_LIBS})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/modules)

install(TARGETS DeepinInstallerProgressFrame
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/deepin-installer/modules
)
