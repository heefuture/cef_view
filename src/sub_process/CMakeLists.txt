
cmake_minimum_required(VERSION 3.14)

set(BROWSER_TARGET "browser")

message("this is browser project")

set(CEFVIEWDIR ${CMAKE_CURRENT_SOURCE_DIR}/../cefview)

# 主应用程序源文件

set(APP_SRCS
    ${CEFVIEWDIR}/client/CefClient.cpp
    ${CEFVIEWDIR}/client/CefClientRenderer.cpp
    ${CEFVIEWDIR}/client/CefSwitches.cpp

    main_process.cpp
)

# 创建主应用程序包
add_executable(${BROWSER_TARGET} ${APP_SRCS})

add_dependencies(${BROWSER_TARGET} libcef_dll_wrapper)

# Logical target used to link the libcef library.
ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
# #ADD_LOGICAL_TARGET("CrashHunter_PC3_lib" "${CRASHHUNTER_LIB}" "${CRASHHUNTER_LIB}")

# 链接库
target_link_libraries(
    browser PRIVATE
    libcef_lib
    libcef_dll_wrapper
    ${CEF_STANDARD_LIBS}
    glu32.lib
    imm32.lib
    opengl32.lib
)

target_include_directories(
    browser PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CEFVIEWDIR}/
    ${CEFVIEWDIR}/include
)










