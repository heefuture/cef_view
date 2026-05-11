/**
 * @file        PathUtil.cpp
 * @brief       Windows platform implementation of PathUtil
 * @version     1.0
 * @date        2025.11.27
 */
#include "utils/PathUtil.h"

#include <windows.h>
#include <ShlObj.h>
#include <memory>

#include <filesystem>
#include <utils/LogUtil.h>

namespace fs = std::filesystem;

namespace cefview {

const std::string PathUtil::sPathSep = "\\";

std::string PathUtil::GetAppDirectory() {
    char buffer[MAX_PATH] = {};
    DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length == 0) {
        LOGE << "Failed to get module file name: " << GetLastError();
        return std::string();
    }
    fs::path exePath(buffer);
    return exePath.parent_path().string();
}

std::string PathUtil::GetAppResourcePath() {
    std::string appDir = GetAppDirectory();
    if (appDir.empty()) {
        return std::string();
    }

    fs::path resourcePath = fs::path(appDir) / "resources";
    if (fs::exists(resourcePath) && fs::is_directory(resourcePath)) {
        return resourcePath.string();
    }

    return appDir;
}

std::string PathUtil::GetResourcePath(const std::string& resourceName) {
    std::string resourceDir = GetAppResourcePath();
    if (resourceDir.empty()) {
        return std::string();
    }

    fs::path fullPath = fs::path(resourceDir) / resourceName;
    if (fs::exists(fullPath) && fs::is_regular_file(fullPath)) {
        return fullPath.string();
    }

    std::string appDir = GetAppDirectory();
    fullPath = fs::path(appDir) / resourceName;
    if (fs::exists(fullPath) && fs::is_regular_file(fullPath)) {
        return fullPath.string();
    }

    LOGW << "Resource not found: " << resourceName;
    return std::string();
}

std::string PathUtil::GetSysTempDirectory() {
    std::unique_ptr<TCHAR[]> buffer(new TCHAR[MAX_PATH + 1]);
    if (!::GetTempPath(MAX_PATH, buffer.get())) {
        return std::string();
    }

#ifdef UNICODE
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
}

std::string PathUtil::GetAppWorkingDirectory() {
    std::unique_ptr<TCHAR[]> buffer(new TCHAR[MAX_PATH + 1]);
    if (!::GetCurrentDirectory(MAX_PATH, buffer.get())) {
        return std::string();
    }

#ifdef UNICODE
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
}

bool PathUtil::CreatePath(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    std::error_code errorCode;
    fs::path dirPath(path);

    if (fs::exists(dirPath, errorCode)) {
        return fs::is_directory(dirPath, errorCode);
    }

    bool result = fs::create_directories(dirPath, errorCode);
    if (errorCode) {
        LOGE << "Failed to create directory: " << path << " error: " << errorCode.message();
        return false;
    }
    return result;
}

std::string PathUtil::GetAppCacheDirectory(const std::string& appName) {
    // Resolve the directory name: use appName if provided, otherwise derive from the executable name.
    std::string dirName = appName;
    if (dirName.empty()) {
        char buffer[MAX_PATH] = {};
        DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        if (length > 0) {
            dirName = fs::path(buffer).stem().string();
        }
    }
    if (dirName.empty()) {
        LOGE << "Failed to resolve cache directory name";
        return std::string();
    }

    char localAppData[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
        LOGE << "Failed to get LocalAppData path";
        return std::string();
    }

    std::string path = std::string(localAppData) + sPathSep + dirName;
    if (!CreatePath(path)) {
        LOGE << "Failed to create cache directory: " << path;
        return std::string();
    }

    return path;
}

}  // namespace cefview
