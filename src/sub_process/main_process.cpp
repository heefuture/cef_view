#include "include/cef_base.h"
#include "include/cef_app.h"
#include "cefclient/client_app.h"
#include "mycrashhandler.h"
#include "HSAngCrashHelper.h"

#define STRINGIFY(x) #x
#define SVN_REVISION_STR(x) STRINGIFY(x)

using namespace std;

//void crash()
//{
//	volatile int *a = (int *)(NULL);
//	*a = 1;
//}
// Program entry point function.
int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  // Enable High-DPI support on Windows 7 or newer.
  CefEnableHighDPISupport();


	MyCrashHandler *crashHandler = MyCrashHandler::getInstance();
	crashHandler->setParam(KEY_OS_TYPE, "windesktop");
	crashHandler->setParam(KEY_PROJECT, "dep477");
	crashHandler->setParam(KEY_APP_KEY, "653b8f9e0a1d8254c156e0acab45551a");
	crashHandler->setParam(KEY_ENGINE_VER, HSAngCrashHelper::GetVersion().c_str());
	crashHandler->setParam(KEY_RES_VER, SVN_REVISION_STR(SVN_REVISION));
	HSAngCrashHelper::get().getUrs([crashHandler](string urs) {
		crashHandler->setParam(KEY_UID, urs.c_str());
		crashHandler->setParam(KEY_URS, urs.c_str());
	});
	crashHandler->startCrashHunter();

  // Structure for passing command-line arguments.
  // The definition of this structure is platform-specific.
  CefMainArgs main_args(hInstance);

  // Optional implementation of the CefApp interface.
  CefRefPtr<ClientApp> app(new ClientApp);

  // Execute the sub-process logic. This will block until the sub-process should exit.
  return CefExecuteProcess(main_args, app.get(), nullptr);
}

