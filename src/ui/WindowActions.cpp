#include "ui/WindowActions.h"

namespace ui {

namespace {
constexpr UINT WM_APP_SERIAL_DATA = WM_APP + 1;
} // namespace

// Helper to load a string resource into std::wstring
inline std::wstring LoadStringFromRes(HINSTANCE hInst, UINT id) {
    wchar_t buf[256];
    int len = ::LoadString(hInst, id, buf, _countof(buf));
    return std::wstring(buf, static_cast<std::size_t>(len));
}

WindowActions::WindowActions(MainWindow& owner) : owner_(owner) {}

void WindowActions::RefreshPorts() {
    const std::wstring previous = ComboText(owner_.comboPort_);

    ::SendMessage(owner_.comboPort_, CB_RESETCONTENT, 0, 0);
    const auto ports = serial::PortScanner::Scan();

    int selectedIndex = -1;
    for (std::size_t i = 0; i < ports.size(); ++i) {
        const std::wstring text = ports[i].portName + L" - " + ports[i].friendlyName;
        const LRESULT idx = ::SendMessage(owner_.comboPort_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
        if (idx >= 0 && previous == text) {
            selectedIndex = static_cast<int>(idx);
        }
    }

    if (selectedIndex < 0 && !ports.empty()) {
        selectedIndex = 0;
    }

    if (selectedIndex >= 0) {
        ::SendMessage(owner_.comboPort_, CB_SETCURSEL, static_cast<WPARAM>(selectedIndex), 0);
        const std::wstring fmt = LoadStringFromRes(owner_.instance_, IDS_PORT_LIST_REFRESHED);
        wchar_t buffer[256];
        wsprintfW(buffer, fmt.c_str(), static_cast<int>(ports.size()));
        owner_.AppendLog(LogKind::System, std::wstring(buffer));
    } else {
        owner_.AppendLog(LogKind::System, LoadStringFromRes(owner_.instance_, IDS_NO_PORTS_FOUND));
    }
}

bool WindowActions::OpenSelectedPort() {
    if (owner_.serialPort_.IsOpen()) {
        return true;
    }

    const int selected = static_cast<int>(::SendMessage(owner_.comboPort_, CB_GETCURSEL, 0, 0));
    if (selected < 0) {
        owner_.AppendLog(LogKind::Error, LoadStringFromRes(owner_.instance_, IDS_NO_COM_PORT));
        return false;
    }

    wchar_t portText[256] = {};
    ::SendMessage(owner_.comboPort_, CB_GETLBTEXT, static_cast<WPARAM>(selected), reinterpret_cast<LPARAM>(portText));
    std::wstring selection = portText;
    const std::size_t spacePos = selection.find(L' ');
    const std::wstring portName = (spacePos == std::wstring::npos) ? selection : selection.substr(0, spacePos);

    bool settingsOk = false;
    const serial::PortSettings settings = BuildPortSettingsFromUi(&settingsOk);
    if (!settingsOk) {
        owner_.AppendLog(LogKind::Error, LoadStringFromRes(owner_.instance_, IDS_INVALID_SERIAL));
        return false;
    }

    owner_.serialPort_.SetDataCallback([this](const std::vector<uint8_t>& packet) {
        auto* message = new std::vector<uint8_t>(packet);
        if (!::PostMessageW(owner_.window_, WM_APP_SERIAL_DATA, 0, reinterpret_cast<LPARAM>(message))) {
            delete message;
        }
    });

    if (!owner_.serialPort_.Open(portName, settings)) {
        const std::wstring fmt = LoadStringFromRes(owner_.instance_, IDS_FAILED_OPEN);
        wchar_t buffer[256];
        wsprintfW(buffer, fmt.c_str(), portName.c_str());
        owner_.AppendLog(LogKind::Error, std::wstring(buffer));
        return false;
    }

    owner_.txBytes_ = 0;
    owner_.rxBytes_ = 0;
    owner_.UpdateStatusText();

    const std::wstring connectedStr = LoadStringFromRes(owner_.instance_, IDS_STATUS_CONNECTED);
    ::SetWindowText(owner_.ledStatus_, connectedStr.c_str());
    ::SendMessage(owner_.statusBar_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(connectedStr.c_str()));
    const std::wstring fmt = LoadStringFromRes(owner_.instance_, IDS_PORT_OPENED);
    wchar_t buffer[256];
    wsprintfW(buffer, fmt.c_str(), portName.c_str(), static_cast<int>(settings.baudRate));
    owner_.AppendLog(LogKind::System, std::wstring(buffer));
    // Update button visibility after successful connection
    owner_.UpdateConnectionButtons();
    return true;
}

void WindowActions::ClosePort() {
    owner_.serialPort_.SetDataCallback({});
    if (owner_.serialPort_.IsOpen()) {
        owner_.serialPort_.Close();
        const std::wstring disconnectedStr = LoadStringFromRes(owner_.instance_, IDS_STATUS_DISCONNECTED);
        ::SetWindowText(owner_.ledStatus_, disconnectedStr.c_str());
        ::SendMessage(owner_.statusBar_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(disconnectedStr.c_str()));
        owner_.AppendLog(LogKind::System, LoadStringFromRes(owner_.instance_, IDS_PORT_CLOSED));
        // Update button visibility after closing
        owner_.UpdateConnectionButtons();
    }
}

void WindowActions::SendInputData() {
    if (!owner_.serialPort_.IsOpen()) {
        owner_.AppendLog(LogKind::Error, LoadStringFromRes(owner_.instance_, IDS_PORT_NOT_OPEN));
        return;
    }

    const int length = ::GetWindowTextLength(owner_.editSend_);
    if (length <= 0) {
        return;
    }

    std::wstring text;
    text.resize(static_cast<std::size_t>(length));
    ::GetWindowText(owner_.editSend_, text.data(), length + 1);

    std::vector<uint8_t> bytes;
    
    const int mode = static_cast<int>(::SendMessage(owner_.comboRxMode_, CB_GETCURSEL, 0, 0));
    
    if (mode == 0) {
        // Текстовый режим - UTF-8
        int utf8Len = ::WideCharToMultiByte(
            CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
        
        if (utf8Len > 1) {
            bytes.resize(static_cast<std::size_t>(utf8Len - 1));
            ::WideCharToMultiByte(
                CP_UTF8, 0, text.c_str(), -1,
                reinterpret_cast<LPSTR>(bytes.data()), utf8Len - 1, nullptr, nullptr);
        }
    } else {        
        // ============ HEX режим - продвинутый парсинг ============
        
        // Разделяем строку по любым не-hex символам
        std::wstring current;
        for (wchar_t ch : text) {
            if (iswxdigit(ch)) {
                current += ch;
            } else {
                // Если накопили 2 символа - конвертируем
                if (current.length() >= 2) {
                    // Берем по 2 символа
                    for (size_t i = 0; i < current.length() / 2; i++) {
                        std::wstring byteStr = current.substr(i * 2, 2);
                        wchar_t* end;
                        long val = std::wcstol(byteStr.c_str(), &end, 16);
                        bytes.push_back(static_cast<uint8_t>(val));
                    }
                    // Если остался 1 символ - добавим 0 спереди
                    if (current.length() % 2 == 1) {
                        std::wstring byteStr = L"0" + current.substr(current.length() - 1);
                        wchar_t* end;
                        long val = std::wcstol(byteStr.c_str(), &end, 16);
                        bytes.push_back(static_cast<uint8_t>(val));
                    }
                }
                current.clear();
            }
        }
        
        // Обрабатываем остаток строки
        if (!current.empty()) {
            if (current.length() >= 2) {
                for (size_t i = 0; i < current.length() / 2; i++) {
                    std::wstring byteStr = current.substr(i * 2, 2);
                    wchar_t* end;
                    long val = std::wcstol(byteStr.c_str(), &end, 16);
                    bytes.push_back(static_cast<uint8_t>(val));
                }
                if (current.length() % 2 == 1) {
                    std::wstring byteStr = L"0" + current.substr(current.length() - 1);
                    wchar_t* end;
                    long val = std::wcstol(byteStr.c_str(), &end, 16);
                    bytes.push_back(static_cast<uint8_t>(val));
                }
            } else {
                // Один символ в конце
                std::wstring byteStr = L"0" + current;
                wchar_t* end;
                long val = std::wcstol(byteStr.c_str(), &end, 16);
                bytes.push_back(static_cast<uint8_t>(val));
            }
        }
    }

    if (bytes.empty()) {
        owner_.AppendLog(LogKind::Error, LoadStringFromRes(owner_.instance_, IDS_NO_DATA_TO_SEND));
        return;
    }

    DWORD written = 0;
    if (owner_.serialPort_.Write(bytes.data(), static_cast<DWORD>(bytes.size()), &written)) {
        owner_.txBytes_ += written;
        owner_.UpdateStatusText();
        
        if (mode == 0) {
            std::wstring displayText = text;
            if (displayText.length() > 100) {
                displayText = displayText.substr(0, 100) + L"...";
            }
            owner_.AppendLog(LogKind::Tx, L"TX: " + displayText);
        } else {
            owner_.AppendLog(LogKind::Tx, L"TX: " + MainWindow::BytesToHex(bytes));
        }
    } else {
        owner_.AppendLog(LogKind::Error, LoadStringFromRes(owner_.instance_, IDS_WRITE_FAILED));
    }
}

void WindowActions::HandleSerialData(const std::vector<uint8_t>& bytes) {
    owner_.rxBytes_ += static_cast<std::uint64_t>(bytes.size());
    owner_.UpdateStatusText();
    owner_.AppendLog(LogKind::Rx, L"RX: " + FormatIncoming(bytes));
}

std::wstring WindowActions::ComboText(HWND combo) {
    const int len = ::GetWindowTextLength(combo);
    if (len <= 0) {
        return L"";
    }

    std::wstring text;
    text.resize(static_cast<std::size_t>(len));
    ::GetWindowTextW(combo, text.data(), len + 1);
    return text;
}

serial::PortSettings WindowActions::BuildPortSettingsFromUi(bool* ok) const {
    serial::PortSettings s{};
    if (ok != nullptr) {
        *ok = false;
    }

    const std::wstring baudText = ComboText(owner_.comboBaud_);
    const unsigned long baud = std::wcstoul(baudText.c_str(), nullptr, 10);
    if (baud == 0UL || baud > std::numeric_limits<DWORD>::max()) {
        return s;
    }
    s.baudRate = static_cast<DWORD>(baud);

    const int bitsSel = static_cast<int>(::SendMessage(owner_.comboDataBits_, CB_GETCURSEL, 0, 0));
    s.dataBits = static_cast<BYTE>((bitsSel >= 0) ? (bitsSel + 5) : 8);

    const int paritySel = static_cast<int>(::SendMessage(owner_.comboParity_, CB_GETCURSEL, 0, 0));
    s.parity = (paritySel == 1) ? serial::ParityMode::Odd :
        (paritySel == 2) ? serial::ParityMode::Even :
        (paritySel == 3) ? serial::ParityMode::Mark :
        (paritySel == 4) ? serial::ParityMode::Space :
                           serial::ParityMode::None;

    const int stopSel = static_cast<int>(::SendMessage(owner_.comboStopBits_, CB_GETCURSEL, 0, 0));
    s.stopBits = (stopSel == 1) ? serial::StopBitsMode::OnePointFive :
        (stopSel == 2) ? serial::StopBitsMode::Two :
                         serial::StopBitsMode::One;

    const int flowSel = static_cast<int>(::SendMessage(owner_.comboFlow_, CB_GETCURSEL, 0, 0));
    s.flowControl = (flowSel == 1) ? serial::FlowControlMode::Hardware :
        (flowSel == 2) ? serial::FlowControlMode::Software :
                         serial::FlowControlMode::None;

    s.rts = (::SendMessage(owner_.checkRts_, BM_GETCHECK, 0, 0) == BST_CHECKED);
    s.dtr = (::SendMessage(owner_.checkDtr_, BM_GETCHECK, 0, 0) == BST_CHECKED);

    if (ok != nullptr) {
        *ok = true;
    }
    return s;
}

std::wstring WindowActions::FormatIncoming(const std::vector<uint8_t>& bytes) const {
    const int mode = static_cast<int>(::SendMessage(owner_.comboRxMode_, CB_GETCURSEL, 0, 0));
    
    if (mode == 0) {
        if (bytes.empty()) return L"";
        
        // Сначала пробуем UTF-8
        int utf16Len = ::MultiByteToWideChar(
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
            
            // Обработка управляющих символов
            std::wstring result;
            result.reserve(text.length());
            for (wchar_t ch : text) {
                if (ch == L'\r') {
                    continue;
                } else if (ch == L'\n') {
                    result += L'\n';
                } else if (ch == L'\t') {
                    result += L"    ";
                } else if (ch < 32) {
                    wchar_t buf[8] = {};
                    ::StringCchPrintfW(buf, 8, L"^%c", ch + 64);
                    result += buf;
                } else {
                    result += ch;
                }
            }
            return result;
        }
        
        // Пробуем системную кодовую страницу
        int acpLen = ::MultiByteToWideChar(
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
            
            std::wstring result;
            result.reserve(text.length());
            for (wchar_t ch : text) {
                if (ch == L'\r') {
                    continue;
                } else if (ch == L'\n') {
                    result += L'\n';
                } else if (ch == L'\t') {
                    result += L"    ";
                } else if (ch < 32) {
                    wchar_t buf[8] = {};
                    ::StringCchPrintfW(buf, 8, L"^%c", ch + 64);
                    result += buf;
                } else {
                    result += ch;
                }
            }
            return result;
        }
        
        // Если ничего не работает - HEX
        return L"[BIN] " + MainWindow::BytesToHex(bytes);
    }
    
    // HEX режим
    return MainWindow::BytesToHex(bytes);
}

} // namespace ui
