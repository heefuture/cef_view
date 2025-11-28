/**
 * @file        PathUtil.h
 * @brief       Utility class for managing application resources and paths
 * @version     1.0
 * @date        2025.11.27
 */
#pragma once

#include <string>

namespace cefview::util {

/**
 * Utility class for managing application resources and paths
 */
class PathUtil {
public:
    /**
     * Get the directory where the application executable is located
     * @return Full path to the application directory
     */
    static std::string getAppDirectory();

    /**
     * Get the directory where resources are stored
     * @return Full path to the resources directory
     */
    static std::string getAppResourcePath();

    /**
     * Get the full path to a specific resource file
     * @param resourceName Name of the resource file (e.g., "StatusIcon.png")
     * @return Full path to the resource file, or empty string if not found
     */
    static std::string getResourcePath(const std::string& resourceName);

    /**
     * Get the system temporary directory path
     * @return Full path to the temp directory
     */
    static std::string getAppTempDirectory();

private:
    PathUtil() = delete;
    ~PathUtil() = delete;
    PathUtil(const PathUtil&) = delete;
    PathUtil& operator=(const PathUtil&) = delete;
};

}  // namespace cefview::util
