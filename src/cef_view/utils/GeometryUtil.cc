#include "GeometryUtil.h"

#include <cmath>

namespace cefview::util
{

int logicalToDevice(int value, float device_scale_factor) {
    float scaled_val = static_cast<float>(value) * device_scale_factor;
    return static_cast<int>(std::floor(scaled_val));
}

CefRect logicalToDevice(const CefRect &value, float device_scale_factor) {
    return CefRect(logicalToDevice(value.x, device_scale_factor),
                   logicalToDevice(value.y, device_scale_factor),
                   logicalToDevice(value.width, device_scale_factor),
                   logicalToDevice(value.height, device_scale_factor));
}

int deviceToLogical(int value, float device_scale_factor) {
    float scaled_val = static_cast<float>(value) / device_scale_factor;
    return static_cast<int>(std::floor(scaled_val));
}

CefRect deviceToLogical(const CefRect &value, float device_scale_factor) {
    return CefRect(deviceToLogical(value.x, device_scale_factor),
                   deviceToLogical(value.y, device_scale_factor),
                   deviceToLogical(value.width, device_scale_factor),
                   deviceToLogical(value.height, device_scale_factor));
}

void deviceToLogical(CefMouseEvent &value, float device_scale_factor)
{
    value.x = deviceToLogical(value.x, device_scale_factor);
    value.y = deviceToLogical(value.y, device_scale_factor);
}

void deviceToLogical(CefTouchEvent &value, float device_scale_factor)
{
    value.x = deviceToLogical(value.x, device_scale_factor);
    value.y = deviceToLogical(value.y, device_scale_factor);
}

} // namespace cefview::util