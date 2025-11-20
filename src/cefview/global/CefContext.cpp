#include "CefContext.h"
#include "CefManager.h"

namespace cefview {

CefContext::CefContext(const CefConfig& config)
    : _config(config)
{
    iniitialize();
}

CefContext::~CefContext()
{
    CefManager::getInstance()->quitCef();
}

void CefContext::iniitialize() {
    CefManager::getInstance()->initCef(_config);
}

void CefContext::doMessageLoopWork() {
    if (_config.multiThreadedMessageLoop) {
        return;
    }
    CefManager::getInstance()->doCefMessageLoopWork();
}

} // namespace cefview