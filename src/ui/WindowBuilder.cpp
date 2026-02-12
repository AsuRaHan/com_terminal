#include "ui/WindowBuilder.h"

#include <uxtheme.h>
// #pragma comment(lib, "uxtheme.lib")

namespace ui {

WindowBuilder::WindowBuilder(MainWindow& owner) : owner_(owner) {}

void WindowBuilder::CreateStatusBar() {
    owner_.statusBar_ = ::CreateWindowEx(
        0,
        STATUSCLASSNAMEW,
        nullptr,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATUS_BAR)),
        owner_.instance_,
        nullptr);

    if (!owner_.statusBar_) {
        OutputDebugString(L"Failed to create status bar\n");
        return;
    }

    int parts[3] = {260, 520, -1}; // Три части: статус, статистика, подсказки
    ::SendMessage(owner_.statusBar_, SB_SETPARTS, 3, reinterpret_cast<LPARAM>(parts));
    ::SendMessage(owner_.statusBar_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(L"Disconnected"));
    ::SendMessage(owner_.statusBar_, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(L"TX: 0  RX: 0"));
    ::SendMessage(owner_.statusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"Ready"));
    // ::SendMessage(owner_.statusBar_, SB_SETTEXTW, 3, reinterpret_cast<LPARAM>(L"Press F1 for help"));
}

void WindowBuilder::CreateTooltips() {
    // Создаем окно подсказок
    owner_.tooltip_ = ::CreateWindowEx(
        0,  // Убираем WS_EX_TOPMOST - нахуй не надо
        TOOLTIPS_CLASS,
        nullptr,
        WS_POPUP | TTS_ALWAYSTIP,  // Убираем TTS_BALLOON и TTS_CLOSE - хуйня
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        owner_.window_,
        nullptr,
        owner_.instance_,
        nullptr);

    if (!owner_.tooltip_) {
        OutputDebugString(L"Failed to create tooltips\n");
        return;
    }

    // Настройка
    ::SendMessage(owner_.tooltip_, TTM_SETMAXTIPWIDTH, 0, 400);
    ::SendMessage(owner_.tooltip_, TTM_SETDELAYTIME, TTDT_INITIAL, 500);
    ::SendMessage(owner_.tooltip_, TTM_SETDELAYTIME, TTDT_AUTOPOP, 8000);
    
    // ВАЖНО: Активируем тултипы
    ::SendMessage(owner_.tooltip_, TTM_ACTIVATE, TRUE, 0);

    AddAllTooltips();
}

void WindowBuilder::AddTooltip(HWND control, const std::wstring& text, const std::wstring& title) {
    if (!owner_.tooltip_ || !control || !::IsWindow(control)) return;

    TOOLINFOW ti{};
    ti.cbSize = sizeof(TOOLINFOW);
    
    // ВАЖНО: Правильная комбинация флагов
    ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;  // TTF_TRANSPARENT нахуй не нужен
    ti.hwnd = owner_.window_;  // Родительское окно
    ti.uId = reinterpret_cast<UINT_PTR>(control);  // HWND как ID
    ti.lpszText = const_cast<LPWSTR>(text.c_str());
    
    // ВАЖНО: Сначала добавляем тултип
    ::SendMessage(owner_.tooltip_, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
    
    // ВАЖНО: Потом устанавливаем заголовок (отдельным сообщением)
    if (!title.empty()) {
        ::SendMessage(owner_.tooltip_, TTM_SETTITLE, 0, reinterpret_cast<LPARAM>(title.c_str()));
    }
}



void WindowBuilder::AddAllTooltips() {    
    // ============ Port Settings ============
    AddTooltip(owner_.comboPort_, 
        L"Select COM port. Click Refresh to update list.",
        L"COM Port");

    AddTooltip(owner_.comboBaud_,
        L"Baud rate (9600, 19200, 38400, 57600, 115200, etc)",
        L"Baud Rate");

    AddTooltip(owner_.buttonRefresh_,
        L"Scan for available COM ports",
        L"Refresh");

    AddTooltip(owner_.buttonOpen_,
        L"Open selected COM port",
        L"Open Port");

    AddTooltip(owner_.buttonClose_,
        L"Close current COM port",
        L"Close Port");

    AddTooltip(owner_.comboDataBits_,
        L"Data bits: 5, 6, 7, or 8",
        L"Data Bits");

    AddTooltip(owner_.comboParity_,
        L"Parity: None, Odd, Even, Mark, Space",
        L"Parity");

    AddTooltip(owner_.comboStopBits_,
        L"Stop bits: 1, 1.5, or 2",
        L"Stop Bits");

    AddTooltip(owner_.comboFlow_,
        L"Flow control: None, RTS/CTS, XON/XOFF",
        L"Flow Control");

    AddTooltip(owner_.checkRts_,
        L"Request To Send signal",
        L"RTS");

    AddTooltip(owner_.checkDtr_,
        L"Data Terminal Ready signal",
        L"DTR");

    // ============ Statistics ============
    AddTooltip(owner_.textTxTotal_,
        L"Total bytes transmitted",
        L"TX Total");

    AddTooltip(owner_.textRxTotal_,
        L"Total bytes received",
        L"RX Total");

    AddTooltip(owner_.textTxRate_,
        L"Current transmit speed",
        L"TX Rate");

    AddTooltip(owner_.textRxRate_,
        L"Current receive speed",
        L"RX Rate");

    // ============ Terminal Log ============
    AddTooltip(owner_.richLog_,
        L"Terminal output - Green:RX Blue:TX Gray:System Red:Error",
        L"Terminal Log");

    // ============ Terminal Control ============
    AddTooltip(owner_.ledStatus_,
        L"Connection status - Green:Open Red:Closed",
        L"Status");

    AddTooltip(owner_.comboRxMode_,
        L"Display mode: Text (UTF-8) or HEX",
        L"RX Mode");

    AddTooltip(owner_.checkSaveLog_,
        L"Save log to file",
        L"Save Log");

    AddTooltip(owner_.buttonClear_,
        L"Clear terminal and reset counters",
        L"Clear");

    // ============ Send Data ============
    AddTooltip(owner_.editSend_,
        L"Data to send - Text or HEX (space separated)",
        L"Send Buffer");

    AddTooltip(owner_.buttonSend_,
        L"Send data",
        L"Send");

    // ============ Groups ============
    AddTooltip(owner_.groupPort_,
        L"Serial port configuration",
        L"Port Settings");

    AddTooltip(owner_.groupStats_,
        L"Communication statistics",
        L"Statistics");

    AddTooltip(owner_.groupLog_,
        L"Terminal communication log",
        L"Log");

    AddTooltip(owner_.groupTerminalCtrl_,
        L"Terminal display controls",
        L"Terminal Control");

    AddTooltip(owner_.groupSend_,
        L"Send data to device",
        L"Send Data");
}




void WindowBuilder::CreateControls() {
    // ============ СНАЧАЛА СОЗДАЕМ ГРУППЫ ============
    owner_.groupPort_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        L"Port Settings",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_GROUP_PORT)),
        owner_.instance_,
        nullptr);

    // owner_.groupStats_ = ::CreateWindowEx(
    //     0,
    //     WC_BUTTONW,
    //     L"Statistics",
    //     WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
    //     0, 0, 0, 0,
    //     owner_.window_,
    //     reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_GROUP_STATS)),
    //     owner_.instance_,
    //     nullptr);

    owner_.groupLog_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        L"Terminal Log",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_GROUP_LOG)),
        owner_.instance_,
        nullptr);

    owner_.groupTerminalCtrl_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        L"Terminal Control",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_GROUP_TERMINAL_CTRL)),
        owner_.instance_,
        nullptr);

    owner_.groupSend_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        L"Send Data",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_GROUP_SEND)),
        owner_.instance_,
        nullptr);

    // ============ ТЕПЕРЬ СОЗДАЕМ ЭЛЕМЕНТЫ УПРАВЛЕНИЯ ============
    
    // === Port Settings элементы ===
    owner_.buttonRefresh_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        L"Refresh",  // Добавил символ для наглядности
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_REFRESH)),
        owner_.instance_,
        nullptr);

    owner_.comboPort_ = ::CreateWindowEx(
        0,
        WC_COMBOBOXW,
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        0, 0, 0, 200,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_PORT)),
        owner_.instance_,
        nullptr);

    owner_.comboBaud_ = ::CreateWindowEx(
        0,
        WC_COMBOBOXW,
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_TABSTOP,
        0, 0, 0, 200,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_BAUD)),
        owner_.instance_,
        nullptr);

    // Добавляем скорости
    constexpr const wchar_t* baudRates[] = {L"9600", L"19200", L"38400", L"57600", L"115200", L"230400", L"460800", L"921600"};
    for (const auto* rate : baudRates) {
        ::SendMessage(owner_.comboBaud_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(rate));
    }
    ::SetWindowText(owner_.comboBaud_, L"115200");

    owner_.buttonOpen_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        L"▶ Open",  // Символ play
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_OPEN)),
        owner_.instance_,
        nullptr);

    owner_.buttonClose_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        L"■ Close",  // Символ stop
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_CLOSE)),
        owner_.instance_,
        nullptr);

    owner_.comboDataBits_ = ::CreateWindowEx(
        0,
        WC_COMBOBOXW,
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        0, 0, 0, 200,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_DATABITS)),
        owner_.instance_,
        nullptr);

    owner_.comboParity_ = ::CreateWindowEx(
        0,
        WC_COMBOBOXW,
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        0, 0, 0, 200,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_PARITY)),
        owner_.instance_,
        nullptr);

    owner_.comboStopBits_ = ::CreateWindowEx(
        0,
        WC_COMBOBOXW,
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        0, 0, 0, 200,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_STOPBITS)),
        owner_.instance_,
        nullptr);

    owner_.comboFlow_ = ::CreateWindowEx(
        0,
        WC_COMBOBOXW,
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        0, 0, 0, 200,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_FLOW)),
        owner_.instance_,
        nullptr);

    owner_.checkRts_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        L"RTS",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHK_RTS)),
        owner_.instance_,
        nullptr);

    owner_.checkDtr_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        L"DTR",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHK_DTR)),
        owner_.instance_,
        nullptr);

    // === Statistics элементы ===
    // owner_.textTxTotal_ = ::CreateWindowEx(
    //     0,
    //     WC_STATICW,
    //     L"TX: 0 bytes",
    //     WS_CHILD | WS_VISIBLE | SS_LEFT,
    //     0, 0, 0, 0,
    //     owner_.window_,
    //     reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TX_TOTAL)),
    //     owner_.instance_,
    //     nullptr);

    // owner_.textRxTotal_ = ::CreateWindowEx(
    //     0,
    //     WC_STATICW,
    //     L"RX: 0 bytes",
    //     WS_CHILD | WS_VISIBLE | SS_LEFT,
    //     0, 0, 0, 0,
    //     owner_.window_,
    //     reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_RX_TOTAL)),
    //     owner_.instance_,
    //     nullptr);

    // owner_.textTxRate_ = ::CreateWindowEx(
    //     0,
    //     WC_STATICW,
    //     L"TX Rate: 0 bps",
    //     WS_CHILD | WS_VISIBLE | SS_LEFT,
    //     0, 0, 0, 0,
    //     owner_.window_,
    //     reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TX_RATE)),
    //     owner_.instance_,
    //     nullptr);

    // owner_.textRxRate_ = ::CreateWindowEx(
    //     0,
    //     WC_STATICW,
    //     L"RX Rate: 0 bps",
    //     WS_CHILD | WS_VISIBLE | SS_LEFT,
    //     0, 0, 0, 0,
    //     owner_.window_,
    //     reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_RX_RATE)),
    //     owner_.instance_,
    //     nullptr);

    // === Terminal Control элементы ===
    owner_.ledStatus_ = ::CreateWindowEx(
        0,
        WC_STATICW,
        L"Disconnected",  // Символ круга
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LED_STATUS)),
        owner_.instance_,
        nullptr);

    owner_.comboRxMode_ = ::CreateWindowEx(
        0,
        WC_COMBOBOXW,
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        0, 0, 0, 200,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_RXMODE)),
        owner_.instance_,
        nullptr);

    owner_.checkSaveLog_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        L"Save log",  // Символ дискеты
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHK_SAVELOG)),
        owner_.instance_,
        nullptr);

    owner_.buttonClear_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        L"Clear",  // Символ корзины
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_CLEAR)),
        owner_.instance_,
        nullptr);

    // === Основные элементы ===
    owner_.richLog_ = ::CreateWindowEx(
        WS_EX_CLIENTEDGE,
        MSFTEDIT_CLASS,
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_RICH_LOG)),
        owner_.instance_,
        nullptr);

    owner_.editSend_ = ::CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WC_EDITW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_SEND)),
        owner_.instance_,
        nullptr);

    owner_.buttonSend_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        L"Send",  // Символ отправки
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_SEND)),
        owner_.instance_,
        nullptr);

    // ApplyThemes();
    // ============ Установка шрифта для лога ============
    CHARFORMAT2W format{};
    format.cbSize = sizeof(format);
    format.dwMask = CFM_FACE | CFM_SIZE | CFM_CHARSET;
    format.yHeight = 180;
    format.bCharSet = DEFAULT_CHARSET;
    ::StringCchCopyW(format.szFaceName, LF_FACESIZE, L"Consolas");
    ::SendMessage(owner_.richLog_, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&format));

    // ============ СОЗДАЕМ ПОДСКАЗКИ ============
    CreateTooltips();
}





void WindowBuilder::FillConnectionDefaults() {
    constexpr const wchar_t* dataBits[] = {L"5", L"6", L"7", L"8"};
    for (const auto* v : dataBits) {
        ::SendMessage(owner_.comboDataBits_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(v));
    }
    ::SendMessage(owner_.comboDataBits_, CB_SETCURSEL, 3, 0);

    constexpr const wchar_t* parity[] = {L"None", L"Odd", L"Even", L"Mark", L"Space"};
    for (const auto* v : parity) {
        ::SendMessage(owner_.comboParity_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(v));
    }
    ::SendMessage(owner_.comboParity_, CB_SETCURSEL, 0, 0);

    constexpr const wchar_t* stopBits[] = {L"1", L"1.5", L"2"};
    for (const auto* v : stopBits) {
        ::SendMessage(owner_.comboStopBits_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(v));
    }
    ::SendMessage(owner_.comboStopBits_, CB_SETCURSEL, 0, 0);

    constexpr const wchar_t* flow[] = {L"None", L"RTS/CTS", L"XON/XOFF"};
    for (const auto* v : flow) {
        ::SendMessage(owner_.comboFlow_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(v));
    }
    ::SendMessage(owner_.comboFlow_, CB_SETCURSEL, 0, 0);

    constexpr const wchar_t* rxMode[] = {L"Text", L"HEX"};
    for (const auto* v : rxMode) {
        ::SendMessage(owner_.comboRxMode_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(v));
    }
    ::SendMessage(owner_.comboRxMode_, CB_SETCURSEL, 1, 0); // HEX по умолчанию

    ::SendMessage(owner_.checkRts_, BM_SETCHECK, BST_UNCHECKED, 0);
    ::SendMessage(owner_.checkDtr_, BM_SETCHECK, BST_UNCHECKED, 0);
    ::SendMessage(owner_.checkSaveLog_, BM_SETCHECK, BST_UNCHECKED, 0);
}

void WindowBuilder::ApplyThemes() {
    // Применяем тему ко всем контролам
    HMODULE hUxTheme = ::LoadLibrary(L"uxtheme.dll");
    if (hUxTheme) {
        typedef HRESULT (WINAPI *SetWindowThemeFn)(HWND, LPCWSTR, LPCWSTR);
        auto pSetWindowTheme = (SetWindowThemeFn)::GetProcAddress(hUxTheme, "SetWindowTheme");
        
        if (pSetWindowTheme) {
            // Применяем Explorer тему ко всем контролам
            struct {
                HWND* hwnd;
                const wchar_t* name;
            } controls[] = {
                {&owner_.groupPort_, L"Explorer"},
                {&owner_.groupStats_, L"Explorer"},
                {&owner_.groupLog_, L"Explorer"},
                {&owner_.groupTerminalCtrl_, L"Explorer"},
                {&owner_.groupSend_, L"Explorer"},
                {&owner_.comboPort_, L"Explorer"},
                {&owner_.comboBaud_, L"Explorer"},
                {&owner_.buttonRefresh_, L"Explorer"},
                {&owner_.buttonOpen_, L"Explorer"},
                {&owner_.buttonClose_, L"Explorer"},
                {&owner_.comboDataBits_, L"Explorer"},
                {&owner_.comboParity_, L"Explorer"},
                {&owner_.comboStopBits_, L"Explorer"},
                {&owner_.comboFlow_, L"Explorer"},
                {&owner_.checkRts_, L"Explorer"},
                {&owner_.checkDtr_, L"Explorer"},
                {&owner_.textTxTotal_, L"Explorer"},
                {&owner_.textRxTotal_, L"Explorer"},
                {&owner_.textTxRate_, L"Explorer"},
                {&owner_.textRxRate_, L"Explorer"},
                {&owner_.ledStatus_, L"Explorer"},
                {&owner_.comboRxMode_, L"Explorer"},
                {&owner_.checkSaveLog_, L"Explorer"},
                {&owner_.buttonClear_, L"Explorer"},
                {&owner_.editSend_, L"Explorer"},
                {&owner_.buttonSend_, L"Explorer"},
            };
            
            for (const auto& ctrl : controls) {
                if (*ctrl.hwnd) {
                    pSetWindowTheme(*ctrl.hwnd, ctrl.name, nullptr);
                }
            }
            
            // RichEdit - особая тема
            if (owner_.richLog_) {
                pSetWindowTheme(owner_.richLog_, L"Explorer", nullptr);
            }
        }
        
        ::FreeLibrary(hUxTheme);
    }
}

} // namespace ui