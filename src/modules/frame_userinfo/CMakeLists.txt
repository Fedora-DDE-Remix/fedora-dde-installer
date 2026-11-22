set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/modules)
# 增加源文件
file(GLOB SRC_FILES *.h *.cpp)

list(FILTER SRC_FILES EXCLUDE REGEX "ModulesLoader\\.(cpp|h)$")

add_library(DeepinInstallerUserInfoFrame SHARED "${SRC_FILES}"
                                           "${INTERFACE_FILES}"
                                           )
target_link_libraries(DeepinInstallerUserInfoFrame ${LINK_LIBS})


install(TARGETS DeepinInstallerUserInfoFrame
    LIBRARY DESTINATION ${INSTALLER_MODULE_DIR}
)

