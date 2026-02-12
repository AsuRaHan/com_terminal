#include "ui/WindowLayout.h"

#include <windows.h>

#include <algorithm>

#include "ui/MainWindow.h"

namespace ui {

WindowLayout::WindowLayout(MainWindow& owner) : owner_(owner) {}

void WindowLayout::ResizeChildren() const {
    RECT client{};
    ::GetClientRect(owner_.window_, &client);

    if (owner_.statusBar_ != nullptr) {
        ::SendMessage(owner_.statusBar_, WM_SIZE, 0, 0);
    }

    RECT statusRect{};
    int statusHeight = 0;
    if (owner_.statusBar_ != nullptr && ::GetWindowRect(owner_.statusBar_, &statusRect)) {
        statusHeight = statusRect.bottom - statusRect.top;
    }

    const int left = 8;
    const int top = 8;
    const int right = client.right - 8;

    int y = top;
    const int rowHeight = 28;
    const int comboDropHeight = 240;
    const int gap = 8;

    ::MoveWindow(owner_.comboPort_, left, y, 250, comboDropHeight, TRUE);
    ::MoveWindow(owner_.comboBaud_, left + 260, y, 120, comboDropHeight, TRUE);
    ::MoveWindow(owner_.buttonRefresh_, left + 390, y, 90, rowHeight, TRUE);
    // Place both Open and Close buttons at the same coordinates so they overlap.
    // The visibility of each button will be toggled based on connection status.
    const int connectButtonX = left + 490; // shared X position
    ::MoveWindow(owner_.buttonOpen_, connectButtonX, y, 80, rowHeight, TRUE);
    ::MoveWindow(owner_.buttonClose_, connectButtonX, y, 80, rowHeight, TRUE);
    // Place Clear button next to the Open/Close buttons on the first row.
    // It uses the same height and is positioned immediately after Close.
    ::MoveWindow(owner_.buttonClear_, connectButtonX + 80, y, 80, rowHeight, TRUE);
    ::MoveWindow(owner_.ledStatus_, right - 160, y, 152, rowHeight, TRUE);

    y += rowHeight + gap;

    // Second row – serial parameters
    const int dataBitsX = left;
    const int parityX = dataBitsX + 120 + 10;   // 120 width + 10 gap
    const int stopBitsX = parityX + 160 + 10;    // 160 width + 10 gap
    const int flowX = stopBitsX + 120 + 10;      // 120 width + 10 gap

    ::MoveWindow(owner_.comboDataBits_, dataBitsX, y, 120, comboDropHeight, TRUE);
    ::MoveWindow(owner_.comboParity_, parityX, y, 160, comboDropHeight, TRUE);
    ::MoveWindow(owner_.comboStopBits_, stopBitsX, y, 120, comboDropHeight, TRUE);
    ::MoveWindow(owner_.comboFlow_, flowX, y, 140, comboDropHeight, TRUE);

    // Position RTS/DTR checkboxes on the far right side, leaving space for RX mode and Save Log
    const int rtsX = right - 280;   // leave room for two other controls
    const int dtrX = rtsX + 70;     // small gap between them
    ::MoveWindow(owner_.checkRts_, rtsX, y + 3, 60, rowHeight, TRUE);
    ::MoveWindow(owner_.checkDtr_, dtrX, y + 3, 60, rowHeight, TRUE);

    // RX mode combo and Save Log checkbox positioned after DTR
    const int rxModeX = right - 140;
    const int saveLogX = right - 50;
    ::MoveWindow(owner_.comboRxMode_, rxModeX, y, 90, comboDropHeight, TRUE);
    ::MoveWindow(owner_.checkSaveLog_, saveLogX, y + 3, 90, rowHeight, TRUE);

    y += rowHeight + gap;

    const int sendHeight = 90;
    const int logBottom = client.bottom - statusHeight - sendHeight - (gap * 2);
    ::MoveWindow(owner_.richLog_, left, y, right - left, std::max(100, logBottom - y), TRUE);

    const int sendTop = client.bottom - statusHeight - sendHeight - 8;
    ::MoveWindow(owner_.editSend_, left, sendTop, right - left - 90, sendHeight, TRUE);
    ::MoveWindow(owner_.buttonSend_, right - 80, sendTop, 72, sendHeight, TRUE);
}

void WindowLayout::ApplyMinTrackSize(MINMAXINFO* minMaxInfo) {
    if (minMaxInfo == nullptr) {
        return;
    }
    minMaxInfo->ptMinTrackSize.x = 900;
    minMaxInfo->ptMinTrackSize.y = 700;
}

} // namespace ui
