#include "ui/WindowBuilder.h"
#include <string>
#include <stddef.h>

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

    int parts[3] = {260, 520, -1}; // Три части: статус, статистика, подсказки
    ::SendMessage(owner_.statusBar_, SB_SETPARTS, 3, reinterpret_cast<LPARAM>(parts));
    // Load status bar strings from resources
    {
        wchar_t buf[256] = {0};
        int len = LoadStringW(owner_.instance_, IDS_STATUS_DISCONNECTED, buf, _countof(buf));
        std::wstring text = len > 0 ? buf : L"Disconnected";
        ::SendMessage(owner_.statusBar_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(text.c_str()));
    }
    {
        wchar_t buf[256] = {0};
        int len = LoadStringW(owner_.instance_, IDS_STATUS_TXRX, buf, _countof(buf));
        std::wstring text = len > 0 ? buf : L"TX: 0  RX: 0";
        ::SendMessage(owner_.statusBar_, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(text.c_str()));
    }
    {
        wchar_t buf[256] = {0};
        int len = LoadStringW(owner_.instance_, IDS_STATUS_READY, buf, _countof(buf));
        std::wstring text = len > 0 ? buf : L"Ready";
        ::SendMessage(owner_.statusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(text.c_str()));
    }
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

    // Формируем полный текст
    std::wstring fullText;
    if (!title.empty()) {
        fullText = title + L"\n" + text;
    } else {
        fullText = text;
    }

    TOOLINFOW ti{};
    ti.cbSize = sizeof(TOOLINFOW);
    ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
    ti.hwnd = owner_.window_;
    ti.uId = reinterpret_cast<UINT_PTR>(control);
    
    // ВАЖНО: храним строку где-то, чтобы не умерла
    static std::unordered_map<HWND, std::wstring> tooltipTexts;
    tooltipTexts[control] = fullText;
    ti.lpszText = const_cast<LPWSTR>(tooltipTexts[control].c_str());
    
    ::SendMessage(owner_.tooltip_, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
}

void WindowBuilder::AddAllTooltips() {
    // Helper lambda to load string from resources with fallback
    auto add = [&](HWND ctrl, int id, const std::wstring& title, const std::wstring& fallback) {
        wchar_t buf[256] = {0};
        int len = LoadStringW(owner_.instance_, id, buf, _countof(buf));
        std::wstring txt = len > 0 ? buf : fallback;
        AddTooltip(ctrl, txt, title);
    };

    // ============ Port Settings ============
    add(owner_.comboPort_, IDS_TIP_COMBO_PORT, L"COM Port",
        L"Select COM port. Click Refresh to update list.");

        add(owner_.comboBaud_, IDS_TIP_COMBO_BAUD, L"Baud Rate",
            L"Baud rate (9600, 19200, 38400, 57600, 115200, etc)");

        add(owner_.buttonRefresh_, IDS_TIP_BUTTON_REFRESH, L"Refresh",
            L"Scan for available COM ports");

        add(owner_.buttonOpen_, IDS_TIP_BUTTON_OPEN, L"Open Port",
            L"Open selected COM port");

        add(owner_.buttonClose_, IDS_TIP_BUTTON_CLOSE, L"Close Port",
            L"Close current COM port");

        add(owner_.comboDataBits_, IDS_TIP_COMBO_DATABITS, L"Data Bits",
            L"Data bits: 5, 6, 7, or 8");

        add(owner_.comboParity_, IDS_TIP_COMBO_PARITY, L"Parity",
            L"Parity: None, Odd, Even, Mark, Space");

        add(owner_.comboStopBits_, IDS_TIP_COMBO_STOPBITS, L"Stop Bits",
            L"Stop bits: 1, 1.5, or 2");

        add(owner_.comboFlow_, IDS_TIP_COMBO_FLOW, L"Flow Control",
            L"Flow control: None, RTS/CTS, XON/XOFF");

        add(owner_.checkRts_, IDS_TIP_CHECK_RTS, L"RTS",
            L"Request To Send signal");

        add(owner_.checkDtr_, IDS_TIP_CHECK_DTR, L"DTR",
            L"Data Terminal Ready signal");

    // ============ Statistics ============
    // AddTooltip(owner_.textTxTotal_,
    //     L"Total bytes transmitted",
    //     L"TX Total");

    // AddTooltip(owner_.textRxTotal_,
    //     L"Total bytes received",
    //     L"RX Total");

    // AddTooltip(owner_.textTxRate_,
    //     L"Current transmit speed",
    //     L"TX Rate");

    // AddTooltip(owner_.textRxRate_,
    //     L"Current receive speed",
    //     L"RX Rate");

    // ============ Terminal Log ============
    add(owner_.richLog_, IDS_TIP_GROUP_LOG, L"Terminal Log",
        L"Terminal output - Green:RX Blue:TX Gray:System Red:Error");

    // ============ Terminal Control ============
    add(owner_.ledStatus_, IDS_TIP_GROUP_TERMINAL_CTRL, L"Status",
        L"Connection status - Green:Open Red:Closed");

    add(owner_.comboRxMode_, IDS_TIP_COMBO_RXMODE, L"RX Mode",
        L"Display mode: Text (UTF-8) or HEX");

    add(owner_.checkSaveLog_, IDS_TIP_CHECK_SAVELOG, L"Save Log",
        L"Save log to file");

    add(owner_.buttonClear_, IDS_TIP_BUTTON_CLEAR, L"Clear",
        L"Clear terminal and reset counters");
    // ============ Send Data ============
    add(owner_.editSend_, IDS_TIP_EDIT_SEND, L"Send Buffer",
        L"Data to send - Text or HEX (space separated)");

    add(owner_.buttonSend_, IDS_TIP_BUTTON_SEND, L"Send",
        L"Send data");

    // ============ Groups ============
    add(owner_.groupPort_, IDS_TIP_GROUP_PORT, L"Port Settings",
        L"Serial port configuration");

    add(owner_.groupStats_, IDS_TIP_GROUP_STATS, L"Statistics",
        L"Communication statistics");

    add(owner_.groupLog_, IDS_TIP_GROUP_LOG, L"Log",
        L"Terminal communication log");

    add(owner_.groupTerminalCtrl_, IDS_TIP_GROUP_TERMINAL_CTRL, L"Terminal Control",
        L"Terminal display controls");

    add(owner_.groupSend_, IDS_TIP_GROUP_SEND, L"Send Data",
        L"Send data to device");
}

void WindowBuilder::CreateControls() {
    // ============ СНАЧАЛА СОЗДАЕМ ГРУППЫ ============
    owner_.groupPort_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        nullptr, // текст задаём после создания
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_GROUP_PORT)),
        owner_.instance_,
        nullptr);
    {
        wchar_t buf[256] = {0};
        int len = LoadStringW(owner_.instance_, IDS_GROUP_PORT_TEXT, buf, _countof(buf));
        ::SetWindowText(owner_.groupPort_, len > 0 ? buf : L"Port Settings");
    }

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
        nullptr,
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_GROUP_LOG)),
        owner_.instance_,
        nullptr);
    {
        wchar_t buf[256] = {0};
        int len = LoadStringW(owner_.instance_, IDS_GROUP_LOG_TEXT, buf, _countof(buf));
        ::SetWindowText(owner_.groupLog_, len > 0 ? buf : L"Terminal Log");
    }

    owner_.groupTerminalCtrl_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        nullptr,
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_GROUP_TERMINAL_CTRL)),
        owner_.instance_,
        nullptr);
    {
        wchar_t buf[256] = {0};
        int len = LoadStringW(owner_.instance_, IDS_GROUP_TERMINAL_CTRL_TEXT, buf, _countof(buf));
        ::SetWindowText(owner_.groupTerminalCtrl_, len > 0 ? buf : L"Terminal Control");
    }

    owner_.groupSend_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        nullptr,
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_GROUP_SEND)),
        owner_.instance_,
        nullptr);
    {
        wchar_t buf[256] = {0};
        int len = LoadStringW(owner_.instance_, IDS_GROUP_SEND_TEXT, buf, _countof(buf));
        ::SetWindowText(owner_.groupSend_, len > 0 ? buf : L"Send Data");
    }

    // ============ ТЕПЕРЬ СОЗДАЕМ ЭЛЕМЕНТЫ УПРАВЛЕНИЯ ============
    
    // === Port Settings элементы ===
    owner_.buttonRefresh_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_REFRESH)),
        owner_.instance_,
        nullptr);
    {
        wchar_t buf[256] = {0};
        int len = LoadStringW(owner_.instance_, IDS_BTN_REFRESH_TEXT, buf, _countof(buf));
        ::SetWindowText(owner_.buttonRefresh_, len > 0 ? buf : L"Refresh");
    }

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
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_OPEN)),
        owner_.instance_,
        nullptr);
    {
        wchar_t buf[256] = {0};
        int len = LoadStringW(owner_.instance_, IDS_BTN_OPEN_TEXT, buf, _countof(buf));
        ::SetWindowText(owner_.buttonOpen_, len > 0 ? buf : L"Open");
    }

    owner_.buttonClose_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_CLOSE)),
        owner_.instance_,
        nullptr);
    {
        wchar_t buf[256] = {0};
        int len = LoadStringW(owner_.instance_, IDS_BTN_CLOSE_TEXT, buf, _countof(buf));
        ::SetWindowText(owner_.buttonClose_, len > 0 ? buf : L"Close");
    }

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
        L"Save log",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHK_SAVELOG)),
        owner_.instance_,
        nullptr);

    owner_.buttonClear_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_CLEAR)),
        owner_.instance_,
        nullptr);
    {
        wchar_t buf[256] = {0};
        int len = LoadStringW(owner_.instance_, IDS_BTN_CLEAR_TEXT, buf, _countof(buf));
        ::SetWindowText(owner_.buttonClear_, len > 0 ? buf : L"Clear");
    }

    // === Основные элементы ===
    owner_.richLog_ = ::CreateWindowEx(
        0,
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
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_SEND)),
        owner_.instance_,
        nullptr);
    {
        wchar_t buf[256] = {0};
        int len = LoadStringW(owner_.instance_, IDS_BTN_SEND_TEXT, buf, _countof(buf));
        ::SetWindowText(owner_.buttonSend_, len > 0 ? buf : L"Send");
    }

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

} // namespace ui