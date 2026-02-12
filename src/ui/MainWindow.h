#pragma once

#include <windows.h>
#include <dbt.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "core/LogVirtualizer.h"
#include "serial/SerialPort.h"

namespace ui {

class WindowBuilder;
class WindowLayout;
class WindowActions;

enum class LogKind {
    Rx,
    Tx,
    System,
    Error
};

class MainWindow final {
public:
    explicit MainWindow(HINSTANCE instance);
    ~MainWindow();

    bool Create(int nCmdShow);
    [[nodiscard]] HWND Handle() const noexcept;

private:
    friend class WindowBuilder;
    friend class WindowLayout;
    friend class WindowActions;

    static LRESULT CALLBACK WndProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WndProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    bool RegisterClass();

    void AppendLog(LogKind kind, const std::wstring& text);
    void AppendLineToRichEdit(const std::wstring& line, COLORREF color, bool scrollToCaret);
    void RebuildRichEditFromVirtualBuffer();
    void UpdateStatusText();
    static COLORREF ColorForLogKind(LogKind kind) noexcept;

    static std::wstring BuildTimestamp();
    static std::wstring BytesToHex(const std::vector<uint8_t>& bytes);

    HINSTANCE instance_;
    HWND window_;
    HWND statusBar_;
    HWND comboPort_;
    HWND comboBaud_;
    HWND buttonRefresh_;
    HWND buttonOpen_;
    HWND buttonClose_;
    HWND richLog_;
    HWND editSend_;
    HWND buttonSend_;
    HWND buttonClear_; // New button to clear terminal output
    HWND ledStatus_;
    HWND comboDataBits_;
    HWND comboParity_;
    HWND comboStopBits_;
    HWND comboFlow_;
    HWND checkRts_;
    HWND checkDtr_;
    HWND comboRxMode_;
    HWND checkSaveLog_;

    HBRUSH ledBrushDisconnected_;
    HBRUSH ledBrushConnected_;

    HDEVNOTIFY deviceNotify_;

    serial::SerialPort serialPort_;
    core::LogVirtualizer logVirtualizer_;
    bool rebuildingRichEdit_;
    std::uint64_t txBytes_;
    std::uint64_t rxBytes_;

    std::unique_ptr<WindowBuilder> builder_;
    std::unique_ptr<WindowLayout> layout_;
    std::unique_ptr<WindowActions> actions_;
    // Toggles visibility of the Open/Close buttons depending on connection status.
    void UpdateConnectionButtons();
    void ClearTerminal(); // Clears the rich edit log and resets counters
};

} // namespace ui
