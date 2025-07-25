#include <algorithm>
#include <string>
#include <memory>
#include "ProcessMessageDelegateWrapper.h"
#include "include/wrapper/cef_stream_resource_handler.h"

using namespace cef;
ProcessMessageDelegateWrapper::ProcessMessageDelegateWrapper(cef::ProcessMessageHandler* msgHandler) :
    _handler(msgHandler, [](cef::ProcessMessageHandler* inst) { inst->releseAndDelete(); }) {
}

  // From CefHandler::ProcessMessageDelegate.
bool ProcessMessageDelegateWrapper::onProcessMessageReceived(
        CefRefPtr<CefHandler> handler,
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefProcessId source_process,
        CefRefPtr<CefProcessMessage> message) {

    std::string message_name = message->GetName();
    if (message_name.find("doQuery") == 0) {
        // Handle the message.
        std::string result;
        CefRefPtr<CefListValue> args = message->GetArgumentList();
        if (args->GetSize() > 0 && args->GetType(0) == VTYPE_STRING) {
            std::string strArgs = args->GetString(0).ToString();
            result = _handler->hanldeProcessMessage(strArgs);
        }
        else {
            result = "Invalid request";
        }

        // Send the result back to the render process.
        CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create(message_name);

        response->GetArgumentList()->SetString(0, result);
        frame->SendProcessMessage(PID_RENDERER, response);

        return true;
    }
    return false;
 }