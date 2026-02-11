#pragma once

#include <windows.h>

#include <string>
#include <vector>

#include "serial/SerialPort.h"

namespace ui {

class MainWindow;

class WindowActions final {
public:
    explicit WindowActions(MainWindow& owner);

    void RefreshPorts();
    bool OpenSelectedPort();
    void ClosePort();
    void SendInputData();
    void HandleSerialData(const std::vector<uint8_t>& bytes);

private:
    static std::wstring ComboText(HWND combo);
    std::wstring FormatIncoming(const std::vector<uint8_t>& bytes) const;
    serial::PortSettings BuildPortSettingsFromUi(bool* ok) const;

    MainWindow& owner_;
};

} // namespace ui
