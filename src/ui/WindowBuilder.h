#pragma once

namespace ui {

class MainWindow;

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
