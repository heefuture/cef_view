
cmake_minimum_required(VERSION 3.14)

set(APP_TARGET "cefapp")

if(GEN_NINJA OR GEN_MAKEFILES)
    # By default Ninja and Make builds don't create a subdirectory named after
    # the configuration.
    set(APP_TARGET_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}")

    # Output binaries (executables, libraries) to the correct directory.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${APP_TARGET_OUT_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${APP_TARGET_OUT_DIR})
else()
    set(APP_TARGET_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>")
endif()

#message(STATUS "Output dir : ${TARGET_OUT_DIR}")
# Logical target used to link the libcef library.
ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")

# 主应用程序源文件
file(GLOB APP_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.*)


set(CEFVIEWDIR ${CMAKE_CURRENT_SOURCE_DIR}/../cefview)
include_directories(${CEFVIEWDIR})

# 创建主应用程序包
add_executable(${APP_TARGET} ${APP_SRCS})

add_dependencies(${APP_TARGET} libcef_dll_wrapper cefview)

# 链接库
target_link_libraries(${APP_TARGET} libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS} cefview)

if (WIN32)
    if(USE_ATL)
        # add_definitions(-D_CEF_USE_ATL)
        # Required by VS2013 to link accessibility API functions.
        target_link_libraries(${APP_TARGET} oleacc.lib)
    endif()
    target_link_libraries(${APP_TARGET} d3d11.lib dxgi.lib d3dcompiler.lib)
endif()

if(USE_SANDBOX)
  # Logical target used to link the cef_sandbox library.
  ADD_LOGICAL_TARGET("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
  target_link_libraries(${APP_TARGET} cef_sandbox_lib ${CEF_SANDBOX_STANDARD_LIBS})
endif()

# # Add the custom manifest files to the executable.
# ADD_WINDOWS_MANIFEST("${CMAKE_CURRENT_SOURCE_DIR}/win" "${APP_TARGET}" "exe")

set_target_properties(${APP_TARGET} PROPERTIES
    WIN32_EXECUTABLE ON
)

# # 设置输出目录
# set_target_properties(${APP_TARGET} PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY "${TARGET_OUT_DIR}"
# )

# # Copy CEF binary and resource files to the target output directory.
# COPY_FILES("${CEF_TARGET}" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
# COPY_FILES("${CEF_TARGET}" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

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
