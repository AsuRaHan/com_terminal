#pragma once

#include <windows.h>
#include <dbt.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "core/LogVirtualizer.h"
#include "serial/SerialPort.h"

#include <versionhelpers.h>  // Для IsWindows10OrGreater()
#include <dwmapi.h>          // Для DwmSetWindowAttribute()
// #pragma comment(lib, "dwmapi.lib")

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

    bool isDarkTheme_;
    // void ApplyTheme(bool darkMode);
    // void UpdateThemeMenu();
        // Цвета для лога с учетом темы - НЕ статический!
    COLORREF ColorForLogKindWithTheme(LogKind kind);

    // COLORREF AdjustColorForTheme(COLORREF color, LogKind kind);

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
    HWND buttonClear_;
    HWND ledStatus_;
    HWND comboDataBits_;
    HWND comboParity_;
    HWND comboStopBits_;
    HWND comboFlow_;
    HWND checkRts_;
    HWND checkDtr_;
    HWND comboRxMode_;
    HWND checkSaveLog_;

    HWND groupPort_;
    HWND groupStats_;
    HWND groupTerminalCtrl_;
    HWND groupLog_;
    HWND groupSend_;

    HWND textTxTotal_;
    HWND textRxTotal_;
    HWND textTxRate_;
    HWND textRxRate_;
    // Handle for tooltip window used to provide user hints
    HWND tooltip_; // Added for UI hint support

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

    HBRUSH bgBrush_;        // Кисть фона
    HBRUSH editBrush_;      // Кисть для EDIT
    HBRUSH comboBrush_;     // Кисть для COMBOBOX
    HBRUSH groupBrush_;     // Кисть для групп
    HBRUSH staticBrush_;    // Кисть для статиков

    // Добавь методы для управления темой и цветами, а также для обновления состояния UI
    // void CreateThemeBrushes(bool darkMode); 
    // void DestroyThemeBrushes();
    // void ApplyThemesToAllControls();
    // Toggles visibility of the Open/Close buttons depending on connection status.
    void UpdateConnectionButtons();
    void ClearTerminal(); // Clears the rich edit log and resets counters
    void ShowLogContextMenu(int x, int y); // Shows context menu for log actions (copy, clear, save)
    void CopySelectedText(); // Copies selected text from rich edit to clipboard
    void SelectAllText(); // Selects all text in the rich edit control
    void SaveLogToFile(); // Opens Save File dialog and saves log content to a file
    // void SaveThemeSetting(bool darkMode); // Saves the user's theme preference to the registry or config file
    // void LoadThemeSetting(); // Loads the user's theme preference from the registry or config file
    // void ReapplyLogColors(); // Reapplies colors to existing log entries based on current theme
};

} // namespace ui
