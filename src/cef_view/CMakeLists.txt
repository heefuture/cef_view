cmake_minimum_required(VERSION 3.14)

set(CEFVIEW_TARGET "cefview")

message(STATUS "Output dir : ${TARGET_OUT_DIR}")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND CEFVIEW_COMPILE_OPTIONS -Werror -Wall -Wextra -Weffc++ -Wconversion -pedantic -Werror=return-type -Wno-unused-command-line-argument -Wno-unused)
endif ()

option(WEBVIEW_BUILD_STATIC "option for static lib" ON)
if(WEBVIEW_BUILD_STATIC)
    add_definitions(-DWEBVIEW_BUILD_STATIC)
endif()

if (MSVC)
    list(APPEND CEFVIEW_COMPILE_OPTIONS "/utf-8")
    list(APPEND CEFVIEW_COMPILE_OPTIONS /we4251 /we4275 /we4715 /we4700)
    list(APPEND CEFVIEW_COMPILE_OPTIONS /W4)

    list(APPEND CEFVIEW_DEFINES NOMINMAX _USE_MATH_DEFINES)
    list(APPEND CEFVIEW_DEFINES UNICODE _UNICODE)
endif (MSVC)

# Logical target used to link the libcef library.
# macOS loads CEF framework at runtime via cef_library_loader, no libcef_lib needed.
if(NOT APPLE)
    # Only Release CEF binaries are available, use Release lib for all configurations.
    ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_RELEASE}" "${CEF_LIB_RELEASE}")
endif()

if(WEBVIEW_BUILD_STATIC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCEF_STATIC")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
else()
    add_definitions(-DWEBVIEW_EXPORTS)
endif()

# ============================================
# Define library target
# ============================================
if (WEBVIEW_BUILD_STATIC)
    add_library(${CEFVIEW_TARGET} STATIC)
else ()
    add_library(${CEFVIEW_TARGET} SHARED)
endif ()

# ============================================
# Include directories
# ============================================
target_include_directories(${CEFVIEW_TARGET}
    PUBLIC
        # Consumers and internal: #include "client/xxx.h", #include "utils/xxx.h"
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Platform-specific include directories
if(WIN32)
    target_include_directories(${CEFVIEW_TARGET}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/view/win
            ${CMAKE_CURRENT_SOURCE_DIR}/osr/win
            ${CMAKE_CURRENT_SOURCE_DIR}/utils/win
    )
elseif(APPLE)
    target_include_directories(${CEFVIEW_TARGET}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/view/mac
            ${CMAKE_CURRENT_SOURCE_DIR}/osr/mac
            ${CMAKE_CURRENT_SOURCE_DIR}/utils/mac
    )
endif()

# ============================================
# Source files and IDE source groups
# ============================================
file(GLOB BRIDGE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/bridge/*.*)
source_group(bridge FILES ${BRIDGE_SRCS})

file(GLOB CLIENT_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/client/*.*)
source_group(client FILES ${CLIENT_SRCS})

file(GLOB GLOBAL_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/global/*.*)
source_group(global FILES ${GLOBAL_SRCS})

file(GLOB OSR_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/osr/*.*)
source_group(osr FILES ${OSR_SRCS})

file(GLOB SCHEME_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/scheme/*.*)
source_group(scheme FILES ${SCHEME_SRCS})

file(GLOB UTILS_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/*.*)
source_group(utils FILES ${UTILS_SRCS})

file(GLOB VIEW_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/view/*.*)
source_group(view FILES ${VIEW_SRCS})

target_sources(${CEFVIEW_TARGET} PRIVATE
    ${BRIDGE_SRCS}
    ${CLIENT_SRCS}
    ${GLOBAL_SRCS}
    ${OSR_SRCS}
    ${SCHEME_SRCS}
    ${UTILS_SRCS}
    ${VIEW_SRCS}
)

if(WIN32)
    file(GLOB VIEW_WIN_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/view/win/*.*)
    source_group(view\\win FILES ${VIEW_WIN_SRCS})

    file(GLOB OSR_WIN_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/osr/win/*.*)
    source_group(osr\\win FILES ${OSR_WIN_SRCS})

    file(GLOB UTILS_WIN_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/win/*.*)
    source_group(utils\\win FILES ${UTILS_WIN_SRCS})

    target_sources(${CEFVIEW_TARGET} PRIVATE
        ${VIEW_WIN_SRCS}
        ${OSR_WIN_SRCS}
        ${UTILS_WIN_SRCS}
    )
elseif(APPLE)
    file(GLOB VIEW_MAC_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/view/mac/*.*)
    source_group(view/mac FILES ${VIEW_MAC_SRCS})

    file(GLOB OSR_MAC_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/osr/mac/*.*)
    source_group(osr/mac FILES ${OSR_MAC_SRCS})

    file(GLOB UTILS_MAC_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/mac/*.*)
    source_group(utils/mac FILES ${UTILS_MAC_SRCS})

    target_sources(${CEFVIEW_TARGET} PRIVATE
        ${VIEW_MAC_SRCS}
        ${OSR_MAC_SRCS}
        ${UTILS_MAC_SRCS}
    )
endif()

# ============================================
# Windows platform link libraries
# ============================================
if(WIN32)
    target_link_libraries(${CEFVIEW_TARGET}
        PRIVATE
            d3d11.lib
            dxgi.lib
            d3dcompiler.lib
            dcomp.lib
            imm32.lib
            d2d1.lib
            windowsapp.lib
    )

    if(USE_ATL)
        target_link_libraries(${CEFVIEW_TARGET} PRIVATE oleacc.lib)
    endif()
endif()

# ============================================
# macOS platform link libraries
# ============================================
if(APPLE)
    target_link_libraries(${CEFVIEW_TARGET}
        PUBLIC
            "-framework Cocoa"
            "-framework OpenGL"
            "-framework IOSurface"
            "-framework Metal"
            "-framework QuartzCore"
    )

    # Set Objective-C++ compile flags for all .mm files
    file(GLOB_RECURSE ALL_MAC_MM_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/view/mac/*.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/osr/mac/*.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/mac/*.mm
    )
    set_source_files_properties(${ALL_MAC_MM_FILES}
        PROPERTIES
            COMPILE_FLAGS "-x objective-c++ -fobjc-arc"
    )
endif()

# ============================================
# CEF common configuration
# ============================================
target_link_libraries(${CEFVIEW_TARGET}
    PUBLIC
        libcef_dll_wrapper
)

# macOS uses framework loaded at runtime; Linux/Windows link libcef statically.
if(NOT APPLE)
    target_link_libraries(${CEFVIEW_TARGET}
        PUBLIC
            libcef_lib
    )
endif()

target_compile_features(${CEFVIEW_TARGET} PUBLIC cxx_std_17)

message(STATUS "CefView C++ compile flags: ${CEFVIEW_COMPILE_OPTIONS}")

if(WIN32)
    if (MSVC)
        target_compile_definitions(${CEFVIEW_TARGET} PRIVATE ${CEFVIEW_DEFINES})
        target_compile_options(${CEFVIEW_TARGET} PRIVATE ${CEFVIEW_COMPILE_OPTIONS})
        target_compile_options(${CEFVIEW_TARGET} PRIVATE $<$<CONFIG:Debug>:${CEFVIEW_COMPILER_FLAGS_DEBUG}>)
        target_compile_options(${CEFVIEW_TARGET} PRIVATE $<$<CONFIG:Release>:${CEFVIEW_COMPILER_FLAGS_RELEASE}>)
    endif ()
endif()
