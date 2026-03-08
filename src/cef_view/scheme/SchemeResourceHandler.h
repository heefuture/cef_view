/**
 * @file        SchemeResourceHandler.h
 * @brief       Base class for custom scheme resource handlers
 * @version     1.0
 * @date        2025.01.09
 */
#pragma once

#include "include/cef_resource_handler.h"
#include "include/cef_scheme.h"
#include <string>

namespace cefview {

/**
 * @brief Base class for custom scheme resource handlers
 *
 * Provides base interface for handling custom scheme requests.
 * Subclasses should implement the handler methods.
 */
class SchemeResourceHandler : public CefResourceHandler {
public:
    explicit SchemeResourceHandler(const std::string& basePath);
    virtual ~SchemeResourceHandler() = default;

    bool Open(CefRefPtr<CefRequest> request, bool& handleRequest,
              CefRefPtr<CefCallback> callback) override {
        (void)request;
        (void)callback;
        handleRequest = false;
        return false;
    }

    void GetResponseHeaders(CefRefPtr<CefResponse> response,
                           int64_t& responseLength, CefString& redirectUrl) override {
        (void)response;
        (void)redirectUrl;
        responseLength = 0;
    }

    bool Read(void* dataOut, int bytesToRead, int& bytesRead,
              CefRefPtr<CefResourceReadCallback> callback) override {
        (void)dataOut;
        (void)bytesToRead;
        (void)callback;
        bytesRead = 0;
        return false;
    }

    void Cancel() override {}

protected:
    std::string _basePath;

    IMPLEMENT_REFCOUNTING(SchemeResourceHandler);
    DISALLOW_COPY_AND_ASSIGN(SchemeResourceHandler);
};

/**
 * @brief Template factory for creating scheme handlers
 *
 * @tparam HandlerType The concrete handler type derived from SchemeResourceHandler
 */
template<typename HandlerType>
class SchemeHandlerFactory : public CefSchemeHandlerFactory {
public:
    explicit SchemeHandlerFactory(const std::string& basePath)
        : _basePath(basePath) {}

    CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> frame,
                                         const CefString& schemeName,
                                         CefRefPtr<CefRequest> request) override {
        (void)browser;
        (void)frame;
        (void)schemeName;
        (void)request;

        std::string url = request->GetURL();
        return new HandlerType(_basePath);
    }

private:
    std::string _basePath;
    IMPLEMENT_REFCOUNTING(SchemeHandlerFactory);
    DISALLOW_COPY_AND_ASSIGN(SchemeHandlerFactory);
};

}  // namespace cefview
