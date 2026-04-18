#!/usr/bin/env bash
# ============================================================================
# Generate Xcode project on macOS.
# Automatically downloads the CEF distribution matching the host architecture
# (or the one specified via --arch), extracts it to ./libcef and runs CMake
# with the Xcode generator, producing the project at ./build_xcode.
# ============================================================================

set -e

# ============================================================================
# CEF download configuration - modify these variables to upgrade CEF version
# ============================================================================
CEF_VERSION="142.0.17+g60aac24+chromium-142.0.7444.176"
CEF_URL_BASE="https://cef-builds.spotifycdn.com/cef_binary_142.0.17%2Bg60aac24%2Bchromium-142.0.7444.176"

# ============================================================================
# Helpers
# ============================================================================
log_info() {
    printf '\033[0;36m[INFO]\033[0m %s\n' "$1"
}

log_warn() {
    printf '\033[0;33m[WARN]\033[0m %s\n' "$1"
}

log_error() {
    printf '\033[0;31m[ERROR]\033[0m %s\n' "$1" 1>&2
}

usage() {
    cat <<'EOF'
Usage: ./GenerateProject.sh [options]

Options:
  --debug-cef       Download standard (full) CEF distribution with Debug binaries.
                    Without this flag, downloads the minimal distribution (Release only).
  --arch <arch>     Force CEF architecture: arm64 or x64.
                    Defaults to the host machine architecture.
  --help, -h        Show this help message.

Examples:
  ./GenerateProject.sh                      Release mode (default, minimal package)
  ./GenerateProject.sh --debug-cef          Debug mode (standard package with Debug dylibs)
  ./GenerateProject.sh --arch x64           Force Intel CEF package on Apple Silicon host
EOF
}

# ============================================================================
# Parse command line arguments
# ============================================================================
USE_DEBUG_CEF=0
ARCH_OVERRIDE=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug-cef)
            USE_DEBUG_CEF=1
            shift
            ;;
        --arch)
            if [[ $# -lt 2 ]]; then
                log_error "--arch requires a value (arm64 or x64)."
                exit 1
            fi
            ARCH_OVERRIDE="$2"
            shift 2
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            log_error "Unknown argument: $1"
            usage
            exit 1
            ;;
    esac
done

# ============================================================================
# Resolve CEF platform suffix from architecture
# ============================================================================
if [[ -n "${ARCH_OVERRIDE}" ]]; then
    HOST_ARCH="${ARCH_OVERRIDE}"
else
    HOST_ARCH="$(uname -m)"
fi

case "${HOST_ARCH}" in
    arm64|aarch64)
        CEF_PLATFORM="macosarm64"
        ;;
    x86_64|x64|amd64)
        CEF_PLATFORM="macosx64"
        ;;
    *)
        log_error "Unsupported architecture: ${HOST_ARCH}"
        exit 1
        ;;
esac

log_info "Host architecture resolved to: ${CEF_PLATFORM}"

if [[ ${USE_DEBUG_CEF} -eq 1 ]]; then
    log_warn "Debug CEF mode enabled - will download standard distribution with Debug binaries."
    CEF_ARCHIVE_NAME="cef_binary_${CEF_VERSION}_${CEF_PLATFORM}"
    CEF_URL="${CEF_URL_BASE}_${CEF_PLATFORM}.tar.bz2"
else
    log_info "Release CEF mode (default) - will download minimal distribution."
    CEF_ARCHIVE_NAME="cef_binary_${CEF_VERSION}_${CEF_PLATFORM}_minimal"
    CEF_URL="${CEF_URL_BASE}_${CEF_PLATFORM}_minimal.tar.bz2"
fi

# ============================================================================
# Switch to script directory
# ============================================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

# ============================================================================
# Step 1: Check if CEF is already downloaded
# ============================================================================
if [[ -d "libcef/include" ]]; then
    log_info "CEF library already exists, skipping download."
    log_info "To re-download, delete the libcef directory first."
else
    # ========================================================================
    # Step 2: Download CEF distribution
    # ========================================================================
    log_info "Downloading CEF distribution..."
    log_info "URL: ${CEF_URL}"

    if ! curl -fL --retry 3 --retry-delay 3 --retry-all-errors -C - -o "cef_download.tar.bz2" "${CEF_URL}"; then
        log_error "Failed to download CEF. Please check your network connection."
        rm -f "cef_download.tar.bz2"
        exit 1
    fi

    # ========================================================================
    # Step 3: Extract and rename
    # ========================================================================
    log_info "Extracting CEF archive..."

    if ! tar -xjf "cef_download.tar.bz2"; then
        log_error "Failed to extract CEF archive."
        rm -f "cef_download.tar.bz2"
        exit 1
    fi

    if [[ -d "libcef" ]]; then
        log_warn "Removing old libcef directory..."
        rm -rf "libcef"
    fi

    if ! mv "${CEF_ARCHIVE_NAME}" "libcef"; then
        log_error "Failed to rename extracted directory '${CEF_ARCHIVE_NAME}' to 'libcef'."
        rm -f "cef_download.tar.bz2"
        exit 1
    fi

    rm -f "cef_download.tar.bz2"
    log_info "CEF extracted to libcef"
fi

# ============================================================================
# Step 4: Generate Xcode project
# ============================================================================
log_info "Generating Xcode project..."

CMAKE_ARGS=(-G "Xcode" -S . -B build_xcode)
if [[ ${USE_DEBUG_CEF} -eq 1 ]]; then
    CMAKE_ARGS+=(-DCEF_USE_DEBUG=ON)
fi

# Match CEF binary architecture so Xcode builds the same slice.
case "${CEF_PLATFORM}" in
    macosarm64)
        CMAKE_ARGS+=(-DCMAKE_OSX_ARCHITECTURES=arm64)
        ;;
    macosx64)
        CMAKE_ARGS+=(-DCMAKE_OSX_ARCHITECTURES=x86_64)
        ;;
esac

if ! cmake "${CMAKE_ARGS[@]}"; then
    log_error "CMake generation failed."
    exit 1
fi

log_info "Project generated successfully at build_xcode/"
log_info "Open build_xcode/CefView.xcodeproj in Xcode to start developing."
