#include "utils/PathUtil.h"

#include <windows.h>
#include <iostream>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

namespace cefview {

#ifdef _WIN32
const std::string PathUtil::sPathSep = "\\";
#else
const std::string PathUtil::sPathSep = "/";
#endif

std::string PathUtil::GetAppDirectory() {
    char buffer[MAX_PATH];
    DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);

    if (length == 0) {
        std::cerr << "Failed to get module file name: " << GetLastError() << std::endl;
        return "";
    }

    fs::path exePath(buffer);
    return exePath.parent_path().string();
}

std::string PathUtil::GetAppResourcePath() {
    std::string appDir = GetAppDirectory();
    if (appDir.empty()) {
        return "";
    }

    // Resources are located in the same directory as the executable
    // or in a "resources" subdirectory
    fs::path resourcePath = fs::path(appDir) / "resources";

    // Check if resources subdirectory exists
    if (fs::exists(resourcePath) && fs::is_directory(resourcePath)) {
        return resourcePath.string();
    }

    // Otherwise, return the app directory itself
    return appDir;
}

std::string PathUtil::GetResourcePath(const std::string& resourceName) {
    std::string resourceDir = GetAppResourcePath();
    if (resourceDir.empty()) {
        return "";
    }

    fs::path fullPath = fs::path(resourceDir) / resourceName;

    // Check if file exists
    if (fs::exists(fullPath) && fs::is_regular_file(fullPath)) {
        return fullPath.string();
    }

    // If not found in resources directory, try app directory
    std::string appDir = GetAppDirectory();
    fullPath = fs::path(appDir) / resourceName;

    if (fs::exists(fullPath) && fs::is_regular_file(fullPath)) {
        return fullPath.string();
    }

    std::cerr << "Resource not found: " << resourceName << std::endl;
    return "";
}

std::string PathUtil::GetAppTempDirectory() {
#if defined(OS_WIN)
    std::unique_ptr<TCHAR[]> buffer(new TCHAR[MAX_PATH + 1]);
    if (!::GetTempPath(MAX_PATH, buffer.get())) {
        return std::string();
    }
    
    // Convert TCHAR to std::string
#ifdef UNICODE
    // Wide char to multibyte conversion
    int sizeNeeded = ::WideCharToMultiByte(CP_UTF8, 0, buffer.get(), -1, nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0) {
        return std::string();
    }
    std::string result(sizeNeeded - 1, 0);
    ::WideCharToMultiByte(CP_UTF8, 0, buffer.get(), -1, &result[0], sizeNeeded, nullptr, nullptr);
    return result;
#else
    return std::string(buffer.get());
#endif

#elif defined(OS_MACOSX)
    return std::string();
#else
    return std::string();
#endif
}

}  // namespace cefview
