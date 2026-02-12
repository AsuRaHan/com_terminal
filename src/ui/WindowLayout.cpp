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
    ::MoveWindow(owner_.ledStatus_, right - 160, y, 152, rowHeight, TRUE);

    y += rowHeight + gap;

    ::MoveWindow(owner_.comboDataBits_, left, y, 120, comboDropHeight, TRUE);
    ::MoveWindow(owner_.comboParity_, left + 130, y, 130, comboDropHeight, TRUE);
    ::MoveWindow(owner_.comboStopBits_, left + 270, y, 120, comboDropHeight, TRUE);
    ::MoveWindow(owner_.comboFlow_, left + 400, y, 140, comboDropHeight, TRUE);
    ::MoveWindow(owner_.checkRts_, left + 545, y + 3, 60, rowHeight, TRUE);
    ::MoveWindow(owner_.checkDtr_, left + 610, y + 3, 60, rowHeight, TRUE);
    ::MoveWindow(owner_.comboRxMode_, right - 190, y, 90, comboDropHeight, TRUE);
    ::MoveWindow(owner_.checkSaveLog_, right - 95, y + 3, 90, rowHeight, TRUE);

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
    minMaxInfo->ptMinTrackSize.x = 800;
    minMaxInfo->ptMinTrackSize.y = 600;
}

} // namespace ui
