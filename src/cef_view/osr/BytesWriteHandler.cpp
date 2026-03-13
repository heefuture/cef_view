/**
 * @file        BytesWriteHandler.cpp
 * @brief       Implementation of bytes write handler
 * @version     1.0
 * @author      cef_view project
 * @date        2025-11-19
 */
#include "BytesWriteHandler.h"

#include <cstdio>
#include <cstdlib>

#include "include/wrapper/cef_helpers.h"

namespace cefview {

BytesWriteHandler::BytesWriteHandler(size_t grow)
    : _grow(grow)
    , _datasize(grow)
    , _offset(0) {
    DCHECK_GT(grow, 0U);
    _data = malloc(grow);
    DCHECK(_data != nullptr);
}

BytesWriteHandler::~BytesWriteHandler() {
    if (_data) {
        free(_data);
    }
}

size_t BytesWriteHandler::Write(const void* ptr, size_t size, size_t n) {
    base::AutoLock lock_scope(_lock);
    size_t rv;
    if (_offset + static_cast<int64_t>(size * n) >= _datasize && grow(size * n) == 0) {
        rv = 0;
    } else {
        memcpy(reinterpret_cast<char*>(_data) + _offset, ptr, size * n);
        _offset += size * n;
        rv = n;
    }

    return rv;
}

int BytesWriteHandler::Seek(int64_t offset, int whence) {
    int rv = -1L;
    base::AutoLock lock_scope(_lock);
    switch (whence) {
    case SEEK_CUR:
        if (_offset + offset > _datasize || _offset + offset < 0) {
            break;
        }
        _offset += offset;
        rv = 0;
        break;
    case SEEK_END: {
        int64_t offset_abs = std::abs(offset);
        if (offset_abs > _datasize) {
            break;
        }
        _offset = _datasize - offset_abs;
        rv = 0;
        break;
    }
    case SEEK_SET:
        if (offset > _datasize || offset < 0) {
            break;
        }
        _offset = offset;
        rv = 0;
        break;
    }

    return rv;
}

int64_t BytesWriteHandler::Tell() {
    base::AutoLock lock_scope(_lock);
    return _offset;
}

int BytesWriteHandler::Flush() {
    return 0;
}

size_t BytesWriteHandler::grow(size_t size) {
    _lock.AssertAcquired();
    size_t rv;
    size_t s = (size > _grow ? size : _grow);
    void* tmp = realloc(_data, _datasize + s);
    DCHECK(tmp != nullptr);
    if (tmp) {
        _data = tmp;
        _datasize += s;
        rv = _datasize;
    } else {
        rv = 0;
    }

    return rv;
}

}  // namespace cefview
