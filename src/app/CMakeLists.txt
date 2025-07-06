
cmake_minimum_required(VERSION 3.14)

set(APP_TARGET "cefapp")

#message(STATUS "Output dir : ${TARGET_OUT_DIR}")

# 主应用程序源文件
file(GLOB APP_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.*)

source_group(app FILES ${CEF_APP_SRCS})

set(CEFVIEWDIR ${CMAKE_CURRENT_SOURCE_DIR}/../cefview)

# 创建主应用程序包
add_executable(${APP_TARGET} ${APP_SRCS})

add_dependencies(${APP_TARGET} libcef_dll_wrapper cefview)

include_directories(${CEFVIEWDIR})

# 链接库
target_link_libraries(${APP_TARGET} libcef_dll_wrapper cefview)

# # 设置输出目录
# set_target_properties(${APP_TARGET} PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
# )

# 复制 CEF 库和资源
add_custom_command(
    TARGET ${APP_TARGET}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying CEF lib..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CEF_ROOT}/${CMAKE_BUILD_TYPE}"
        "$<TARGET_FILE_DIR:${APP_TARGET}>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CEF_ROOT}/Resources"
        "$<TARGET_FILE_DIR:${APP_TARGET}>"
)
