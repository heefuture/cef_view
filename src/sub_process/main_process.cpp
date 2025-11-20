#include "include/cef_base.h"
#include "include/cef_app.h"
#include <client/CefViewApp.h>

#define STRINGIFY(x) #x
#define SVN_REVISION_STR(x) STRINGIFY(x)

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

  // Structure for passing command-line arguments.
  // The definition of this structure is platform-specific.
  CefMainArgs main_args(hInstance);

  // Optional implementation of the CefApp interface.
  CefRefPtr<cefview::CefViewApp> app(new cefview::CefViewApp);

  // Execute the sub-process logic. This will block until the sub-process should exit.
  return CefExecuteProcess(main_args, app.get(), nullptr);
}

