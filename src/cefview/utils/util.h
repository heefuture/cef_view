/**
* @file        util.h
* @brief       Utility functions and macros for CEF client.
* @version     1.0
* @author      heefuture
* @date        2025.11.18
* @copyright
*/
#ifndef UTIL_H_
#define UTIL_H_
#pragma once

#include "include/cef_task.h"

#ifdef _WIN32

#include <windows.h>
#ifndef NDEBUG
#define ASSERT(condition) if (!(condition)) { DebugBreak(); }
#else
#define ASSERT(condition) ((void)0)
#endif

#else  // !_WIN32

#include <assert.h>  // NOLINT(build/include_order)

#ifndef NDEBUG
#define ASSERT(condition) if (!(condition)) { assert(false); }
#else
#define ASSERT(condition) ((void)0)
#endif

#endif  // !_WIN32

#define REQUIRE_UI_THREAD()   ASSERT(CefCurrentlyOn(TID_UI));
#define REQUIRE_IO_THREAD()   ASSERT(CefCurrentlyOn(TID_IO));
#define REQUIRE_FILE_THREAD() ASSERT(CefCurrentlyOn(TID_FILE));

#endif  // UTIL_H_
