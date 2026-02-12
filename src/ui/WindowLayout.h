#pragma once


#include <algorithm>
#include <windows.h>
#include "ui/MainWindow.h"

namespace ui {

class WindowLayout final {
public:
    explicit WindowLayout(MainWindow& owner);

    void ResizeChildren() const;
    static void ApplyMinTrackSize(MINMAXINFO* minMaxInfo);

private:
    MainWindow& owner_;
};

} // namespace ui
