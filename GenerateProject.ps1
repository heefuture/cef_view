# ============================================================================
# CEF download configuration - modify these variables to upgrade CEF version
# ============================================================================
$CEF_VERSION = "142.0.17+g60aac24+chromium-142.0.7444.176"
$CEF_ARCHIVE_NAME = "cef_binary_${CEF_VERSION}_windows64_minimal"
$CEF_URL = "https://cef-builds.spotifycdn.com/cef_binary_142.0.17%2Bg60aac24%2Bchromium-142.0.7444.176_windows64_minimal.tar.bz2"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
Push-Location $ScriptDir

# ============================================================================
# Step 1: Check if CEF is already downloaded
# ============================================================================
if (Test-Path "libcef\include") {
    Write-Host "[INFO] CEF library already exists, skipping download." -ForegroundColor Green
} else {
    # ========================================================================
    # Step 2: Download CEF minimal distribution
    # ========================================================================
    Write-Host "[INFO] Downloading CEF minimal distribution..." -ForegroundColor Cyan
    Write-Host "[INFO] URL: $CEF_URL"

    $ProgressPreference = 'SilentlyContinue'
    curl.exe -L -o "cef_minimal.tar.bz2" $CEF_URL
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] Failed to download CEF. Please check your network connection." -ForegroundColor Red
        Pop-Location
        exit 1
    }

    # ========================================================================
    # Step 3: Extract and rename
    # ========================================================================
    Write-Host "[INFO] Extracting CEF archive..." -ForegroundColor Cyan

    tar -xjf "cef_minimal.tar.bz2"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] Failed to extract CEF archive." -ForegroundColor Red
        Remove-Item -Path "cef_minimal.tar.bz2" -Force -ErrorAction SilentlyContinue
        Pop-Location
        exit 1
    }

    if (Test-Path "libcef") {
        Write-Host "[INFO] Removing old libcef directory..." -ForegroundColor Yellow
        Remove-Item -Path "libcef" -Recurse -Force
    }

    Rename-Item -Path $CEF_ARCHIVE_NAME -NewName "libcef"
    if (-not $?) {
        Write-Host "[ERROR] Failed to rename extracted directory." -ForegroundColor Red
        Remove-Item -Path "cef_minimal.tar.bz2" -Force -ErrorAction SilentlyContinue
        Pop-Location
        exit 1
    }

    Remove-Item -Path "cef_minimal.tar.bz2" -Force -ErrorAction SilentlyContinue
    Write-Host "[INFO] CEF extracted to libcef" -ForegroundColor Green
}

# ============================================================================
# Step 4: Generate Visual Studio project
# ============================================================================
Write-Host "[INFO] Generating Visual Studio 2022 project..." -ForegroundColor Cyan

cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] CMake generation failed." -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "[INFO] Project generated successfully at build\" -ForegroundColor Green
Write-Host "[INFO] Open build\CefView.sln in Visual Studio to start developing." -ForegroundColor Green
Pop-Location
