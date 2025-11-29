#include "ScreenUtil.h"

#include <cmath>

namespace cefview {

int ScreenUtil::LogicalToDevice(int value, float deviceScaleFactor) {
    float scaledVal = static_cast<float>(value) * deviceScaleFactor;
    return static_cast<int>(std::floor(scaledVal));
}

CefRect ScreenUtil::LogicalToDevice(const CefRect& value, float deviceScaleFactor) {
    return CefRect(LogicalToDevice(value.x, deviceScaleFactor),
                   LogicalToDevice(value.y, deviceScaleFactor),
                   LogicalToDevice(value.width, deviceScaleFactor),
                   LogicalToDevice(value.height, deviceScaleFactor));
}

int ScreenUtil::DeviceToLogical(int value, float deviceScaleFactor) {
    float scaledVal = static_cast<float>(value) / deviceScaleFactor;
    return static_cast<int>(std::floor(scaledVal));
}

CefRect ScreenUtil::DeviceToLogical(const CefRect& value, float deviceScaleFactor) {
    return CefRect(DeviceToLogical(value.x, deviceScaleFactor),
                   DeviceToLogical(value.y, deviceScaleFactor),
                   DeviceToLogical(value.width, deviceScaleFactor),
                   DeviceToLogical(value.height, deviceScaleFactor));
}

void ScreenUtil::DeviceToLogical(CefMouseEvent& value, float deviceScaleFactor) {
    value.x = DeviceToLogical(value.x, deviceScaleFactor);
    value.y = DeviceToLogical(value.y, deviceScaleFactor);
}

void ScreenUtil::DeviceToLogical(CefTouchEvent& value, float deviceScaleFactor) {
    value.x = DeviceToLogical(value.x, deviceScaleFactor);
    value.y = DeviceToLogical(value.y, deviceScaleFactor);
}

}  // namespace cefview
