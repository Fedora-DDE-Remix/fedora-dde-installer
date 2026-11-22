set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/modules)
# 增加源文件
file(GLOB SRC_FILES *.h *.cpp)

add_library(DeepinInstallerSelectOSVersionFrame SHARED "${SRC_FILES}" "${INTERFACE_FILES}" )
target_link_libraries(DeepinInstallerSelectOSVersionFrame ${LINK_LIBS})

install(TARGETS DeepinInstallerSelectOSVersionFrame
    LIBRARY DESTINATION ${INSTALLER_MODULE_DIR}
)
