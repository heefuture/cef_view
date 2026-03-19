@echo off
setlocal enabledelayedexpansion

rem ============================================================================
rem CEF download configuration - modify these variables to upgrade CEF version
rem ============================================================================
set CEF_VERSION=142.0.17+g60aac24+chromium-142.0.7444.176
set CEF_ARCHIVE_NAME=cef_binary_%CEF_VERSION%_windows64_minimal
set CEF_URL=https://cef-builds.spotifycdn.com/cef_binary_142.0.17%%2Bg60aac24%%2Bchromium-142.0.7444.176_windows64_minimal.tar.bz2

pushd "%~dp0"

rem ============================================================================
rem Step 1: Check if CEF is already downloaded
rem ============================================================================
if exist "libcef\include" (
    echo [INFO] CEF library already exists, skipping download.
    goto :generate
)

rem ============================================================================
rem Step 2: Download CEF minimal distribution
rem ============================================================================
echo [INFO] Downloading CEF minimal distribution...
echo [INFO] URL: %CEF_URL%

curl -L -o "cef_minimal.tar.bz2" "%CEF_URL%"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to download CEF. Please check your network connection.
    goto :error
)

rem ============================================================================
rem Step 3: Extract and rename
rem ============================================================================
echo [INFO] Extracting CEF archive...

tar -xjf "cef_minimal.tar.bz2"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to extract CEF archive.
    del /q "cef_minimal.tar.bz2" 2>nul
    goto :error
)

if exist "libcef" (
    echo [INFO] Removing old libcef directory...
    rmdir /s /q "libcef"
)

rename "%CEF_ARCHIVE_NAME%" libcef
if %errorlevel% neq 0 (
    echo [ERROR] Failed to rename extracted directory.
    del /q "cef_minimal.tar.bz2" 2>nul
    goto :error
)

del /q "cef_minimal.tar.bz2" 2>nul
echo [INFO] CEF extracted to libcef

rem ============================================================================
rem Step 4: Generate Visual Studio project
rem ============================================================================
:generate
echo [INFO] Generating Visual Studio 2022 project...

cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
if %errorlevel% neq 0 (
    echo [ERROR] CMake generation failed.
    goto :error
)

echo [INFO] Project generated successfully at build\
echo [INFO] Open build\CefView.sln in Visual Studio to start developing.
goto :done

:error
echo [ERROR] Setup failed.
popd
endlocal
exit /b 1

:done
popd
endlocal
exit /b 0
