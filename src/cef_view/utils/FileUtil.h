/**
 * @file        FileUtil.h
 * @brief       Utility class for file operations
 * @version     1.0
 * @author      heefuture
 * @date        2025.11.18
 * @copyright
 */
#ifndef FILEUTIL_H
#define FILEUTIL_H
#pragma once

#include <limits>
#include <string>

namespace cefview {

/**
 * @brief Utility class for file operations
 */
class FileUtil {
public:
    /**
     * @brief Read file contents to string
     * @param path File path to read
     * @param maxSize Maximum size to read, truncates if exceeded
     * @return File contents as string, empty if failed
     */
    static std::string ReadFileToString(const std::string& path, size_t maxSize = std::numeric_limits<size_t>::max());

    /**
     * @brief Write buffer to file
     * @param path File path to write
     * @param data Data buffer to write
     * @param size Size of data buffer
     * @return Number of bytes written, or -1 on error
     */
    static int WriteFile(const std::string& path, const char* data, int size);

    /**
     * @brief Join two paths with platform-specific separator
     * @param path1 First path component
     * @param path2 Second path component
     * @return Combined path with correct separator
     */
    static std::string JoinPath(const std::string& path1, const std::string& path2);

    /**
     * @brief Extract file extension from path
     * @param path File path
     * @return File extension without dot, empty if no extension
     */
    static std::string GetFileExtension(const std::string& path);

private:
    FileUtil() = delete;
    ~FileUtil() = delete;
    FileUtil(const FileUtil&) = delete;
    FileUtil& operator=(const FileUtil&) = delete;
};

}  // namespace cefview

#endif //!FILEUTIL_H
