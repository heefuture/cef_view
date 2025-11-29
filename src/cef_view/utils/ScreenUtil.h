/**
 * @file        ScreenUtil.h
 * @brief       Utility class for screen coordinate conversion between logical and device pixels
 * @version     1.0
 * @date        2025.11.29
 */
#pragma once

#include "include/internal/cef_types_wrappers.h"

namespace cefview {

/**
 * @brief Utility class for converting coordinates between logical and device pixels
 */
class ScreenUtil {
public:
    /**
     * @brief Convert value from logical coordinates to device coordinates
     * @param value The value in logical coordinates
     * @param deviceScaleFactor The device scale factor
     * @return The value in device coordinates
     */
    static int LogicalToDevice(int value, float deviceScaleFactor);

    /**
     * @brief Convert rectangle from logical coordinates to device coordinates
     * @param value The rectangle in logical coordinates
     * @param deviceScaleFactor The device scale factor
     * @return The rectangle in device coordinates
     */
    static CefRect LogicalToDevice(const CefRect& value, float deviceScaleFactor);

    /**
     * @brief Convert value from device coordinates to logical coordinates
     * @param value The value in device coordinates
     * @param deviceScaleFactor The device scale factor
     * @return The value in logical coordinates
     */
    static int DeviceToLogical(int value, float deviceScaleFactor);

    /**
     * @brief Convert rectangle from device coordinates to logical coordinates
     * @param value The rectangle in device coordinates
     * @param deviceScaleFactor The device scale factor
     * @return The rectangle in logical coordinates
     */
    static CefRect DeviceToLogical(const CefRect& value, float deviceScaleFactor);

    /**
     * @brief Convert mouse event coordinates from device to logical
     * @param value The mouse event to convert (modified in place)
     * @param deviceScaleFactor The device scale factor
     */
    static void DeviceToLogical(CefMouseEvent& value, float deviceScaleFactor);

    /**
     * @brief Convert touch event coordinates from device to logical
     * @param value The touch event to convert (modified in place)
     * @param deviceScaleFactor The device scale factor
     */
    static void DeviceToLogical(CefTouchEvent& value, float deviceScaleFactor);

private:
    ScreenUtil() = delete;
    ~ScreenUtil() = delete;
    ScreenUtil(const ScreenUtil&) = delete;
    ScreenUtil& operator=(const ScreenUtil&) = delete;
};

}  // namespace cefview
