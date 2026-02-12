#pragma once
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <strsafe.h>

#include "resource.h"
#include "ui/MainWindow.h"

namespace ui {

// class MainWindow;

class WindowBuilder final {
public:
    explicit WindowBuilder(MainWindow& owner);

    void CreateStatusBar();
    void CreateControls();
    void FillConnectionDefaults();

private:
    MainWindow& owner_;
};

} // namespace ui
