
cmake_minimum_required(VERSION 3.14)

set(CEFVIEWDIR ${CMAKE_CURRENT_SOURCE_DIR}/../cef_view)

# Logical target used to link the libcef library.
if(CEF_USE_DEBUG)
    # Standard distribution: use matching Debug/Release CEF libs.
    ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
else()
    # Minimal distribution: only Release CEF lib available, use for all configurations.
    ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_RELEASE}" "${CEF_LIB_RELEASE}")
endif()

if(WIN32)
    # Windows: Build separate browser.exe subprocess
    set(BROWSER_TARGET "browser")
    message(STATUS "Building Windows browser subprocess")

    set(APP_SRCS SubProcessMain.cpp)

    add_executable(${BROWSER_TARGET} WIN32 ${APP_SRCS})
    add_dependencies(${BROWSER_TARGET} libcef_dll_wrapper cefview)
    target_include_directories(${BROWSER_TARGET} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CEFVIEWDIR}/
    )
    target_link_libraries(${BROWSER_TARGET}
        libcef_lib
        libcef_dll_wrapper
        cefview
        ${CEF_STANDARD_LIBS}
    )

    if(USE_ATL)
        target_link_libraries(${BROWSER_TARGET} oleacc.lib)
    endif()

    if(USE_SANDBOX)
        ADD_LOGICAL_TARGET("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
        target_link_libraries(${BROWSER_TARGET} cef_sandbox_lib ${CEF_SANDBOX_STANDARD_LIBS})
    endif()

elseif(APPLE)
    # macOS: Build helper app bundles for sub-processes.
    # CEF requires separate Helper bundles for renderer, GPU, plugin, and alerts processes.
    message(STATUS "Building macOS helper app bundles")

    set(CEF_HELPER_TARGET "cefapp_Helper")
    set(CEF_HELPER_OUTPUT_NAME "cefapp Helper")

    set(HELPER_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/HelperProcess.mm
        ${CEFVIEWDIR}/client/CefViewApp.cpp
        ${CEFVIEWDIR}/client/CefViewAppDelegateRenderer.cpp
        ${CEFVIEWDIR}/bridge/CefJsBridgeRender.cpp
        ${CEFVIEWDIR}/bridge/CefJsHandler.cpp
        ${CEFVIEWDIR}/utils/CefSwitches.cpp
    )

    # Determine the target output directory.
    SET_CEF_TARGET_OUT_DIR()

    # Create the multiple Helper app bundle targets.
    foreach(_suffix_list ${CEF_HELPER_APP_SUFFIXES})
        # Convert to a list and extract the suffix values.
        string(REPLACE ":" ";" _suffix_list ${_suffix_list})
        list(GET _suffix_list 0 _name_suffix)
        list(GET _suffix_list 1 _target_suffix)
        list(GET _suffix_list 2 _plist_suffix)

        # Define Helper target and output names.
        set(_helper_target "${CEF_HELPER_TARGET}${_target_suffix}")
        set(_helper_output_name "${CEF_HELPER_OUTPUT_NAME}${_name_suffix}")

        # Create Helper-specific variants of the helper-Info.plist file.
        # Done manually because configure_file uses global env variables and would
        # insert the wrong values with multiple targets.
        set(_helper_info_plist "${CMAKE_CURRENT_BINARY_DIR}/helper-Info${_target_suffix}.plist")
        file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../app/mac/helper-Info.plist.in" _plist_contents)
        string(REPLACE "\${EXECUTABLE_NAME}" "${_helper_output_name}" _plist_contents ${_plist_contents})
        string(REPLACE "\${PRODUCT_NAME}" "${_helper_output_name}" _plist_contents ${_plist_contents})
        string(REPLACE "\${BUNDLE_ID_SUFFIX}" "${_plist_suffix}" _plist_contents ${_plist_contents})
        file(WRITE ${_helper_info_plist} ${_plist_contents})

        # Create Helper executable target.
        add_executable(${_helper_target} MACOSX_BUNDLE ${HELPER_SRCS})
        SET_EXECUTABLE_TARGET_PROPERTIES(${_helper_target})
        add_dependencies(${_helper_target} libcef_dll_wrapper)
        target_compile_options(${_helper_target} PRIVATE -Wno-unused)
        target_include_directories(${_helper_target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/..
            ${CEFVIEWDIR}/
        )
        target_link_libraries(${_helper_target}
            libcef_dll_wrapper
            ${CEF_STANDARD_LIBS}
        )
        set_target_properties(${_helper_target} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${_helper_info_plist}
            OUTPUT_NAME ${_helper_output_name}
        )
    endforeach()
endif()
