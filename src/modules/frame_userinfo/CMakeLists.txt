# 增加源文件
file(GLOB SRC_FILES *.h *.cpp)

find_package(DdeSessionShell REQUIRED)

add_library(DeepinInstallerUserInfoFrame SHARED "${SRC_FILES}"
                                           "${INTERFACE_FILES}"
                                           )
target_link_libraries(DeepinInstallerUserInfoFrame ${LINK_LIBS})

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/modules)

install(TARGETS DeepinInstallerUserInfoFrame
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/deepin-installer/modules
)

