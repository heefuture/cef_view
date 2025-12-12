
#include "FileUtil.h"
#include "PathUtil.h"

#include <algorithm>
#include <cstdio>
#include <memory>

namespace cefview {

std::string FileUtil::ReadFileToString(const std::string& path, size_t maxSize) {
    std::string contents;
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) {
        return contents;
    }

    const size_t kBufferSize = 1 << 16;
    std::unique_ptr<char[]> buf(new char[kBufferSize]);
    size_t len;
    size_t size = 0;
    bool readStatus = true;

    // Many files supplied in |path| have incorrect size (proc files etc).
    // Hence, the file is read sequentially as opposed to a one-shot read.
    while ((len = fread(buf.get(), 1, kBufferSize, file)) > 0) {
        contents.append(buf.get(), std::min(len, maxSize - size));
        if ((maxSize - size) < len) {
            readStatus = false;
            break;
        }

        size += len;
    }
    readStatus = readStatus && !ferror(file);
    fclose(file);

    return contents;
}

int FileUtil::WriteFile(const std::string& path, const char* data, int size) {
    FILE* file = fopen(path.c_str(), "wb");
    if (!file) {
        return -1;
    }

    int written = 0;

    do {
        size_t write = fwrite(data + written, 1, size - written, file);
        if (write == 0) {
            break;
        }
        written += static_cast<int>(write);
    } while (written < size);

    fclose(file);

    return written;
}

std::string FileUtil::JoinPath(const std::string& path1, const std::string& path2) {
    if (path1.empty() && path2.empty()) {
        return std::string();
    }
    if (path1.empty()) {
        return path2;
    }
    if (path2.empty()) {
        return path1;
    }

    std::string result = path1;
    if (result[result.size() - 1] != PathUtil::sPathSep[0]) {
        result += PathUtil::sPathSep;
    }
    if (path2[0] == PathUtil::sPathSep[0]) {
        result += path2.substr(1);
    } else {
        result += path2;
    }
    return result;
}

std::string FileUtil::GetFileExtension(const std::string& path) {
    size_t sep = path.find_last_of(".");
    if (sep != std::string::npos) {
        return path.substr(sep + 1);
    }
    return std::string();
}

}  // namespace cefview
