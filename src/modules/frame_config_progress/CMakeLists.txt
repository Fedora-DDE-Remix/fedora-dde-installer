# 增加源文件
file(GLOB SRC_FILES *.h *.cpp)

add_library(DeepinConfigProgressFrame SHARED "${SRC_FILES}" "${INTERFACE_FILES}")
target_link_libraries(DeepinConfigProgressFrame ${LINK_LIBS})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/modules)

install(TARGETS DeepinConfigProgressFrame
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/deepin-installer/modules
)
