cmake_minimum_required(VERSION 3.14)

set(CEFVIEW_TARGET "cefview")

message(STATUS "Output dir : ${TARGET_OUT_DIR}")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND CEFVIEW_COMPILE_OPTIONS -Werror -Wall -Wextra -Weffc++ -Wconversion -pedantic -Werror=return-type -Wno-unused-command-line-argument)
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

    # set(CEFVIEW_RUNTIME_LIBRARY_FLAG "/MT" CACHE STRING "Optional flag specifying which runtime to use")
    # if (CEFVIEW_RUNTIME_LIBRARY_FLAG)
    #     list(APPEND CEFVIEW_COMPILER_FLAGS_DEBUG ${CEFVIEW_RUNTIME_LIBRARY_FLAG}d)
    #     list(APPEND CEFVIEW_COMPILER_FLAGS_RELEASE ${CEFVIEW_RUNTIME_LIBRARY_FLAG})
    # endif()
endif (MSVC)

# Logical target used to link the libcef library.
ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")

file(GLOB_RECURSE HANDLER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/bridge/*.*)
list(APPEND SRC_FILES ${HANDLER_SRCS})
source_group(bridge FILES ${HANDLER_SRCS})

file(GLOB_RECURSE CLIENT_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/client/*.*)
list(APPEND SRC_FILES ${CLIENT_SRCS})
source_group(client FILES ${CLIENT_SRCS})

file(GLOB_RECURSE MGR_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/global/*.*)
list(APPEND SRC_FILES ${MGR_SRCS})
source_group(global FILES ${MGR_SRCS})

file(GLOB_RECURSE OSR_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/osr/*.*)
list(APPEND SRC_FILES ${OSR_SRCS})
source_group(osr FILES ${OSR_SRCS})

file(GLOB_RECURSE UTIL_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/utils/*.*)
list(APPEND SRC_FILES ${UTIL_SRCS})
source_group(utils FILES ${UTIL_SRCS})

file(GLOB_RECURSE VIEW_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/view/*.*)
list(APPEND SRC_FILES ${VIEW_SRCS})
source_group(view FILES ${VIEW_SRCS})

if(WEBVIEW_BUILD_STATIC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCEF_STATIC")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
else()
    add_definitions(-DWEBVIEW_EXPORTS)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if (WEBVIEW_BUILD_STATIC)
add_library (${CEFVIEW_TARGET} STATIC ${SRC_FILES})
else ()
add_library (${CEFVIEW_TARGET} SHARED ${SRC_FILES})
endif ()

# set_target_properties(${CEFVIEW_TARGET} PROPERTIES
#     ARCHIVE_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
#     LIBRARY_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
# )

target_link_libraries(${CEFVIEW_TARGET} libcef_lib libcef_dll_wrapper)

message(STATUS "CefView C++ compile flags: ${CEFVIEW_COMPILE_OPTIONS}")

# Link D3D11 libraries for off-screen rendering (OSR) support
if(WIN32)
    if(USE_ATL)
        # Required by VS2013 to link accessibility API functions.
        target_link_libraries(${CEFVIEW_TARGET} oleacc.lib)
    endif()

    #  glu32.lib opengl32.lib
    target_link_libraries(${CEFVIEW_TARGET} d3d11.lib dxgi.lib d3dcompiler.lib imm32.lib)

    if (MSVC)
        target_compile_definitions(${CEFVIEW_TARGET} PRIVATE ${CEFVIEW_DEFINES})
        target_compile_options(${CEFVIEW_TARGET} PRIVATE ${CEFVIEW_COMPILE_OPTIONS})
        target_compile_options(${CEFVIEW_TARGET} PRIVATE $<$<CONFIG:Debug>:${CEFVIEW_COMPILER_FLAGS_DEBUG}>)
        target_compile_options(${CEFVIEW_TARGET} PRIVATE $<$<CONFIG:Release>:${CEFVIEW_COMPILER_FLAGS_RELEASE}>)
    endif ()
endif()

