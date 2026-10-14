#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "ui/MainWindow.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/, int nCmdShow) {
    // Enable modern Per-Monitor V2 DPI awareness if supported
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef BOOL (WINAPI *pfnSetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
        auto fnSetAwareness = reinterpret_cast<pfnSetProcessDpiAwarenessContext>(
            GetProcAddress(hUser32, "SetProcessDpiAwarenessContext")
        );
        if (fnSetAwareness) {
            fnSetAwareness(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        } else {
            SetProcessDPIAware();
        }
    }

    return DisplayStabilizer::MainWindow::run(hInstance, nCmdShow);
}
