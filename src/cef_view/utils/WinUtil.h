/**
* @file        WinUtil.h
* @brief       Header file for Windows utility functions.
* @version     1.0
* @author      heefuture
* @date        2025.11.18
* @copyright
*/
#ifndef WINUTIL_H
#define WINUTIL_H
#pragma once

#include <windows.h>

#include <string>

#include "include/internal/cef_types_wrappers.h"

namespace cefview::util {

// Returns the current time in microseconds.
uint64_t getTimeNow();

// Set the window's user data pointer.
void setUserDataPtr(HWND hWnd, void* ptr);

// Return the window's user data pointer.
template <typename T>
T getUserDataPtr(HWND hWnd) {
  return reinterpret_cast<T>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
}

// Set the window's window procedure pointer and return the old value.
WNDPROC setWndProcPtr(HWND hWnd, WNDPROC wndProc);

bool isWindows8OrNewer();

// Return the resource string with the specified id.
std::wstring getResourceString(UINT id);

bool isMouseEventFromTouch(UINT message);

int getCefMouseModifiers(WPARAM wparam);

int getCefKeyboardModifiers(WPARAM wparam, LPARAM lparam);

bool isKeyDown(WPARAM wparam);

bool isProcessPerMonitorDpiAware();

float getWindowScaleFactor(HWND hwnd);
// Returns the device scale factor. For example, 200% display scaling will
// return 2.0.
float getDeviceScaleFactor();

CefRect getWindowRect(HWND hwnd, float deviceScaleFactor);

}  // namespace cefview::util

#endif  // WINUTIL_H
