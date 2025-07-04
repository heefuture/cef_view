# message("this is browser project")

# #set(QtCefWebViewDir ../qtcefwebview)
# #set(CrashHunterDir ${CMAKE_CURRENT_SOURCE_DIR}/../../CrashHunter)
# #set(CRASHHUNTER_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../CrashHunter/x86/CrashHunter_PC3.lib)
# set(Srcs
# 	${QtCefWebViewDir}/client_app.cpp
# 	${QtCefWebViewDir}/client_app_delegates.cpp
# 	${QtCefWebViewDir}/client_renderer.cpp
# 	${QtCefWebViewDir}/client_switches.cpp

# 	main_process.cpp
# )

# add_executable(browser ${Srcs})


# # Logical target used to link the libcef library.
# #ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
# #ADD_LOGICAL_TARGET("CrashHunter_PC3_lib" "${CRASHHUNTER_LIB}" "${CRASHHUNTER_LIB}")

# target_link_libraries(
#     browser PRIVATE
# 	libcef_lib
# 	libcef_dll_wrapper
# 	${CEF_STANDARD_LIBS}
# 	glu32.lib
# 	imm32.lib
# 	opengl32.lib
# 	CrashHunter_PC3_lib
# )

# target_link_options(browser PRIVATE "/LARGEADDRESSAWARE")
# target_link_options(browser PRIVATE "/STACK:0x8000")


# target_include_directories(
#     browser PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/..
# 	${QtCefWebViewDir}/
# 	${QtCefWebViewDir}/include
# 	${CrashHunterDir}/include/
# )

# set(BuildTarget ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/browser.exe)
# add_custom_command(
# 	TARGET browser POST_BUILD
#     COMMAND mt.exe -nologo -manifest ${CMAKE_CURRENT_SOURCE_DIR}/browser.manifest -outputresource:${BuildTarget}
# )