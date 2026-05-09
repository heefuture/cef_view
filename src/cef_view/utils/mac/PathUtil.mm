/**
 * @file        PathUtil.mm
 * @brief       macOS platform implementation of PathUtil
 * @version     1.0
 * @date        2026.02.10
 * @copyright
 */
#include "utils/PathUtil.h"

#import <Foundation/Foundation.h>

#include <filesystem>
#include <utils/LogUtil.h>

namespace fs = std::filesystem;

namespace cefview {

const std::string PathUtil::sPathSep = "/";

std::string PathUtil::GetAppDirectory() {
    // In the browser process +[NSBundle mainBundle] is the host .app.
    // In a CEF helper sub-process it is the embedded
    // "<Host>.app/Contents/Frameworks/<Helper>.app" — walk back up to
    // the host bundle so every process agrees on a single app root.
    NSBundle* bundle = [NSBundle mainBundle];
    NSString* bundlePath = [bundle bundlePath];
    if (!bundlePath) {
        LOGE << "Failed to get bundle path from NSBundle";
        return std::string();
    }

    NSString* frameworksDir = [bundlePath stringByDeletingLastPathComponent];
    if ([[frameworksDir lastPathComponent] isEqualToString:@"Frameworks"]) {
        NSString* contentsDir = [frameworksDir stringByDeletingLastPathComponent];
        NSString* hostAppPath = [contentsDir stringByDeletingLastPathComponent];
        if ([[hostAppPath pathExtension] isEqualToString:@"app"]) {
            return std::string([hostAppPath UTF8String]);
        }
    }

    return std::string([bundlePath UTF8String]);
}

std::string PathUtil::GetAppResourcePath() {
    @autoreleasepool {
        NSBundle* mainBundle = [NSBundle mainBundle];
        NSString* bundlePath = [mainBundle bundlePath];
        if (!bundlePath) {
            LOGE << "Failed to get bundle path from NSBundle";
            return std::string();
        }

        // In a CEF helper sub-process mainBundle points at the embedded
        // "<Host>.app/Contents/Frameworks/<Helper>.app" — walk back up to
        // the host bundle so every process resolves resources from the
        // same host app.
        if ([bundlePath containsString:@"Helper"]) {
            NSArray* pathComponents = [bundlePath pathComponents];
            NSMutableArray* mainAppComponents = [NSMutableArray array];

            BOOL foundFrameworks = NO;
            for (NSString* component in pathComponents) {
                [mainAppComponents addObject:component];
                if ([component isEqualToString:@"Frameworks"]) {
                    foundFrameworks = YES;
                    break;
                }
            }

            if (foundFrameworks) {
                [mainAppComponents removeLastObject];
                NSString* mainAppPath = [NSString pathWithComponents:mainAppComponents];
                NSBundle* mainAppBundle = [NSBundle bundleWithPath:mainAppPath];
                if (mainAppBundle) {
                    NSString* resourcePath = [mainAppBundle resourcePath];
                    if (resourcePath) {
                        return std::string([resourcePath UTF8String]);
                    }
                }
            }
        }

        NSString* resourcePath = [mainBundle resourcePath];
        if (resourcePath) {
            return std::string([resourcePath UTF8String]);
        }
    }
    return std::string();
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
    NSString* tempDir = NSTemporaryDirectory();
    if (!tempDir) {
        LOGE << "Failed to get temporary directory";
        return std::string();
    }
    return std::string([tempDir UTF8String]);
}

std::string PathUtil::GetAppWorkingDirectory() {
    std::error_code errorCode;
    fs::path currentPath = fs::current_path(errorCode);
    if (errorCode) {
        LOGE << "Failed to get working directory: " << errorCode.message();
        return std::string();
    }
    return currentPath.string();
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
    // Resolve the directory name: use appName if provided, otherwise use the bundle identifier.
    std::string dirName = appName;
    if (dirName.empty()) {
        NSString* bundleId = [[NSBundle mainBundle] bundleIdentifier];
        if (bundleId) {
            dirName = std::string([bundleId UTF8String]);
        }
    }
    if (dirName.empty()) {
        LOGE << "Failed to resolve cache directory name";
        return std::string();
    }

    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    if ([paths count] == 0) {
        LOGE << "Failed to get caches directory";
        return std::string();
    }

    std::string basePath = std::string([[paths firstObject] UTF8String]);
    std::string path = basePath + sPathSep + dirName;

    if (!CreatePath(path)) {
        LOGE << "Failed to create cache directory: " << path;
        return std::string();
    }

    return path;
}

}  // namespace cefview
