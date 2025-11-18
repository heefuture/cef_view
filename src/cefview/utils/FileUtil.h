/**
* @file        FileUtil.h
* @brief
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

namespace cefview::util {

// Platform-specific path separator.
extern const char kPathSep;

// Reads the file at |path| and return the contents as a string. If the file
// size exceeds |max_size|, the returned string is truncated to |max_size|.
std::string readFileToString(const std::string& path, size_t max_size = std::numeric_limits<size_t>::max());

// Writes the given buffer into the file, overwriting any data that was
// previously there. Returns the number of bytes written, or -1 on error.
int writeFile(const std::string& path, const char* data, int size);

// Combines |path1| and |path2| with the correct platform-specific path
// separator.
std::string joinPath(const std::string& path1, const std::string& path2);

// Extracts the file extension from |path|.
std::string getFileExtension(const std::string& path);

}  // namespace cefview::util

#endif //!FILEUTIL_H
