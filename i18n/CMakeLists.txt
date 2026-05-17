# 生成翻译
## 设置QT语言cmake模块
find_package(Qt6 COMPONENTS Widgets LinguistTools REQUIRED)

## 通过源代码生成ts文件
set(CREATE_TS_DIR
    ${CMAKE_SOURCE_DIR}/src
)
find_program(LUPDATE_EXECUTABLE lupdate)
execute_process(
    COMMAND ${LUPDATE_EXECUTABLE} -source-language en
    -no-obsolete
    -recursive ${CREATE_TS_DIR}
    -ts ${CMAKE_SOURCE_DIR}/i18n/deepin-installer-en_US.ts)

## 获取ts源文件
file(GLOB INSTALLER_TRANSLATION_TS
    ${CMAKE_SOURCE_DIR}/i18n/deepin-installer*.ts)

## 设置QM文件输出路径
set_source_files_properties(${INSTALLER_TRANSLATION_TS} PROPERTIES OUTPUT_LOCATION ${CMAKE_BINARY_DIR}/i18n)
qt6_add_translation(INSTALLER_TRANSLATION_QM ${INSTALLER_TRANSLATION_TS})
add_custom_target(installer-qm-update DEPENDS ${INSTALLER_TRANSLATION_QM})

## 将ts源文件显示在qtcreate中
add_custom_target(ts SOURCES ${INSTALLER_TRANSLATION_TS})

## 安装qm文件
install(
    DIRECTORY ${CMAKE_BINARY_DIR}/i18n
    DESTINATION ${INSTALL_TARGET}
    FILES_MATCHING PATTERN "*.qm"
)


