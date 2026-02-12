#pragma once

#include <windows.h>
#include <commctrl.h>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>
#include <sstream>      // Для stringstream
#include <iomanip>      // Для манипуляторов
#include <strsafe.h>    // Для StringCchPrintfW

#include "resource.h"
#include "serial/PortScanner.h"
#include "ui/MainWindow.h"


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
