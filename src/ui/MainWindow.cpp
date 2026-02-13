#include "ui/MainWindow.h"

#include <windowsx.h>
#include <commctrl.h>
#include <richedit.h>
#include <strsafe.h>

#include <algorithm>
#include <sstream>

#include "resource.h"
#include "ui/WindowActions.h"
#include "ui/WindowBuilder.h"
#include "ui/WindowLayout.h"

namespace {

constexpr UINT WM_APP_SERIAL_DATA = WM_APP + 1;

constexpr GUID kGuidDevinterfaceComport = {
    0x86E0D1E0, 0x8089, 0x11D0, {0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73}
};

RECT CenteredRect(int width, int height) {
    RECT workArea{};
    ::SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

    const int x = workArea.left + ((workArea.right - workArea.left) - width) / 2;
    const int y = workArea.top + ((workArea.bottom - workArea.top) - height) / 2;

    RECT rect{x, y, x + width, y + height};
    return rect;
}

} // namespace

namespace ui {

MainWindow::MainWindow(HINSTANCE instance): 
    instance_(instance),
    window_(nullptr),
    statusBar_(nullptr),
    comboPort_(nullptr),
    comboBaud_(nullptr),
    buttonRefresh_(nullptr),
    buttonOpen_(nullptr),
    buttonClose_(nullptr),
    richLog_(nullptr),
    editSend_(nullptr),
    buttonSend_(nullptr),
    buttonClear_(nullptr),
    ledStatus_(nullptr),
    comboDataBits_(nullptr),
    comboParity_(nullptr),
    comboStopBits_(nullptr),
    comboFlow_(nullptr),
    checkRts_(nullptr),
    checkDtr_(nullptr),
    comboRxMode_(nullptr),
    checkSaveLog_(nullptr),
    // textTxTotal_(nullptr),
    // textRxTotal_(nullptr),
    // textTxRate_(nullptr),
    // textRxRate_(nullptr),
    ledBrushDisconnected_(::CreateSolidBrush(RGB(200, 50, 50))),
    ledBrushConnected_(::CreateSolidBrush(RGB(50, 160, 70))),
    deviceNotify_(nullptr),
    serialPort_(),
    logVirtualizer_(2000, 5000, 5U * 1024U * 1024U),
    rebuildingRichEdit_(false),
    txBytes_(0),
    rxBytes_(0),
    tooltip_(nullptr),
    builder_(std::make_unique<WindowBuilder>(*this)),
    layout_(std::make_unique<WindowLayout>(*this)),
    actions_(std::make_unique<WindowActions>(*this)) {
}

MainWindow::~MainWindow() {
    actions_->ClosePort();
    if (deviceNotify_) ::UnregisterDeviceNotification(deviceNotify_);
}

bool MainWindow::Create(int nCmdShow) {
    if (!RegisterClass()) {
        return false;
    }

    constexpr int kWidth = 900;
    constexpr int kHeight = 700;
    RECT rect = CenteredRect(kWidth, kHeight);

    window_ = ::CreateWindowEx(
        0,
        L"COMTerminalMainWindow",
        L"COM Terminal",
        WS_OVERLAPPEDWINDOW,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        ::LoadMenu(instance_, MAKEINTRESOURCE(IDR_MAIN_MENU)),
        instance_,
        this);

    if (window_ == nullptr) {
        return false;
    }

    // Try enable dark mode for window decorations. If succeeded - use dark theme colors.
    //BOOL value = TRUE;
    //HRESULT hr = ::DwmSetWindowAttribute(window_, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));

    ::ShowWindow(window_, nCmdShow);
    ::UpdateWindow(window_);
    return true;
}

HWND MainWindow::Handle() const noexcept {
    return window_;
}

bool MainWindow::RegisterClass() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &MainWindow::WndProcSetup;
    wc.hInstance = instance_;
    wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = reinterpret_cast<HICON>(::LoadImage(instance_, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"COMTerminalMainWindow";
    wc.hIconSm = reinterpret_cast<HICON>(::LoadImage(instance_, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));

    return ::RegisterClassExW(&wc) != 0;
}

void MainWindow::AppendLog(LogKind kind, const std::wstring& text) {
    if (richLog_ == nullptr) {
        return;
    }

    const COLORREF color = ColorForLogKind(kind);
    
    // Разбиваем текст на строки и добавляем каждую отдельно
    std::wstring line = L"[" + BuildTimestamp() + L"] " + text;
    
    // Заменяем одиночные \r или \n на \r\n
    size_t pos = 0;
    while ((pos = line.find_first_of(L"\r\n", pos)) != std::wstring::npos) {
        if (line[pos] == L'\r' && pos + 1 < line.length() && line[pos + 1] == L'\n') {
            // Это уже \r\n, пропускаем
            pos += 2;
        } else if (line[pos] == L'\r') {
            // Одиночный \r
            line.replace(pos, 1, L"\r\n");
            pos += 2;
        } else if (line[pos] == L'\n') {
            // Одиночный \n
            line.replace(pos, 1, L"\r\n");
            pos += 2;
        }
    }
    
    line += L"\r\n";
    
    const bool saveToDisk = (::SendMessage(checkSaveLog_, BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (saveToDisk && logVirtualizer_.SessionFilePath().empty()) {
        logVirtualizer_.Initialize(L"logs");
    }
    logVirtualizer_.AppendLine(line, color, saveToDisk);
    AppendLineToRichEdit(line, color, true);

    if (!rebuildingRichEdit_ && logVirtualizer_.ShouldRewrite()) {
        RebuildRichEditFromVirtualBuffer();
    }
}

void MainWindow::AppendLineToRichEdit(const std::wstring& line, COLORREF color, bool scrollToCaret) {
    const int end = ::GetWindowTextLengthW(richLog_);
    ::SendMessage(richLog_, EM_SETSEL, static_cast<WPARAM>(end), static_cast<LPARAM>(end));

    CHARFORMAT2W format{};
    format.cbSize = sizeof(format);
    format.dwMask = CFM_COLOR;
    format.crTextColor = color;
    ::SendMessage(richLog_, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));

    ::SendMessage(richLog_, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(line.c_str()));
    if (scrollToCaret) {
        ::SendMessage(richLog_, EM_SCROLLCARET, 0, 0);
    }
}

void MainWindow::RebuildRichEditFromVirtualBuffer() {
    if (richLog_ == nullptr) {
        return;
    }

    rebuildingRichEdit_ = true;

    POINT scrollPos{};
    ::SendMessage(richLog_, EM_GETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&scrollPos));

    CHARRANGE selection{};
    ::SendMessage(richLog_, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&selection));

    ::SendMessage(richLog_, WM_SETREDRAW, FALSE, 0);

    SETTEXTEX setText{};
    setText.flags = ST_DEFAULT;
    setText.codepage = 1200;
    ::SendMessage(richLog_, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&setText), reinterpret_cast<LPARAM>(L""));

    const auto lines = logVirtualizer_.SnapshotBuffer();
    for (const auto& line : lines) {
        AppendLineToRichEdit(line.text, line.color, false);
    }

    const int textLength = ::GetWindowTextLengthW(richLog_);
    const long maxPos = (textLength > 0) ? static_cast<long>(textLength) : 0L;
    selection.cpMin = std::clamp(selection.cpMin, 0L, maxPos);
    selection.cpMax = std::clamp(selection.cpMax, 0L, maxPos);
    if (selection.cpMax < selection.cpMin) {
        selection.cpMax = selection.cpMin;
    }
    ::SendMessage(richLog_, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&selection));
    ::SendMessage(richLog_, EM_SETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&scrollPos));

    ::SendMessage(richLog_, WM_SETREDRAW, TRUE, 0);
    ::RedrawWindow(richLog_, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);

    logVirtualizer_.MarkRewriteDone();
    rebuildingRichEdit_ = false;
}

void MainWindow::UpdateStatusText() {
    wchar_t buffer[128] = {};
    ::StringCchPrintfW(buffer, _countof(buffer), L"TX: %llu  RX: %llu", txBytes_, rxBytes_);
    ::SendMessage(statusBar_, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(buffer));
}

// Update visibility of Open/Close buttons based on connection status.
void MainWindow::UpdateConnectionButtons() {
    if (buttonOpen_ == nullptr || buttonClose_ == nullptr) return;
    BOOL isConnected = serialPort_.IsOpen();
    // Show 'Open' when disconnected, hide otherwise
    ::ShowWindow(buttonOpen_, isConnected ? SW_HIDE : SW_SHOW);
    // Show 'Close' when connected, hide otherwise
    ::ShowWindow(buttonClose_, isConnected ? SW_SHOW : SW_HIDE);
}

// Очистка содержимого терминала и сброс счётчиков
void MainWindow::ClearTerminal() {
    if (richLog_ == nullptr) {
        return;
    }
    // Убираем весь текст из RichEdit
    ::SendMessage(richLog_, EM_SETSEL, 0, -1);
    ::SendMessage(richLog_, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(L"") );

    // Сбрасываем счётчики байтов
    txBytes_ = 0;
    rxBytes_ = 0;
    UpdateStatusText();
}

void MainWindow::ShowLogContextMenu(int x, int y) {
    HMENU menu = ::CreatePopupMenu();
    
    // Используем существующие ID из resource.h
    ::InsertMenu(menu, 0, MF_BYPOSITION, IDM_EDIT_COPY, L"&Copy\tCtrl+C");
    ::InsertMenu(menu, 1, MF_BYPOSITION, IDM_EDIT_SELECTALL, L"Select &All\tCtrl+A");
    ::InsertMenu(menu, 2, MF_SEPARATOR, 0, nullptr);
    ::InsertMenu(menu, 3, MF_BYPOSITION, IDC_BTN_CLEAR, L"&Clear Terminal\tCtrl+L");
    ::InsertMenu(menu, 4, MF_SEPARATOR, 0, nullptr);
    ::InsertMenu(menu, 5, MF_BYPOSITION, IDM_FILE_SAVEAS, L"&Save Log As...");
    // ::InsertMenu(menu, 6, MF_SEPARATOR, 0, nullptr);
    // ::InsertMenu(menu, 7, MF_BYPOSITION, IDM_VIEW_RX, L"Show &RX", MF_CHECKED);
    // ::InsertMenu(menu, 8, MF_BYPOSITION, IDM_VIEW_TX, L"Show &TX", MF_CHECKED);
    // ::InsertMenu(menu, 9, MF_BYPOSITION, IDM_VIEW_SYSTEM, L"Show &System", MF_CHECKED);
    
    ::TrackPopupMenu(menu, TPM_RIGHTBUTTON, x, y, 0, window_, nullptr);
    ::DestroyMenu(menu);
}

void MainWindow::CopySelectedText() {
    if (richLog_ == nullptr) return;
    ::SendMessage(richLog_, WM_COPY, 0, 0);
}

void MainWindow::SelectAllText() {
    if (richLog_ == nullptr) return;
    ::SendMessage(richLog_, EM_SETSEL, 0, -1);
}

void MainWindow::SaveLogToFile() {
    wchar_t filename[MAX_PATH] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = window_;
    ofn.lpstrFilter = L"Log Files (*.log)\0*.log\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"log";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    
    if (::GetSaveFileName(&ofn)) {
        // Сохраняем содержимое richEdit в файл
        std::ofstream file(filename);
        if (file.is_open()) {
            int len = ::GetWindowTextLength(richLog_);
            std::wstring text(len + 1, L'\0');
            ::GetWindowTextW(richLog_, text.data(), len + 1);
            
            // Конвертируем в UTF-8
            int utf8Len = ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
            std::string utf8(utf8Len, '\0');
            ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, utf8.data(), utf8Len, nullptr, nullptr);
            
            file << utf8;
            file.close();
            
            AppendLog(LogKind::System, L"Log saved to: " + std::wstring(filename));
        }
    }
}

std::wstring MainWindow::BuildTimestamp() {
    SYSTEMTIME st{};
    ::GetLocalTime(&st);

    wchar_t buffer[32] = {};
    ::StringCchPrintfW(
        buffer,
        _countof(buffer),
        L"%02u:%02u:%02u.%03u",
        st.wHour,
        st.wMinute,
        st.wSecond,
        st.wMilliseconds);
    return buffer;
}

std::wstring MainWindow::BytesToHex(const std::vector<uint8_t>& bytes) {
    std::wstringstream ss;
    ss.setf(std::ios::uppercase);
    ss << std::hex;

    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i > 0) {
            ss << L' ';
        }
        ss.width(2);
        ss.fill(L'0');
        ss << static_cast<unsigned int>(bytes[i]);
    }

    return ss.str();
}

COLORREF MainWindow::ColorForLogKind(LogKind kind) noexcept {
    switch (kind) {
    case LogKind::Rx:
        return RGB(50, 150, 50);
    case LogKind::Tx:
        return RGB(30, 90, 200);
    case LogKind::System:
        return RGB(120, 120, 120);
    case LogKind::Error:
        return RGB(180, 40, 40);
    }
    return RGB(120, 120, 120);
}

LRESULT CALLBACK MainWindow::WndProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        MainWindow* self = static_cast<MainWindow*>(create->lpCreateParams);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&MainWindow::WndProcThunk));
        self->window_ = hwnd;
        return self->WndProc(hwnd, msg, wParam, lParam);
    }

    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK MainWindow::WndProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* self = reinterpret_cast<MainWindow*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self != nullptr) {
        return self->WndProc(hwnd, msg, wParam, lParam);
    }
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        builder_->CreateStatusBar();
        builder_->CreateControls();
        builder_->FillConnectionDefaults();

        DEV_BROADCAST_DEVICEINTERFACE_W filter{};
        filter.dbcc_size = sizeof(filter);
        filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        filter.dbcc_classguid = kGuidDevinterfaceComport;
        deviceNotify_ = ::RegisterDeviceNotificationW(hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);

        layout_->ResizeChildren();
        // Show the appropriate button based on initial connection state
        UpdateConnectionButtons();
        actions_->RefreshPorts();
        AppendLog(LogKind::System, L"Application started");
        return 0;
    }
    case WM_CONTEXTMENU:
        if (reinterpret_cast<HWND>(wParam) == richLog_) {
            ShowLogContextMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        return 0;
    case WM_SIZE:
        layout_->ResizeChildren();
        return 0;

    case WM_GETMINMAXINFO:
        WindowLayout::ApplyMinTrackSize(reinterpret_cast<MINMAXINFO*>(lParam));
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        // case IDM_VIEW_LIGHT_THEME:
        //     ApplyTheme(false);
        //     return 0;
        // case IDM_VIEW_DARK_THEME:
        //     ApplyTheme(true);
        //     return 0;
        case IDM_FILE_EXIT:
            ::DestroyWindow(hwnd);
            return 0;
        case IDM_PORT_OPEN:
        case IDC_BTN_OPEN:
            actions_->OpenSelectedPort();
            return 0;
        case IDM_PORT_CLOSE:
        case IDC_BTN_CLOSE:
            actions_->ClosePort();
            return 0;
        case IDM_PORT_REFRESH:
        case IDC_BTN_REFRESH:
            actions_->RefreshPorts();
            return 0;
        case IDC_BTN_SEND:
            actions_->SendInputData();
            return 0;
        case IDC_BTN_CLEAR:
            // Call the member function directly; 'owner_' is not a valid identifier here.
            ClearTerminal();
            return 0;
                case IDM_EDIT_COPY:
        CopySelectedText();
        return 0;
        
        case IDM_EDIT_SELECTALL:
            SelectAllText();
            return 0;
            
        case IDM_FILE_SAVEAS:
            SaveLogToFile();
            return 0;
            
        default:
            break;
        }
        break;
        
    //case WM_CTLCOLORBTN: {
    //    HWND control = reinterpret_cast<HWND>(lParam);
    //    HDC dc = reinterpret_cast<HDC>(wParam);

    //    // Проверяем, это Button class (группбокс или чекбокс)
    //    wchar_t className[64] = {};
    //    ::GetClassNameW(control, className, _countof(className));

    //    if (wcscmp(className, WC_BUTTONW) == 0) {
    //        LONG style = ::GetWindowLongW(control, GWL_STYLE);
    //        // GroupBox: делаем текст прозрачным, фон не рисуем
    //        if (style & BS_GROUPBOX) {
    //            ::SetBkMode(dc, TRANSPARENT);
    //            ::SetTextColor(dc, ::GetSysColor(COLOR_WINDOWTEXT));
    //            return reinterpret_cast<LRESULT>(::GetStockObject(NULL_BRUSH));
    //        }

    //        // Checkboxes / Radio buttons: тоже делаем прозрачными
    //        if (style & (BS_AUTOCHECKBOX | BS_CHECKBOX | BS_AUTORADIOBUTTON | BS_RADIOBUTTON)) {
    //            ::SetBkMode(dc, TRANSPARENT);
    //            ::SetTextColor(dc, ::GetSysColor(COLOR_WINDOWTEXT));
    //            return reinterpret_cast<LRESULT>(::GetStockObject(NULL_BRUSH));
    //        }
    //    }
    //    break;
    //}
    
    //case WM_CTLCOLOREDIT: {
    //    HDC dc = reinterpret_cast<HDC>(wParam);
    //    // Use default background for edit controls
    //    ::SetBkColor(dc, ::GetSysColor(COLOR_WINDOW));
    //    ::SetTextColor(dc, ::GetSysColor(COLOR_WINDOWTEXT));
    //    return reinterpret_cast<LRESULT>(::GetSysColorBrush(COLOR_WINDOW));
    //}

    //case WM_CTLCOLORLISTBOX: {
    //    HDC dc = reinterpret_cast<HDC>(wParam);
    //    ::SetBkColor(dc, ::GetSysColor(COLOR_WINDOW));
    //    ::SetTextColor(dc, ::GetSysColor(COLOR_WINDOWTEXT));
    //    return reinterpret_cast<LRESULT>(::GetSysColorBrush(COLOR_WINDOW));
    //}

    case WM_CTLCOLORSTATIC: {
        HWND control = reinterpret_cast<HWND>(lParam);
        HDC dc = reinterpret_cast<HDC>(wParam);
        // Special case: led status has colored background
        if (control == ledStatus_) {
            const bool connected = serialPort_.IsOpen();
            ::SetBkColor(dc, connected ? RGB(50, 160, 70) : RGB(200, 50, 50));
            ::SetTextColor(dc, RGB(255, 255, 255));
            return reinterpret_cast<LRESULT>(connected ? ledBrushConnected_ : ledBrushDisconnected_);
        }
        // For all other static controls make background transparent so they match the window
        ::SetBkMode(dc, TRANSPARENT);
        ::SetTextColor(dc, ::GetSysColor(COLOR_WINDOWTEXT));
        return reinterpret_cast<LRESULT>(::GetStockObject(NULL_BRUSH));
    }

    case WM_DEVICECHANGE:
        if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
            actions_->RefreshPorts();
        }
        return 0;

    case WM_APP_SERIAL_DATA: {
        auto* message = reinterpret_cast<std::vector<uint8_t>*>(lParam);
        if (message != nullptr) {
            actions_->HandleSerialData(*message);
            delete message;
        }
        return 0;
    }

    case WM_DESTROY:
        actions_->ClosePort();
        if (deviceNotify_ != nullptr) {
            ::UnregisterDeviceNotification(deviceNotify_);
            deviceNotify_ = nullptr;
        }
        ::PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace ui
