#include <windows.h>
#include <commctrl.h>

#include "resource.h"
#include "ui/MainWindow.h"

namespace {

void EnableDpiAwareness() {
    using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);

    const HMODULE user32 = ::GetModuleHandleW(L"user32.dll");
    if (user32 != nullptr) {
        auto setContext = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
            ::GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (setContext != nullptr) {
            if (setContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
                return;
            }
        }
    }

    ::SetProcessDPIAware();
}

bool InitCommonControlsWrapper() {
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES;
    const BOOL result = ::InitCommonControlsEx(&icc);
    (void)result;
    return true;
}

} // namespace

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    EnableDpiAwareness();
    ::LoadLibraryW(L"Msftedit.dll");

    if (!InitCommonControlsWrapper()) {
        return 1;
    }

    ui::MainWindow mainWindow(hInstance);
    if (!mainWindow.Create(nCmdShow)) {
        return 2;
    }

    const HACCEL accelerators = ::LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDR_MAIN_ACCEL));

    MSG msg{};
    while (::GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (accelerators != nullptr && ::TranslateAcceleratorW(mainWindow.Handle(), accelerators, &msg)) {
            continue;
        }

        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
