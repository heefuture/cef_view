/**
 * @file        BytesWriteHandler.h
 * @brief       Bytes write handler for stream writing operations
 * @version     1.0
 * @author      cef_view project
 * @date        2025-11-19
 * @copyright
 */
#ifndef BYTESWRITEHANDLER_H
#define BYTESWRITEHANDLER_H
#pragma once

#include "include/base/cef_lock.h"
#include "include/cef_stream.h"

namespace cefview {

/**
 * @brief Bytes write handler for stream writing operations
 */
class BytesWriteHandler : public CefWriteHandler {
public:
    /**
     * @brief Constructor
     * @param grow Growth size for buffer reallocation
     */
    explicit BytesWriteHandler(size_t grow);

    /**
     * @brief Destructor
     */
    ~BytesWriteHandler() override;

    /**
     * @brief Write data to the stream
     * @param ptr Pointer to data to write
     * @param size Size of each element
     * @param n Number of elements
     * @return Number of elements written
     */
    size_t Write(const void* ptr, size_t size, size_t n) override;

    /**
     * @brief Seek to a specific position
     * @param offset Offset from whence
     * @param whence Reference position (SEEK_SET, SEEK_CUR, SEEK_END)
     * @return 0 on success, -1 on error
     */
    int Seek(int64_t offset, int whence) override;

    /**
     * @brief Get current position
     * @return Current position in the stream
     */
    int64_t Tell() override;

    /**
     * @brief Flush the stream
     * @return 0 on success
     */
    int Flush() override;

    /**
     * @brief Check if the stream may block
     * @return false (this stream never blocks)
     */
    bool MayBlock() override { return false; }

    /**
     * @brief Get pointer to the data buffer
     * @return Pointer to data buffer
     */
    void* GetData() { return _data; }

    /**
     * @brief Get size of data written
     * @return Data size in bytes
     */
    int64_t GetDataSize() { return _offset; }

private:
    /**
     * @brief Grow the buffer by the specified size
     * @param size Size to grow
     * @return New buffer size, or 0 on failure
     */
    size_t grow(size_t size);

    size_t _grow;       ///< Growth size for reallocation
    void* _data;        ///< Data buffer pointer
    int64_t _datasize;  ///< Total buffer size
    int64_t _offset;    ///< Current write offset

    base::Lock _lock;   ///< Thread synchronization lock

    IMPLEMENT_REFCOUNTING(BytesWriteHandler);
    DISALLOW_COPY_AND_ASSIGN(BytesWriteHandler);
};

}  // namespace cefview

#endif  // BYTESWRITEHANDLER_H
