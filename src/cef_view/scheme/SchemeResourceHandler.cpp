/**
 * @file        SchemeResourceHandler.cpp
 * @brief       Base class implementation for custom scheme resource handlers
 * @version     1.0
 * @date        2025.01.09
 */
#include "SchemeResourceHandler.h"

namespace cefview {

SchemeResourceHandler::SchemeResourceHandler(const std::string& basePath)
    : _basePath(basePath) {
}

}  // namespace cefview
