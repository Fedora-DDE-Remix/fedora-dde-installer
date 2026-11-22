set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/modules)
# 增加源文件
file(GLOB SRC_FILES *.h *.cpp)

add_library(DeepinInstallerComponentFrame SHARED "${SRC_FILES}" "${INTERFACE_FILES}")
target_link_libraries(DeepinInstallerComponentFrame ${LINK_LIBS})

install(TARGETS DeepinInstallerComponentFrame
    LIBRARY DESTINATION ${INSTALLER_MODULE_DIR}
)
