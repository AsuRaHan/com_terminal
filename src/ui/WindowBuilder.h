#pragma once
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <strsafe.h>

#include "resource.h"
#include "ui/MainWindow.h"

namespace ui {

class WindowBuilder final {
public:
    explicit WindowBuilder(MainWindow& owner);

    void CreateStatusBar();
    void CreateControls();
    void FillConnectionDefaults();
    
    // Новые методы для работы с подсказками
    void CreateTooltips();
    void AddTooltip(HWND control, const std::wstring& text, const std::wstring& title = L"");
    void UpdateTooltips(); // Для локализации/обновления
    void AddAllTooltips(); // Добавляет подсказки для всех элементов управления
private:
    MainWindow& owner_;
    
    // Внутренние константы для подсказок
    static constexpr int TOOLTIP_DELAY = 500;    // мс до появления
    static constexpr int TOOLTIP_DURATION = 8000; // мс отображения
};

} // namespace ui