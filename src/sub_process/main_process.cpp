#include "include/cef_base.h"
#include "include/cef_app.h"
#include "cefclient/client_app.h"

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


  // Structure for passing command-line arguments.
  // The definition of this structure is platform-specific.
  CefMainArgs main_args(hInstance);

  // Optional implementation of the CefApp interface.
  CefRefPtr<ClientApp> app(new ClientApp);

  // Execute the sub-process logic. This will block until the sub-process should exit.
  return CefExecuteProcess(main_args, app.get(), nullptr);
}

