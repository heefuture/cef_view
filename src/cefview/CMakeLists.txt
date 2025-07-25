cmake_minimum_required(VERSION 3.14)

set(CEFVIEW_TARGET "cefview")

message(STATUS "Output dir : ${TARGET_OUT_DIR}")

# Logical target used to link the libcef library.
ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")

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
source_group(view FILES ${VIEW_SRCS})

file(GLOB_RECURSE MGR_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/manager/*.*)
list(APPEND SRC_FILES ${MGR_SRCS})
source_group(manager FILES ${MGR_SRCS})


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

