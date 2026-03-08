/**
 * @file LogUtil.h
 * @brief Logging utility for CefView project
 *
 * Provides unified logging interface based on CEF native logging.
 * Usage: LOGD << "message"; LOGI << "value=" << value;
 */
#ifndef LOGUTIL_H
#define LOGUTIL_H
#pragma once

#include "include/base/cef_logging.h"

namespace cefview {

// Log level aliases using CEF logging
#define LOGV VLOG(1)
#define LOGD DLOG(INFO)
#define LOGI LOG(INFO)
#define LOGW LOG(WARNING)
#define LOGE LOG(ERROR)
#define LOGF LOG(FATAL)

// Conditional logging
#define LOGV_IF(cond) VLOG_IF(1, cond)
#define LOGD_IF(cond) DLOG_IF(INFO, cond)
#define LOGI_IF(cond) LOG_IF(INFO, cond)
#define LOGW_IF(cond) LOG_IF(WARNING, cond)
#define LOGE_IF(cond) LOG_IF(ERROR, cond)

}  // namespace cefview

#endif  // !LOGUTIL_H
