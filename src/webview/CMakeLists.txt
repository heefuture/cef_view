cmake_minimum_required(VERSION 3.14)

set(WEBVIEW_TARGET "webview")

message(STATUS "Output dir : ${TARGET_OUT_DIR}")

file(GLOB_RECURSE UTIL_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/util/*.*)
list(APPEND SRC_FILES ${UTIL_SRCS})
source_group(utils FILES ${UTIL_SRCS})

file(GLOB_RECURSE CLIENT_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/client/*.*)
list(APPEND SRC_FILES ${CLIENT_SRCS})
source_group(client FILES ${CLIENT_SRCS})

file(GLOB_RECURSE HANDLER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/handler/*.*)
list(APPEND SRC_FILES ${HANDLER_SRCS})
source_group(handler FILES ${HANDLER_SRCS})

file(GLOB_RECURSE VIEW_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/view/*.*)
list(APPEND SRC_FILES ${VIEW_SRCS})
source_group(handler FILES ${VIEW_SRCS})

option(BUILD_STATIC "option for static lib" OFF)
if(BUILD_STATIC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCEF_STATIC")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
else()
    add_definitions(-DWEBVIEW_EXPORTS)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if (BUILD_STATIC)
add_library (${WEBVIEW_TARGET} STATIC ${SRC_FILES})
else ()
add_library (${WEBVIEW_TARGET} SHARED ${SRC_FILES})
endif ()

# set_target_properties(${WEBVIEW_TARGET} PROPERTIES
#     ARCHIVE_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
#     LIBRARY_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
# )

target_link_libraries(${WEBVIEW_TARGET} libcef_dll_wrapper)

