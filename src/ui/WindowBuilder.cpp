#include "ui/WindowBuilder.h"



namespace ui {

WindowBuilder::WindowBuilder(MainWindow& owner) : owner_(owner) {}

void WindowBuilder::CreateStatusBar() {
    owner_.statusBar_ = ::CreateWindowEx(
        0,
        STATUSCLASSNAMEW,
        nullptr,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0,
        0,
        0,
        0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATUS_BAR)),
        owner_.instance_,
        nullptr);

    int parts[3] = {260, 520, -1};
    ::SendMessage(owner_.statusBar_, SB_SETPARTS, 3, reinterpret_cast<LPARAM>(parts));
    ::SendMessage(owner_.statusBar_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(L"Disconnected"));
    ::SendMessage(owner_.statusBar_, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(L"TX: 0  RX: 0"));
    ::SendMessage(owner_.statusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"Ready"));
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

    owner_.groupStats_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        L"Statistics",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_GROUP_STATS)),
        owner_.instance_,
        nullptr);

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
        L"Refresh",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,  // Родитель - главное окно!
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
        L"Open",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_OPEN)),
        owner_.instance_,
        nullptr);

    owner_.buttonClose_ = ::CreateWindowEx(
        0,
        WC_BUTTONW,
        L"Close",
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
    owner_.textTxTotal_ = ::CreateWindowEx(
        0,
        WC_STATICW,
        L"TX: 0 bytes",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TX_TOTAL)),
        owner_.instance_,
        nullptr);

    owner_.textRxTotal_ = ::CreateWindowEx(
        0,
        WC_STATICW,
        L"RX: 0 bytes",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_RX_TOTAL)),
        owner_.instance_,
        nullptr);

    owner_.textTxRate_ = ::CreateWindowEx(
        0,
        WC_STATICW,
        L"TX Rate: 0 bps",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TX_RATE)),
        owner_.instance_,
        nullptr);

    owner_.textRxRate_ = ::CreateWindowEx(
        0,
        WC_STATICW,
        L"RX Rate: 0 bps",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_RX_RATE)),
        owner_.instance_,
        nullptr);

    // === Terminal Control элементы ===
    owner_.ledStatus_ = ::CreateWindowEx(
        0,
        WC_STATICW,
        L"Disconnected",
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
        L"Clear",
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
        L"Send",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        owner_.window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_SEND)),
        owner_.instance_,
        nullptr);

    // ============ Установка шрифта для лога ============
    CHARFORMAT2W format{};
    format.cbSize = sizeof(format);
    format.dwMask = CFM_FACE | CFM_SIZE | CFM_CHARSET;
    format.yHeight = 180;
    format.bCharSet = DEFAULT_CHARSET;
    ::StringCchCopyW(format.szFaceName, LF_FACESIZE, L"Consolas");
    ::SendMessage(owner_.richLog_, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&format));
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
    ::SendMessage(owner_.comboRxMode_, CB_SETCURSEL, 1, 0);

    ::SendMessage(owner_.checkRts_, BM_SETCHECK, BST_UNCHECKED, 0);
    ::SendMessage(owner_.checkDtr_, BM_SETCHECK, BST_UNCHECKED, 0);
    ::SendMessage(owner_.checkSaveLog_, BM_SETCHECK, BST_UNCHECKED, 0);
}

} // namespace ui
