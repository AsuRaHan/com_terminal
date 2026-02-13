#include <windows.h>
#include "resource.h"
#include "ui/MainWindow.h"

namespace {

bool EnableDpiAwareness() {
    using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);

    const HMODULE user32 = ::GetModuleHandle(L"user32.dll");
    if (user32 != nullptr) {
        auto setContext = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
            ::GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (setContext != nullptr) {
            if (setContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
                return true;
            }
        }
    }

    // Fallback to legacy DPI awareness
    if (::SetProcessDPIAware()) {
        return true;
    }
    return false;
}

} // namespace

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow){
    if (!EnableDpiAwareness()) {
        ::MessageBox(nullptr, L"Failed to enable DPI awareness.", L"Error", MB_ICONERROR);
        return 1;
    }
    HRESULT hr = S_OK;
    HMODULE hMsftedit = ::LoadLibrary(L"msftedit.dll"); // Загружаем библиотеку для RichEdit 4.1 или выше
    if (!hMsftedit)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }
    if (!SUCCEEDED(hr)) {
        ::MessageBox(nullptr, L"Failed to load Msftedit.dll.", L"Error", MB_ICONERROR);
        return 2;
    }

    ui::MainWindow mainWindow(hInstance);
    if (!mainWindow.Create(nCmdShow)) {
        return 3;
    }

    const HACCEL accelerators = ::LoadAccelerators(hInstance, MAKEINTRESOURCEW(IDR_MAIN_ACCEL));

    MSG msg{};
    while (::GetMessage(&msg, nullptr, 0, 0) > 0) {
        if (accelerators != nullptr && ::TranslateAccelerator(mainWindow.Handle(), accelerators, &msg)) {
            continue;
        }

        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}
