/**
* @file        GeometryUtil.h
* @brief       Header file for geometry utility functions.
* @version     1.0
* @author      heefuture
* @date        2025.11.18
* @copyright
*/
#ifndef GEOMETRYUTIL_H
#define GEOMETRYUTIL_H
#pragma once

#include "include/internal/cef_types_wrappers.h"

namespace cefview::util {

// Convert |value| from logical coordinates to device coordinates.
int logicalToDevice(int value, float device_scale_factor);
CefRect logicalToDevice(const CefRect& value, float device_scale_factor);

// Convert |value| from device coordinates to logical coordinates.
int deviceToLogical(int value, float device_scale_factor);
CefRect deviceToLogical(const CefRect& value, float device_scale_factor);
void deviceToLogical(CefMouseEvent& value, float device_scale_factor);
void deviceToLogical(CefTouchEvent& value, float device_scale_factor);

}  // namespace cefview::util

#endif //!GEOMETRYUTIL_H
