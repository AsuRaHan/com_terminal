#include "ui/WindowActions.h"

#include <windows.h>
#include <commctrl.h>

#include <cstdlib>
#include <limits>

#include "resource.h"
#include "serial/PortScanner.h"
#include "ui/MainWindow.h"

namespace ui {

namespace {
constexpr UINT WM_APP_SERIAL_DATA = WM_APP + 1;
} // namespace

WindowActions::WindowActions(MainWindow& owner) : owner_(owner) {}

void WindowActions::RefreshPorts() {
    const std::wstring previous = ComboText(owner_.comboPort_);

    ::SendMessageW(owner_.comboPort_, CB_RESETCONTENT, 0, 0);
    const auto ports = serial::PortScanner::Scan();

    int selectedIndex = -1;
    for (std::size_t i = 0; i < ports.size(); ++i) {
        const std::wstring text = ports[i].portName + L" - " + ports[i].friendlyName;
        const LRESULT idx = ::SendMessageW(owner_.comboPort_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
        if (idx >= 0 && previous == text) {
            selectedIndex = static_cast<int>(idx);
        }
    }

    if (selectedIndex < 0 && !ports.empty()) {
        selectedIndex = 0;
    }

    if (selectedIndex >= 0) {
        ::SendMessageW(owner_.comboPort_, CB_SETCURSEL, static_cast<WPARAM>(selectedIndex), 0);
        const std::wstring msg = L"Port list refreshed: " + std::to_wstring(ports.size()) + L" found";
        owner_.AppendLog(LogKind::System, msg);
    } else {
        owner_.AppendLog(LogKind::System, L"Port list refreshed: no ports found");
    }
}

bool WindowActions::OpenSelectedPort() {
    if (owner_.serialPort_.IsOpen()) {
        return true;
    }

    const int selected = static_cast<int>(::SendMessageW(owner_.comboPort_, CB_GETCURSEL, 0, 0));
    if (selected < 0) {
        owner_.AppendLog(LogKind::Error, L"No COM port selected");
        return false;
    }

    wchar_t portText[256] = {};
    ::SendMessageW(owner_.comboPort_, CB_GETLBTEXT, static_cast<WPARAM>(selected), reinterpret_cast<LPARAM>(portText));
    std::wstring selection = portText;
    const std::size_t spacePos = selection.find(L' ');
    const std::wstring portName = (spacePos == std::wstring::npos) ? selection : selection.substr(0, spacePos);

    bool settingsOk = false;
    const serial::PortSettings settings = BuildPortSettingsFromUi(&settingsOk);
    if (!settingsOk) {
        owner_.AppendLog(LogKind::Error, L"Invalid serial settings");
        return false;
    }

    owner_.serialPort_.SetDataCallback([this](const std::vector<uint8_t>& packet) {
        auto* message = new std::vector<uint8_t>(packet);
        if (!::PostMessageW(owner_.window_, WM_APP_SERIAL_DATA, 0, reinterpret_cast<LPARAM>(message))) {
            delete message;
        }
    });

    if (!owner_.serialPort_.Open(portName, settings)) {
        owner_.AppendLog(LogKind::Error, L"Failed to open " + portName);
        return false;
    }

    owner_.txBytes_ = 0;
    owner_.rxBytes_ = 0;
    owner_.UpdateStatusText();

    ::SetWindowTextW(owner_.ledStatus_, L"Connected");
    ::SendMessageW(owner_.statusBar_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(L"Connected"));
    owner_.AppendLog(LogKind::System, L"Opened " + portName + L" @ " + std::to_wstring(settings.baudRate));
    return true;
}

void WindowActions::ClosePort() {
    owner_.serialPort_.SetDataCallback({});
    if (owner_.serialPort_.IsOpen()) {
        owner_.serialPort_.Close();
        ::SetWindowTextW(owner_.ledStatus_, L"Disconnected");
        ::SendMessageW(owner_.statusBar_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(L"Disconnected"));
        owner_.AppendLog(LogKind::System, L"Port closed");
    }
}

void WindowActions::SendInputData() {
    if (!owner_.serialPort_.IsOpen()) {
        owner_.AppendLog(LogKind::Error, L"Port is not open");
        return;
    }

    const int length = ::GetWindowTextLengthW(owner_.editSend_);
    if (length <= 0) {
        return;
    }

    std::wstring text;
    text.resize(static_cast<std::size_t>(length));
    ::GetWindowTextW(owner_.editSend_, text.data(), length + 1);

    const int utf8Len = ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Len <= 1) {
        return;
    }

    std::vector<uint8_t> bytes(static_cast<std::size_t>(utf8Len - 1));
    ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, reinterpret_cast<LPSTR>(bytes.data()), utf8Len - 1, nullptr, nullptr);

    DWORD written = 0;
    if (owner_.serialPort_.Write(bytes.data(), static_cast<DWORD>(bytes.size()), &written)) {
        owner_.txBytes_ += written;
        owner_.UpdateStatusText();
        owner_.AppendLog(LogKind::Tx, L"TX " + MainWindow::BytesToHex(bytes));
    } else {
        owner_.AppendLog(LogKind::Error, L"Write failed");
    }
}

void WindowActions::HandleSerialData(const std::vector<uint8_t>& bytes) {
    owner_.rxBytes_ += static_cast<std::uint64_t>(bytes.size());
    owner_.UpdateStatusText();
    owner_.AppendLog(LogKind::Rx, L"RX " + FormatIncoming(bytes));
}

std::wstring WindowActions::ComboText(HWND combo) {
    const int len = ::GetWindowTextLengthW(combo);
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

    const int bitsSel = static_cast<int>(::SendMessageW(owner_.comboDataBits_, CB_GETCURSEL, 0, 0));
    s.dataBits = static_cast<BYTE>((bitsSel >= 0) ? (bitsSel + 5) : 8);

    const int paritySel = static_cast<int>(::SendMessageW(owner_.comboParity_, CB_GETCURSEL, 0, 0));
    s.parity = (paritySel == 1) ? serial::ParityMode::Odd :
        (paritySel == 2) ? serial::ParityMode::Even :
        (paritySel == 3) ? serial::ParityMode::Mark :
        (paritySel == 4) ? serial::ParityMode::Space :
                           serial::ParityMode::None;

    const int stopSel = static_cast<int>(::SendMessageW(owner_.comboStopBits_, CB_GETCURSEL, 0, 0));
    s.stopBits = (stopSel == 1) ? serial::StopBitsMode::OnePointFive :
        (stopSel == 2) ? serial::StopBitsMode::Two :
                         serial::StopBitsMode::One;

    const int flowSel = static_cast<int>(::SendMessageW(owner_.comboFlow_, CB_GETCURSEL, 0, 0));
    s.flowControl = (flowSel == 1) ? serial::FlowControlMode::Hardware :
        (flowSel == 2) ? serial::FlowControlMode::Software :
                         serial::FlowControlMode::None;

    s.rts = (::SendMessageW(owner_.checkRts_, BM_GETCHECK, 0, 0) == BST_CHECKED);
    s.dtr = (::SendMessageW(owner_.checkDtr_, BM_GETCHECK, 0, 0) == BST_CHECKED);

    if (ok != nullptr) {
        *ok = true;
    }
    return s;
}

std::wstring WindowActions::FormatIncoming(const std::vector<uint8_t>& bytes) const {
    const int mode = static_cast<int>(::SendMessageW(owner_.comboRxMode_, CB_GETCURSEL, 0, 0));
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

    return MainWindow::BytesToHex(bytes);
}

} // namespace ui
