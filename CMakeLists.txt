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

include_directories(${CEF_ROOT})

# option(USE_SANDBOX "option for cef use sandbox" OFF)
set(USE_SANDBOX OFF)
# 沙箱配置
if (USE_SANDBOX)
    ADD_LOGICAL_TARGET("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
    add_definitions(-DCEF_USE_SANDBOX)
endif()

set(TARGET_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}")

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TARGET_OUT_DIR})
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${TARGET_OUT_DIR})
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${TARGET_OUT_DIR})

if(WIN32)
    option(BUILD_WITH_MT "option for cef use sandbox" ON)
    add_definitions(-DUNICODE)
    option(WEBVIEW_BUILD_STATIC "option for static lib" ON)
    if(WEBVIEW_BUILD_STATIC)
        add_definitions(-DWEBVIEW_BUILD_STATIC)
    endif()
    if(MSVC)
        # change md to mt;
        set(CefCompilerFlags
            CEF_COMPILER_FLAGS_DEBUG
            CEF_COMPILER_FLAGS_RELEASE
            CEF_COMPILER_FLAGS
            )
        foreach(CompilerFlag ${CefCompilerFlags})
            if(BUILD_WITH_MT)
                string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
            else()
                string(REPLACE "/MT" "/MD" ${CompilerFlag} "${${CompilerFlag}}")
            endif()
            message("main cef ${CompilerFlag}:" ${${CompilerFlag}})
        endforeach()

        set(CompilerFlags
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_RELWITHDEBINFO
            )
        foreach(CompilerFlag ${CompilerFlags})
            if(BUILD_WITH_MT)
                string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
            else()
                string(REPLACE "/MT" "/MD" ${CompilerFlag} "${${CompilerFlag}}")
            endif()
            message("main ${CompilerFlag}:" ${${CompilerFlag}})
        endforeach()

        #generate pdb file for release or debug
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG")
    endif()
endif()

add_subdirectory(src/app)
add_subdirectory(src/sub_process)
add_subdirectory(src/cefview)