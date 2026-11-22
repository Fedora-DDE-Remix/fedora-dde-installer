set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/modules)
# 增加源文件
file(GLOB SRC_FILES *.h *.cpp)

add_library(DeepinLiveConfigProgressFrame SHARED "${SRC_FILES}" "${INTERFACE_FILES}")
target_link_libraries(DeepinLiveConfigProgressFrame ${LINK_LIBS})

install(TARGETS DeepinLiveConfigProgressFrame
    LIBRARY DESTINATION ${INSTALLER_MODULE_DIR}
)
