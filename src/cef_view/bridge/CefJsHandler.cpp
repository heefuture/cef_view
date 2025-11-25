#include "CefJsHandler.h"
#include "CefJsBridgeRender.h"

#include <client/CefSwitches.h>
#include <client/CefViewApp.h>
#include <utils/Util.h>

#include <sstream>
namespace cefview
{
// Forward declarations.
void SetList(CefRefPtr<CefV8Value> source, CefRefPtr<CefListValue> target);
void SetList(CefRefPtr<CefListValue> source, CefRefPtr<CefV8Value> target);

// Transfer a V8 value to a List index.
void SetListValue(CefRefPtr<CefListValue> list, int index,
                 CefRefPtr<CefV8Value> value) {
   if (value->IsArray()) {
       CefRefPtr<CefListValue> new_list = CefListValue::Create();
       SetList(value, new_list);
       list->SetList(index, new_list);
   } else if (value->IsString()) {
       list->SetString(index, value->GetStringValue());
   } else if (value->IsBool()) {
       list->SetBool(index, value->GetBoolValue());
   } else if (value->IsInt()) {
       list->SetInt(index, value->GetIntValue());
   } else if (value->IsDouble()) {
       list->SetDouble(index, value->GetDoubleValue());
   }
}

// Transfer a V8 array to a List.
void SetList(CefRefPtr<CefV8Value> source, CefRefPtr<CefListValue> target) {
   assert(source->IsArray());

   int arg_length = source->GetArrayLength();
   if (arg_length == 0)
       return;

   // Start with null types in all spaces.
   target->SetSize(arg_length);

   for (int i = 0; i < arg_length; ++i)
       SetListValue(target, i, source->GetValue(i));
}

// Transfer a List value to a V8 array index.
void SetListValue(CefRefPtr<CefV8Value> list, int index,
                 CefRefPtr<CefListValue> value) {
   CefRefPtr<CefV8Value> new_value;

   CefValueType type = value->GetType(index);
   switch (type) {
   case VTYPE_LIST: {
       CefRefPtr<CefListValue> list = value->GetList(index);
       new_value = CefV8Value::CreateArray(static_cast<int>(list->GetSize()));
       SetList(list, new_value);
   } break;
   case VTYPE_BOOL:
       new_value = CefV8Value::CreateBool(value->GetBool(index));
       break;
   case VTYPE_DOUBLE:
       new_value = CefV8Value::CreateDouble(value->GetDouble(index));
       break;
   case VTYPE_INT:
       new_value = CefV8Value::CreateInt(value->GetInt(index));
       break;
   case VTYPE_STRING:
       new_value = CefV8Value::CreateString(value->GetString(index));
       break;
   default:
       break;
   }

   if (new_value.get()) {
       list->SetValue(index, new_value);
   } else {
       list->SetValue(index, CefV8Value::CreateNull());
   }
}

// Transfer a List to a V8 array.
void SetList(CefRefPtr<CefListValue> source, CefRefPtr<CefV8Value> target) {
   assert(target->IsArray());

   int arg_length = static_cast<int>(source->GetSize());
   if (arg_length == 0)
       return;

   for (int i = 0; i < arg_length; ++i)
       SetListValue(target, i, source);
}
//////////////////////////////////////////////////////////////////////////////////////

bool CefJSHandler::Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
{
   // 当Web中调用了"CallFunction"函数后，会触发到这里，然后把参数保存，转发到Broswer进程
   // Broswer进程的BrowserHandler类在OnProcessMessageReceived接口中处理kJsCallbackMessage消息，就可以收到这个消息
   if (arguments.size() < 2) {
       exception = "Invalid arguments.";
       return false;
   }

   CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
   CefRefPtr<CefFrame> frame = context->GetFrame();
   CefRefPtr<CefBrowser> browser = context->GetBrowser();

   int64_t browserId = browser->GetIdentifier();
   CefString frameId = frame->GetIdentifier();

   if (name == "call") {
       // 允许没有参数列表的调用，第二个参数为回调
       // 如果传递了参数列表，那么回调是第三个参数
       CefString function_name = arguments[0]->GetStringValue();
       CefString params = "{}";
       CefRefPtr<CefV8Value> callback;
       if (arguments[0]->IsString() && arguments[1]->IsFunction()) {
           callback = arguments[1];
       }
       else if (arguments[0]->IsString() && arguments[1]->IsString() && arguments[2]->IsFunction()) {
           params = arguments[1]->GetStringValue();
           callback = arguments[2];
       }
       else {
           exception = "Invalid arguments.";
           return false;
       }

       // 执行 C++ 方法
       if (!_jsBridge->callCppFunction(function_name, params, callback)) {
           std::string functionNameStr = "Failed to call function " + function_name.ToString() + ".";
           exception = functionNameStr.c_str();
           return false;
       }

       return true;
   }
   else if (name == "register" || name == "setMessageCallback") {
       if (arguments[0]->IsString() && arguments[1]->IsFunction())
       {
           std::string function_name = arguments[0]->GetStringValue();
           CefRefPtr<CefV8Value> callback = arguments[1];
           if (!_jsBridge->registerJSFunc(function_name, callback))
           {
               exception = "Failed to register function.";
               return false;
           }
           return true;
       }
       else {
           exception = "Invalid arguments.";
           return false;
       }
   } else if (name == "unRegister" || name == "removeMessageCallback") {
       if (arguments[0]->IsString())
       {
           std::string function_name = arguments[0]->GetStringValue();
           if (!_jsBridge->unRegisterJSFunc(function_name, frame))
           {
               exception = "Failed to unRegister function.";
               return false;
           }
           return true;
       }
       else {
           exception = "Invalid arguments.";
           return false;
       }
   } else if (name == "sendMessage") {
       // Send a message to the browser process.
       if ((arguments.size() == 1 || arguments.size() == 2) && arguments[0]->IsString() && frame) {
            CefString msgName = arguments[0]->GetStringValue();
            if (!msgName.empty())
            {
                CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(msgName);
                // Translate the arguments, if any.
                if (arguments.size() == 2 && arguments[1]->IsArray())
                    SetList(arguments[1], message->GetArgumentList());
                frame->SendProcessMessage(PID_BROWSER, message);
                return true;
            }
       }
   }

   return false;
}

}
