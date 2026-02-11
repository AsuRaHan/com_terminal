#include "ui/MainWindow.h"

#include <windowsx.h>
#include <commctrl.h>
#include <richedit.h>
#include <strsafe.h>

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <sstream>

#include "resource.h"
#include "serial/PortScanner.h"

namespace {

constexpr UINT WM_APP_SERIAL_DATA = WM_APP + 1;

constexpr GUID kGuidDevinterfaceComport = {
    0x86E0D1E0, 0x8089, 0x11D0, {0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73}
};

struct SerialMessage {
    std::vector<uint8_t> data;
};

RECT CenteredRect(int width, int height) {
    RECT workArea{};
    ::SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);

    const int x = workArea.left + ((workArea.right - workArea.left) - width) / 2;
    const int y = workArea.top + ((workArea.bottom - workArea.top) - height) / 2;

    RECT rect{x, y, x + width, y + height};
    return rect;
}

std::wstring ComboText(HWND combo) {
    const int len = ::GetWindowTextLengthW(combo);
    if (len <= 0) {
        return L"";
    }

    std::wstring text;
    text.resize(static_cast<std::size_t>(len));
    ::GetWindowTextW(combo, text.data(), len + 1);
    return text;
}

} // namespace

namespace ui {

MainWindow::MainWindow(HINSTANCE instance)
    : instance_(instance),
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
      ledStatus_(nullptr),
      comboDataBits_(nullptr),
      comboParity_(nullptr),
      comboStopBits_(nullptr),
      comboFlow_(nullptr),
      checkRts_(nullptr),
      checkDtr_(nullptr),
      comboRxMode_(nullptr),
      checkSaveLog_(nullptr),
      ledBrushDisconnected_(::CreateSolidBrush(RGB(200, 50, 50))),
      ledBrushConnected_(::CreateSolidBrush(RGB(50, 160, 70))),
      deviceNotify_(nullptr),
      logVirtualizer_(2000, 5000, 5U * 1024U * 1024U),
      rebuildingRichEdit_(false),
      txBytes_(0),
      rxBytes_(0) {
}

MainWindow::~MainWindow() {
    ClosePort();
    if (deviceNotify_ != nullptr) {
        ::UnregisterDeviceNotification(deviceNotify_);
        deviceNotify_ = nullptr;
    }
    if (ledBrushDisconnected_ != nullptr) {
        ::DeleteObject(ledBrushDisconnected_);
        ledBrushDisconnected_ = nullptr;
    }
    if (ledBrushConnected_ != nullptr) {
        ::DeleteObject(ledBrushConnected_);
        ledBrushConnected_ = nullptr;
    }
}

bool MainWindow::Create(int nCmdShow) {
    if (!RegisterClass()) {
        return false;
    }

    constexpr int kWidth = 900;
    constexpr int kHeight = 600;
    RECT rect = CenteredRect(kWidth, kHeight);

    window_ = ::CreateWindowExW(
        0,
        L"COMTerminalMainWindow",
        L"COM Terminal",
        WS_OVERLAPPEDWINDOW,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        ::LoadMenuW(instance_, MAKEINTRESOURCEW(IDR_MAIN_MENU)),
        instance_,
        this);

    if (window_ == nullptr) {
        return false;
    }

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
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = reinterpret_cast<HICON>(::LoadImageW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"COMTerminalMainWindow";
    wc.hIconSm = reinterpret_cast<HICON>(::LoadImageW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));

    return ::RegisterClassExW(&wc) != 0;
}

void MainWindow::CreateStatusBar() {
    statusBar_ = ::CreateWindowExW(
        0,
        STATUSCLASSNAMEW,
        nullptr,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATUS_BAR)),
        instance_,
        nullptr);

    int parts[3] = {260, 520, -1};
    ::SendMessageW(statusBar_, SB_SETPARTS, 3, reinterpret_cast<LPARAM>(parts));
    ::SendMessageW(statusBar_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(L"Disconnected"));
    ::SendMessageW(statusBar_, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(L"TX: 0  RX: 0"));
    ::SendMessageW(statusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"Ready"));
}

void MainWindow::CreateControls() {
    // ComboBox для выбора COM-порта
    comboPort_ = ::CreateWindowExW(
        0,
        WC_COMBOBOXW,
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        0,
        0,
        0,
        200,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_PORT)),
        instance_,
        nullptr);

    // ComboBox для выбора скорости передачи (baud rate)
    comboBaud_ = ::CreateWindowExW(
        0,
        WC_COMBOBOXW,
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_TABSTOP,
        0,
        0,
        0,
        200,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_BAUD)),
        instance_,
        nullptr);

    constexpr const wchar_t* baudRates[] = {L"9600", L"19200", L"38400", L"57600", L"115200", L"230400", L"460800", L"921600"};
    for (const auto* rate : baudRates) {
        ::SendMessageW(comboBaud_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(rate));
    }
    ::SetWindowTextW(comboBaud_, L"115200");

    // Кнопка обновления списка доступных COM-портов
    buttonRefresh_ = ::CreateWindowExW(
        0,
        WC_BUTTONW,
        L"Refresh",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_REFRESH)),
        instance_,
        nullptr);

    // Кнопка открытия выбранного порта
    buttonOpen_ = ::CreateWindowExW(
        0,
        WC_BUTTONW,
        L"Open",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_OPEN)),
        instance_,
        nullptr);

    // Кнопка закрытия открытого порта
    buttonClose_ = ::CreateWindowExW(
        0,
        WC_BUTTONW,
        L"Close",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_CLOSE)),
        instance_,
        nullptr);

    // ComboBox для выбора количества данных битов (data bits)
    comboDataBits_ = ::CreateWindowExW(
        0,
        WC_COMBOBOXW,
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        0,
        0,
        0,
        200,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_DATABITS)),
        instance_,
        nullptr);

    // ComboBox для выбора режима паритета
    comboParity_ = ::CreateWindowExW(
        0,
        WC_COMBOBOXW,
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        0,
        0,
        0,
        200,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_PARITY)),
        instance_,
        nullptr);

    // ComboBox для выбора количества стоповых битов
    comboStopBits_ = ::CreateWindowExW(
        0,
        WC_COMBOBOXW,
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        0,
        0,
        0,
        200,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_STOPBITS)),
        instance_,
        nullptr);

    // ComboBox для выбора контроля потока (flow control)
    comboFlow_ = ::CreateWindowExW(
        0,
        WC_COMBOBOXW,
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        0,
        0,
        0,
        200,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_FLOW)),
        instance_,
        nullptr);

    // Чекбокс включения RTS
    checkRts_ = ::CreateWindowExW(
        0,
        WC_BUTTONW,
        L"RTS",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHK_RTS)),
        instance_,
        nullptr);

    // Чекбокс включения DTR
    checkDtr_ = ::CreateWindowExW(
        0,
        WC_BUTTONW,
        L"DTR",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHK_DTR)),
        instance_,
        nullptr);

    // ComboBox для выбора режима отображения входящих данных (текст/HEX)
    comboRxMode_ = ::CreateWindowExW(
        0,
        WC_COMBOBOXW,
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
        0,
        0,
        0,
        200,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_RXMODE)),
        instance_,
        nullptr);

    // Чекбокс сохранения логов
    checkSaveLog_ = ::CreateWindowExW(
        0,
        WC_BUTTONW,
        L"Save log",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHK_SAVELOG)),
        instance_,
        nullptr);

    ledStatus_ = ::CreateWindowExW(
        0,
        WC_STATICW,
        L"Disconnected",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LED_STATUS)),
        instance_,
        nullptr);

    richLog_ = ::CreateWindowExW(
        WS_EX_CLIENTEDGE,
        MSFTEDIT_CLASS,
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_RICH_LOG)),
        instance_,
        nullptr);

    editSend_ = ::CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WC_EDITW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | WS_TABSTOP,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_SEND)),
        instance_,
        nullptr);

    buttonSend_ = ::CreateWindowExW(
        0,
        WC_BUTTONW,
        L"Send",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_SEND)),
        instance_,
        nullptr);

    CHARFORMAT2W format{};
    format.cbSize = sizeof(format);
    format.dwMask = CFM_FACE | CFM_SIZE | CFM_CHARSET;
    format.yHeight = 180;
    format.bCharSet = DEFAULT_CHARSET;
    ::StringCchCopyW(format.szFaceName, LF_FACESIZE, L"Consolas");
    ::SendMessageW(richLog_, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&format));

    FillConnectionDefaults();
    RefreshPorts();
}

void MainWindow::ResizeChildren() {
    RECT client{};
    ::GetClientRect(window_, &client);

    if (statusBar_ != nullptr) {
        ::SendMessageW(statusBar_, WM_SIZE, 0, 0);
    }

    RECT statusRect{};
    int statusHeight = 0;
    if (statusBar_ != nullptr && ::GetWindowRect(statusBar_, &statusRect)) {
        statusHeight = statusRect.bottom - statusRect.top;
    }

    const int left = 8;
    const int top = 8;
    const int right = client.right - 8;

    int y = top;
    const int rowHeight = 28;
    const int comboDropHeight = 240;
    const int gap = 8;

    ::MoveWindow(comboPort_, left, y, 250, comboDropHeight, TRUE);
    ::MoveWindow(comboBaud_, left + 260, y, 120, comboDropHeight, TRUE);
    ::MoveWindow(buttonRefresh_, left + 390, y, 90, rowHeight, TRUE);
    ::MoveWindow(buttonOpen_, left + 490, y, 80, rowHeight, TRUE);
    ::MoveWindow(buttonClose_, left + 580, y, 80, rowHeight, TRUE);
    ::MoveWindow(ledStatus_, right - 160, y, 152, rowHeight, TRUE);

    y += rowHeight + gap;

    ::MoveWindow(comboDataBits_, left, y, 120, comboDropHeight, TRUE);
    ::MoveWindow(comboParity_, left + 130, y, 130, comboDropHeight, TRUE);
    ::MoveWindow(comboStopBits_, left + 270, y, 120, comboDropHeight, TRUE);
    ::MoveWindow(comboFlow_, left + 400, y, 140, comboDropHeight, TRUE);
    ::MoveWindow(checkRts_, left + 545, y + 3, 60, rowHeight, TRUE);
    ::MoveWindow(checkDtr_, left + 610, y + 3, 60, rowHeight, TRUE);
    ::MoveWindow(comboRxMode_, right - 190, y, 90, comboDropHeight, TRUE);
    ::MoveWindow(checkSaveLog_, right - 95, y + 3, 90, rowHeight, TRUE);

    y += rowHeight + gap;

    const int sendHeight = 90;
    const int logBottom = client.bottom - statusHeight - sendHeight - (gap * 2);
    ::MoveWindow(richLog_, left, y, right - left, std::max(100, logBottom - y), TRUE);

    const int sendTop = client.bottom - statusHeight - sendHeight - 8;
    ::MoveWindow(editSend_, left, sendTop, right - left - 90, sendHeight, TRUE);
    ::MoveWindow(buttonSend_, right - 80, sendTop, 72, sendHeight, TRUE);
}

void MainWindow::RefreshPorts() {
    const std::wstring previous = ComboText(comboPort_);

    ::SendMessageW(comboPort_, CB_RESETCONTENT, 0, 0);
    const auto ports = serial::PortScanner::Scan();

    int selectedIndex = -1;
    for (std::size_t i = 0; i < ports.size(); ++i) {
        const std::wstring text = ports[i].portName + L" - " + ports[i].friendlyName;
        const LRESULT idx = ::SendMessageW(comboPort_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
        if (idx >= 0 && previous == text) {
            selectedIndex = static_cast<int>(idx);
        }
    }

    if (selectedIndex < 0 && !ports.empty()) {
        selectedIndex = 0;
    }

    if (selectedIndex >= 0) {
        ::SendMessageW(comboPort_, CB_SETCURSEL, static_cast<WPARAM>(selectedIndex), 0);
        const std::wstring msg = L"Port list refreshed: " + std::to_wstring(ports.size()) + L" found";
        AppendLog(LogKind::System, msg);
    } else {
        AppendLog(LogKind::System, L"Port list refreshed: no ports found");
    }
}

bool MainWindow::OpenSelectedPort() {
    if (serialPort_.IsOpen()) {
        return true;
    }

    const int selected = static_cast<int>(::SendMessageW(comboPort_, CB_GETCURSEL, 0, 0));
    if (selected < 0) {
        AppendLog(LogKind::Error, L"No COM port selected");
        return false;
    }

    wchar_t portText[256] = {};
    ::SendMessageW(comboPort_, CB_GETLBTEXT, static_cast<WPARAM>(selected), reinterpret_cast<LPARAM>(portText));
    std::wstring selection = portText;
    const std::size_t spacePos = selection.find(L' ');
    const std::wstring portName = (spacePos == std::wstring::npos) ? selection : selection.substr(0, spacePos);

    bool settingsOk = false;
    const serial::PortSettings settings = BuildPortSettingsFromUi(&settingsOk);
    if (!settingsOk) {
        AppendLog(LogKind::Error, L"Invalid serial settings");
        return false;
    }

    serialPort_.SetDataCallback([this](const std::vector<uint8_t>& packet) {
        auto* message = new SerialMessage{packet};
        if (!::PostMessageW(window_, WM_APP_SERIAL_DATA, 0, reinterpret_cast<LPARAM>(message))) {
            delete message;
        }
    });

    if (!serialPort_.Open(portName, settings)) {
        AppendLog(LogKind::Error, L"Failed to open " + portName);
        return false;
    }

    txBytes_ = 0;
    rxBytes_ = 0;
    UpdateStatusText();

    ::SetWindowTextW(ledStatus_, L"Connected");
    ::SendMessageW(statusBar_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(L"Connected"));
    AppendLog(LogKind::System, L"Opened " + portName + L" @ " + std::to_wstring(settings.baudRate));
    return true;
}

void MainWindow::ClosePort() {
    serialPort_.SetDataCallback({});
    if (serialPort_.IsOpen()) {
        serialPort_.Close();
        ::SetWindowTextW(ledStatus_, L"Disconnected");
        ::SendMessageW(statusBar_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(L"Disconnected"));
        AppendLog(LogKind::System, L"Port closed");
    }
}

void MainWindow::SendInputData() {
    if (!serialPort_.IsOpen()) {
        AppendLog(LogKind::Error, L"Port is not open");
        return;
    }

    const int length = ::GetWindowTextLengthW(editSend_);
    if (length <= 0) {
        return;
    }

    std::wstring text;
    text.resize(static_cast<std::size_t>(length));
    ::GetWindowTextW(editSend_, text.data(), length + 1);

    const int utf8Len = ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Len <= 1) {
        return;
    }

    std::vector<uint8_t> bytes(static_cast<std::size_t>(utf8Len - 1));
    ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, reinterpret_cast<LPSTR>(bytes.data()), utf8Len - 1, nullptr, nullptr);

    DWORD written = 0;
    if (serialPort_.Write(bytes.data(), static_cast<DWORD>(bytes.size()), &written)) {
        txBytes_ += written;
        UpdateStatusText();
        AppendLog(LogKind::Tx, L"TX " + BytesToHex(bytes));
    } else {
        AppendLog(LogKind::Error, L"Write failed");
    }
}

void MainWindow::AppendLog(LogKind kind, const std::wstring& text) {
    if (richLog_ == nullptr) {
        return;
    }

    const COLORREF color = ColorForLogKind(kind);
    const std::wstring line = L"[" + BuildTimestamp() + L"] " + text + L"\r\n";
    const bool saveToDisk = (::SendMessageW(checkSaveLog_, BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (saveToDisk && logVirtualizer_.SessionFilePath().empty()) {
        logVirtualizer_.Initialize(L"logs");
    }
    logVirtualizer_.AppendLine(line, color, saveToDisk);
    AppendLineToRichEdit(line, color, true);

    if (!rebuildingRichEdit_ && logVirtualizer_.ShouldRewrite()) {
        RebuildRichEditFromVirtualBuffer();
    }
}

void MainWindow::FillConnectionDefaults() {
    constexpr const wchar_t* dataBits[] = {L"5", L"6", L"7", L"8"};
    for (const auto* v : dataBits) {
        ::SendMessageW(comboDataBits_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(v));
    }
    ::SendMessageW(comboDataBits_, CB_SETCURSEL, 3, 0);

    constexpr const wchar_t* parity[] = {L"None", L"Odd", L"Even", L"Mark", L"Space"};
    for (const auto* v : parity) {
        ::SendMessageW(comboParity_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(v));
    }
    ::SendMessageW(comboParity_, CB_SETCURSEL, 0, 0);

    constexpr const wchar_t* stopBits[] = {L"1", L"1.5", L"2"};
    for (const auto* v : stopBits) {
        ::SendMessageW(comboStopBits_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(v));
    }
    ::SendMessageW(comboStopBits_, CB_SETCURSEL, 0, 0);

    constexpr const wchar_t* flow[] = {L"None", L"RTS/CTS", L"XON/XOFF"};
    for (const auto* v : flow) {
        ::SendMessageW(comboFlow_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(v));
    }
    ::SendMessageW(comboFlow_, CB_SETCURSEL, 0, 0);

    constexpr const wchar_t* rxMode[] = {L"Text", L"HEX"};
    for (const auto* v : rxMode) {
        ::SendMessageW(comboRxMode_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(v));
    }
    ::SendMessageW(comboRxMode_, CB_SETCURSEL, 1, 0);

    ::SendMessageW(checkRts_, BM_SETCHECK, BST_UNCHECKED, 0);
    ::SendMessageW(checkDtr_, BM_SETCHECK, BST_UNCHECKED, 0);
    ::SendMessageW(checkSaveLog_, BM_SETCHECK, BST_UNCHECKED, 0);
}

// Создаёт структуру настроек порта, основанную на выбранных пользователем параметрах UI
serial::PortSettings MainWindow::BuildPortSettingsFromUi(bool* ok) const {
    serial::PortSettings s{};
    if (ok != nullptr) {
        *ok = false;
    }

    // Считываем выбранную скорость передачи
    const std::wstring baudText = ComboText(comboBaud_);
    const unsigned long baud = std::wcstoul(baudText.c_str(), nullptr, 10);
    if (baud == 0UL || baud > std::numeric_limits<DWORD>::max()) {
        return s;
    }
    s.baudRate = static_cast<DWORD>(baud);

    // Количество данных битов
    const int bitsSel = static_cast<int>(::SendMessageW(comboDataBits_, CB_GETCURSEL, 0, 0));
    s.dataBits = static_cast<BYTE>((bitsSel >= 0) ? (bitsSel + 5) : 8);

    // Режим паритета
    const int paritySel = static_cast<int>(::SendMessageW(comboParity_, CB_GETCURSEL, 0, 0));
    s.parity = (paritySel == 1) ? serial::ParityMode::Odd :
        (paritySel == 2) ? serial::ParityMode::Even :
        (paritySel == 3) ? serial::ParityMode::Mark :
        (paritySel == 4) ? serial::ParityMode::Space :
                           serial::ParityMode::None;

    // Количество стоповых битов
    const int stopSel = static_cast<int>(::SendMessageW(comboStopBits_, CB_GETCURSEL, 0, 0));
    s.stopBits = (stopSel == 1) ? serial::StopBitsMode::OnePointFive :
        (stopSel == 2) ? serial::StopBitsMode::Two :
                         serial::StopBitsMode::One;

    // Контроль потока
    const int flowSel = static_cast<int>(::SendMessageW(comboFlow_, CB_GETCURSEL, 0, 0));
    s.flowControl = (flowSel == 1) ? serial::FlowControlMode::Hardware :
        (flowSel == 2) ? serial::FlowControlMode::Software :
                         serial::FlowControlMode::None;

    // Состояние RTS и DTR
    s.rts = (::SendMessageW(checkRts_, BM_GETCHECK, 0, 0) == BST_CHECKED);
    s.dtr = (::SendMessageW(checkDtr_, BM_GETCHECK, 0, 0) == BST_CHECKED);

    if (ok != nullptr) {
        *ok = true;
    }
    return s;
}

std::wstring MainWindow::FormatIncoming(const std::vector<uint8_t>& bytes) const {
    const int mode = static_cast<int>(::SendMessageW(comboRxMode_, CB_GETCURSEL, 0, 0));
    if (mode == 0) {
        if (bytes.empty()) {
            return L"";
        }
        const int utf16Len = ::MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<int>(bytes.size()),
            nullptr,
            0);
        if (utf16Len > 0) {
            std::wstring text(static_cast<std::size_t>(utf16Len), L'\0');
            ::MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                reinterpret_cast<const char*>(bytes.data()),
                static_cast<int>(bytes.size()),
                text.data(),
                utf16Len);
            return text;
        }

        const int acpLen = ::MultiByteToWideChar(
            CP_ACP,
            0,
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<int>(bytes.size()),
            nullptr,
            0);
        if (acpLen > 0) {
            std::wstring text(static_cast<std::size_t>(acpLen), L'\0');
            ::MultiByteToWideChar(
                CP_ACP,
                0,
                reinterpret_cast<const char*>(bytes.data()),
                static_cast<int>(bytes.size()),
                text.data(),
                acpLen);
            return text;
        }
    }

    return BytesToHex(bytes);
}

void MainWindow::AppendLineToRichEdit(const std::wstring& line, COLORREF color, bool scrollToCaret) {
    const int end = ::GetWindowTextLengthW(richLog_);
    ::SendMessageW(richLog_, EM_SETSEL, static_cast<WPARAM>(end), static_cast<LPARAM>(end));

    CHARFORMAT2W format{};
    format.cbSize = sizeof(format);
    format.dwMask = CFM_COLOR;
    format.crTextColor = color;
    ::SendMessageW(richLog_, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));

    ::SendMessageW(richLog_, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(line.c_str()));
    if (scrollToCaret) {
        ::SendMessageW(richLog_, EM_SCROLLCARET, 0, 0);
    }
}

void MainWindow::RebuildRichEditFromVirtualBuffer() {
    if (richLog_ == nullptr) {
        return;
    }

    rebuildingRichEdit_ = true;

    POINT scrollPos{};
    ::SendMessageW(richLog_, EM_GETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&scrollPos));

    CHARRANGE selection{};
    ::SendMessageW(richLog_, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&selection));

    ::SendMessageW(richLog_, WM_SETREDRAW, FALSE, 0);

    SETTEXTEX setText{};
    setText.flags = ST_DEFAULT;
    setText.codepage = 1200;
    ::SendMessageW(richLog_, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&setText), reinterpret_cast<LPARAM>(L""));

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
    ::SendMessageW(richLog_, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&selection));
    ::SendMessageW(richLog_, EM_SETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&scrollPos));

    ::SendMessageW(richLog_, WM_SETREDRAW, TRUE, 0);
    ::RedrawWindow(richLog_, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);

    logVirtualizer_.MarkRewriteDone();
    rebuildingRichEdit_ = false;
}

void MainWindow::UpdateStatusText() {
    wchar_t buffer[128] = {};
    ::StringCchPrintfW(buffer, _countof(buffer), L"TX: %llu  RX: %llu", txBytes_, rxBytes_);
    ::SendMessageW(statusBar_, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(buffer));
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
        CreateStatusBar();
        CreateControls();

        DEV_BROADCAST_DEVICEINTERFACE_W filter{};
        filter.dbcc_size = sizeof(filter);
        filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        filter.dbcc_classguid = kGuidDevinterfaceComport;
        deviceNotify_ = ::RegisterDeviceNotificationW(hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);

        ResizeChildren();
        AppendLog(LogKind::System, L"Application started");
        return 0;
    }

    case WM_SIZE:
        ResizeChildren();
        return 0;

    case WM_GETMINMAXINFO: {
        auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        mmi->ptMinTrackSize.x = 800;
        mmi->ptMinTrackSize.y = 600;
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_FILE_EXIT:
            ::DestroyWindow(hwnd);
            return 0;
        case IDM_PORT_OPEN:
        case IDC_BTN_OPEN:
            OpenSelectedPort();
            return 0;
        case IDM_PORT_CLOSE:
        case IDC_BTN_CLOSE:
            ClosePort();
            return 0;
        case IDM_PORT_REFRESH:
        case IDC_BTN_REFRESH:
            RefreshPorts();
            return 0;
        case IDC_BTN_SEND:
            SendInputData();
            return 0;
        default:
            break;
        }
        break;

    case WM_CTLCOLORSTATIC: {
        HWND control = reinterpret_cast<HWND>(lParam);
        if (control == ledStatus_) {
            const bool connected = serialPort_.IsOpen();
            HDC dc = reinterpret_cast<HDC>(wParam);
            ::SetBkColor(dc, connected ? RGB(50, 160, 70) : RGB(200, 50, 50));
            ::SetTextColor(dc, RGB(255, 255, 255));
            return reinterpret_cast<LRESULT>(connected ? ledBrushConnected_ : ledBrushDisconnected_);
        }
        break;
    }

    case WM_DEVICECHANGE:
        if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
            RefreshPorts();
        }
        return 0;

    case WM_APP_SERIAL_DATA: {
        auto* message = reinterpret_cast<SerialMessage*>(lParam);
        if (message != nullptr) {
            rxBytes_ += static_cast<std::uint64_t>(message->data.size());
            UpdateStatusText();
            AppendLog(LogKind::Rx, L"RX " + FormatIncoming(message->data));
            delete message;
        }
        return 0;
    }

    case WM_DESTROY:
        ClosePort();
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
