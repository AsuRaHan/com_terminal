#pragma once

#include <windows.h>

namespace ui {

class MainWindow;

class WindowLayout final {
public:
    explicit WindowLayout(MainWindow& owner);

    void ResizeChildren() const;
    static void ApplyMinTrackSize(MINMAXINFO* minMaxInfo);

private:
    MainWindow& owner_;
};

} // namespace ui
