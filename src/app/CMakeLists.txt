
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

# Logical target used to link the libcef library.
if(CEF_USE_DEBUG)
    # Standard distribution: use matching Debug/Release CEF libs.
    ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
else()
    # Minimal distribution: only Release CEF lib available, use for all configurations.
    ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_RELEASE}" "${CEF_LIB_RELEASE}")
endif()

set(CEFVIEWDIR ${CMAKE_CURRENT_SOURCE_DIR}/../cef_view)
include_directories(${CEFVIEWDIR})

# Platform-specific source files
if(WIN32)
    file(GLOB APP_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/win/*.*)
    source_group(win FILES ${APP_SRCS})
    # Allow includes within win/ directory (e.g. #include "MainWindow.h")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/win)
elseif(APPLE)
    # Exclude plist template files from source list.
    file(GLOB APP_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/mac/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/mac/*.mm
    )
    source_group(mac FILES ${APP_SRCS})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mac)
endif()

add_executable(${APP_TARGET} ${APP_SRCS})

add_dependencies(${APP_TARGET} libcef_dll_wrapper cefview)

target_link_libraries(${APP_TARGET} libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS} cefview)

if(WIN32)
    if(USE_ATL)
        # add_definitions(-D_CEF_USE_ATL)
        # Required by VS2013 to link accessibility API functions.
        target_link_libraries(${APP_TARGET} oleacc.lib)
    endif()
    target_link_libraries(${APP_TARGET} d3d11.lib dxgi.lib d3dcompiler.lib)

    set_target_properties(${APP_TARGET} PROPERTIES
        WIN32_EXECUTABLE ON
    )

    # Add manifest file to executable
    add_custom_command(
        TARGET ${APP_TARGET}
        POST_BUILD
        COMMAND "mt.exe" -nologo
                -manifest "${CMAKE_CURRENT_SOURCE_DIR}/win/app.manifest"
                -outputresource:"$<TARGET_FILE:${APP_TARGET}>"\;\#1
        COMMENT "Adding manifest to executable..."
    )

    # Copy CEF libraries and resources for Windows (flat directory layout).
    if(CEF_USE_DEBUG)
        # Standard distribution: copy Debug or Release CEF binaries based on build configuration.
        add_custom_command(
            TARGET ${APP_TARGET}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Copying CEF binaries ($<CONFIGURATION>)..."
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CEF_ROOT}/$<CONFIGURATION>"
                "$<TARGET_FILE_DIR:${APP_TARGET}>"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CEF_ROOT}/Resources"
                "$<TARGET_FILE_DIR:${APP_TARGET}>"
            COMMAND ${CMAKE_COMMAND} -E echo "Copying app resources..."
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/../resource"
                "$<TARGET_FILE_DIR:${APP_TARGET}>/resources"
        )
    else()
        # Minimal distribution: only Release CEF binaries available.
        add_custom_command(
            TARGET ${APP_TARGET}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Copying CEF binaries (Release only)..."
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CEF_ROOT}/Release"
                "$<TARGET_FILE_DIR:${APP_TARGET}>"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CEF_ROOT}/Resources"
                "$<TARGET_FILE_DIR:${APP_TARGET}>"
            COMMAND ${CMAKE_COMMAND} -E echo "Copying app resources..."
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/../resource"
                "$<TARGET_FILE_DIR:${APP_TARGET}>/resources"
        )
    endif()

elseif(APPLE)
    # Variables referenced from the main Info.plist file.
    set(EXECUTABLE_NAME "${APP_TARGET}")
    set(PRODUCT_NAME "${APP_TARGET}")

    # Output path for the main app bundle.
    set(CEF_APP "${APP_TARGET_OUT_DIR}/${APP_TARGET}.app")

    set_target_properties(${APP_TARGET} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/mac/Info.plist.in
    )
    target_link_libraries(${APP_TARGET} "-framework Cocoa" "-framework OpenGL" "-framework IOSurface")

    # Copy the CEF framework into the Frameworks directory and create symlinks.
    COPY_MAC_FRAMEWORK("${APP_TARGET}" "${CEF_BINARY_DIR}" "${CEF_APP}")

    # Helper target names matching those defined in src/sub_process/CMakeLists.txt.
    set(CEF_HELPER_TARGET "cefapp_Helper")
    set(CEF_HELPER_OUTPUT_NAME "cefapp Helper")

    # Add dependencies on all helper targets and copy them into Frameworks/.
    foreach(_suffix_list ${CEF_HELPER_APP_SUFFIXES})
        string(REPLACE ":" ";" _suffix_list ${_suffix_list})
        list(GET _suffix_list 0 _name_suffix)
        list(GET _suffix_list 1 _target_suffix)

        set(_helper_target "${CEF_HELPER_TARGET}${_target_suffix}")
        set(_helper_output_name "${CEF_HELPER_OUTPUT_NAME}${_name_suffix}")

        # Add the Helper as a dependency of the main executable target.
        add_dependencies(${APP_TARGET} "${_helper_target}")

        # Copy the Helper app bundle into the Frameworks directory.
        add_custom_command(
            TARGET ${APP_TARGET}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "$<TARGET_BUNDLE_DIR:${_helper_target}>"
                    "${CEF_APP}/Contents/Frameworks/${_helper_output_name}.app"
            VERBATIM
        )
    endforeach()

    # Copy app resources into the bundle.
    # CEF resources are already included in the framework copied by COPY_MAC_FRAMEWORK.
    add_custom_command(
        TARGET ${APP_TARGET}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying app resources..."
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/../resource"
            "${CEF_APP}/Contents/Resources/resources"
    )
endif()

if(USE_SANDBOX)
    # Logical target used to link the cef_sandbox library.
    ADD_LOGICAL_TARGET("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
    target_link_libraries(${APP_TARGET} cef_sandbox_lib ${CEF_SANDBOX_STANDARD_LIBS})
endif()
