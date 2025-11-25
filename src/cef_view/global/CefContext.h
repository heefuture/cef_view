/**
* @file        CefContext.h
* @brief
* @version     1.0
* @author      heefuture
* @date        2025.11.19
* @copyright
*/
#ifndef CEFCONTEXT_H
#define CEFCONTEXT_H
#pragma once

#include <global/CefConfig.h>

namespace cefview {

class CefContext
{
public:
    CefContext(const CefConfig& config);
    ~CefContext();

    void doMessageLoopWork();
    const CefConfig& getCefConfig() const { return _config; }
private:
    void iniitialize();

private:
    CefConfig _config;

}; // class CefContext


} // namespace cefview

#endif //!CEFCONTEXT_H