
cmake_minimum_required(VERSION 3.14)

set(BROWSER_TARGET "browser")

message("this is browser_subprocess project")

set(CEFVIEWDIR ${CMAKE_CURRENT_SOURCE_DIR}/../cefview)

# Logical target used to link the libcef library.
ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")

# 主应用程序源文件
set(APP_SRCS
    # ${CEFVIEWDIR}/client/CefViewApp.cpp
    # ${CEFVIEWDIR}/client/CefSwitches.cpp
    main_process.cpp
)

# 创建主应用程序包
add_executable(${BROWSER_TARGET} ${APP_SRCS})

add_dependencies(${BROWSER_TARGET} libcef_dll_wrapper)

target_include_directories(
    ${BROWSER_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CEFVIEWDIR}/
    ${CEFVIEWDIR}/include
)

# 链接库
target_link_libraries(
    ${BROWSER_TARGET}
    libcef_lib
    libcef_dll_wrapper
    ${CEF_STANDARD_LIBS}
    #glu32.lib
    #imm32.lib
    #opengl32.lib
)

if(USE_ATL)
    # Required by VS2013 to link accessibility API functions.
    target_link_libraries(${BROWSER_TARGET} oleacc.lib)
endif()

if(USE_SANDBOX)
  # Logical target used to link the cef_sandbox library.
  ADD_LOGICAL_TARGET("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
  target_link_libraries(${BROWSER_TARGET} cef_sandbox_lib ${CEF_SANDBOX_STANDARD_LIBS})
endif()

# # Add the custom manifest files to the executable.
# ADD_WINDOWS_MANIFEST("${CMAKE_CURRENT_SOURCE_DIR}/win" "${APP_TARGET}" "exe")










