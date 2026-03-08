/**
 * @file OsrRenderer.cpp
 * @brief Off-screen renderer base class implementation
 *
 * This file is part of CefView project.
 * Licensed under BSD-style license.
 */
#include "OsrRenderer.h"

namespace cefview {

OsrRenderer::OsrRenderer(bool transparent)
    : _transparent(transparent) {
}

}  // namespace cefview
