cmake_minimum_required(VERSION 3.14)

#project(cefApp VERSION 0.0.0.0)

project(cefApp)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# 构建类型设置
set(CMAKE_CONFIGURATION_TYPES Debug Release)

message(STATUS "Current build type: ${CMAKE_BUILD_TYPE}")

set(CEF_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/libcef")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")

# 查找 CEF 包
find_package(CEF REQUIRED)
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)
PRINT_CEF_CONFIG()

option(USE_SANDBOX "option for cef use sandbox" OFF)
# 沙箱配置
if (USE_SANDBOX)
    ADD_LOGICAL_TARGET("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
    add_definitions(-DCEF_USE_SANDBOX)
endif()

set(TARGET_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}")

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TARGET_OUT_DIR})
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${TARGET_OUT_DIR})
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${TARGET_OUT_DIR})

add_subdirectory(src/app)
add_subdirectory(src/sub_process)
add_subdirectory(src/webview)