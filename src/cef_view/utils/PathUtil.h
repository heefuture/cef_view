/**
 * @file        PathUtil.h
 * @brief       Utility class for managing application resources and paths
 * @version     1.0
 * @date        2025.11.27
 */
#pragma once

#include <string>

namespace cefview {

/**
 * Utility class for managing application resources and paths
 */
class PathUtil {
public:
    /**
     * Platform-specific path separator
     */
    static const std::string sPathSep;

    /**
     * Get the directory where the application executable is located
     * @return Full path to the application directory
     */
    static std::string GetAppDirectory();

    /**
     * Get the directory where resources are stored
     * @return Full path to the resources directory
     */
    static std::string GetAppResourcePath();

    /**
     * Get the full path to a specific resource file
     * @param resourceName Name of the resource file (e.g., "StatusIcon.png")
     * @return Full path to the resource file, or empty string if not found
     */
    static std::string GetResourcePath(const std::string& resourceName);

    /**
     * Get the system temporary directory path
     * @return Full path to the temp directory
     */
    static std::string GetAppTempDirectory();

    /**
     * Get the current working directory of the application
     * @return Full path to the working directory
     */
    static std::string GetAppWorkingDirectory();

    /**
     * Create a directory path (including all parent directories if needed)
     * @param path Directory path to create
     * @return true if directory was created successfully or already exists, false on error
     */
    static bool CreatePath(const std::string& path);

private:
    PathUtil() = delete;
    ~PathUtil() = delete;
    PathUtil(const PathUtil&) = delete;
    PathUtil& operator=(const PathUtil&) = delete;
};

}  // namespace cefview
