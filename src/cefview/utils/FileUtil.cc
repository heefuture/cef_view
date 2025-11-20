
#include "FileUtil.h"

#include <algorithm>
#include <cstdio>
#include <memory>

namespace cefview::util {


#ifdef _WIN32
const char kPathSep = '\\';
#else
const char kPathSep = '/';
#endif

std::string readFileToString(const std::string& path, size_t max_size) {
  std::string contents;
  FILE* file = fopen(path.c_str(), "rb");
  if (!file) {
    return false;
  }

  const size_t kBufferSize = 1 << 16;
  std::unique_ptr<char[]> buf(new char[kBufferSize]);
  size_t len;
  size_t size = 0;
  bool read_status = true;

  // Many files supplied in |path| have incorrect size (proc files etc).
  // Hence, the file is read sequentially as opposed to a one-shot read.
  while ((len = fread(buf.get(), 1, kBufferSize, file)) > 0) {
    contents.append(buf.get(), std::min(len, max_size - size));
    if ((max_size - size) < len) {
      read_status = false;
      break;
    }

    size += len;
  }
  read_status = read_status && !ferror(file);
  fclose(file);

  return contents;
}

int writeFile(const std::string& path, const char* data, int size) {
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

std::string joinPath(const std::string& path1, const std::string& path2) {
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
  if (result[result.size() - 1] != kPathSep) {
    result += kPathSep;
  }
  if (path2[0] == kPathSep) {
    result += path2.substr(1);
  } else {
    result += path2;
  }
  return result;
}

std::string getFileExtension(const std::string& path) {
  size_t sep = path.find_last_of(".");
  if (sep != std::string::npos) {
    return path.substr(sep + 1);
  }
  return std::string();
}

}  // namespace cefview::util
