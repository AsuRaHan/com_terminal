#include "ui/WindowLayout.h"

namespace ui {

WindowLayout::WindowLayout(MainWindow& owner) : owner_(owner) {}







void WindowLayout::ResizeChildren() const {
    RECT client{};
    ::GetClientRect(owner_.window_, &client);

    // Получаем высоту статусбара
    RECT statusRect{};
    int statusHeight = 0;
    if (owner_.statusBar_ != nullptr && ::GetWindowRect(owner_.statusBar_, &statusRect)) {
        statusHeight = statusRect.bottom - statusRect.top;
        ::SendMessage(owner_.statusBar_, WM_SIZE, 0, 0);
    }

    // ==================================================
    // ВСЕ РАЗМЕРЫ В ОДНОМ МЕСТЕ - ЛЕГКО МЕНЯТЬ!
    // ==================================================
    
    // Базовые отступы
    const int MARGIN = 12;           // Отступ от краев окна
    const int GAP = 8;              // Расстояние между элементами
    const int GROUP_PADDING = 12;    // Внутренний отступ в группах
    const int GROUP_HEADER = 16;     // Высота заголовка группы
    
    // Размеры элементов
    const int ROW_HEIGHT = 26;       // Высота строки
    const int COMBO_DROP_HEIGHT = 250; // Высота выпадающего списка
    
    // === Размеры элементов Port Settings ===
    const int BTN_REFRESH_WIDTH = 70;
    const int COMBO_PORT_WIDTH = 276; // Сделаем чуть шире для отображения имен портов
    const int COMBO_BAUD_WIDTH = 110;
    const int BTN_OPEN_WIDTH = 70;
    const int BTN_CLOSE_WIDTH = 70;
    
    const int COMBO_DATABITS_WIDTH = 70;
    const int COMBO_PARITY_WIDTH = 90;
    const int COMBO_STOPBITS_WIDTH = 70;
    const int COMBO_FLOW_WIDTH = 100;
    const int CHECK_RTS_WIDTH = 50;
    const int CHECK_DTR_WIDTH = 50;
    
    // === Размеры элементов Terminal Control ===
    const int LED_STATUS_WIDTH = 100;
    const int COMBO_RXMODE_WIDTH = 90;
    const int CHECK_SAVELOG_WIDTH = 80;
    const int BTN_CLEAR_WIDTH = 70;
    
    // === Размеры элементов Send Data ===
    const int BTN_SEND_WIDTH = 72;
    
    // === РАССЧИТЫВАЕМ РАЗМЕРЫ ГРУПП ===
    
    // Port Group - точный расчет
    const int PORT_GROUP_WIDTH = 
        BTN_REFRESH_WIDTH + COMBO_PORT_WIDTH + COMBO_BAUD_WIDTH + BTN_OPEN_WIDTH + 
        (4 * GAP) + (2 * GROUP_PADDING);
    
    const int PORT_GROUP_HEIGHT = 
        (ROW_HEIGHT * 2) + GAP + GROUP_PADDING + GROUP_HEADER;
    
    // Terminal Control Group - одна строка элементов
    const int CTRL_GROUP_HEIGHT = 
        ROW_HEIGHT + GROUP_PADDING + GROUP_HEADER;
    
    // Send Group - резиновая высота
    // Высота рассчитывается в коде
    
    // ==================================================
    // ПОЗИЦИОНИРОВАНИЕ
    // ==================================================
    
    int left = MARGIN;
    int top = MARGIN;
    int right = client.right - MARGIN;
    int bottom = client.bottom - statusHeight - MARGIN;

    // ============ ГРУППА 1: Port Settings ============
    RECT group1Rect = {
        left, 
        top, 
        left + PORT_GROUP_WIDTH, 
        top + PORT_GROUP_HEIGHT
    };
    
    ::MoveWindow(owner_.groupPort_, 
                 group1Rect.left, group1Rect.top,
                 group1Rect.right - group1Rect.left,
                 group1Rect.bottom - group1Rect.top, TRUE);

    // Элементы внутри Port Settings
    int x = group1Rect.left + GROUP_PADDING;
    int y = group1Rect.top + GROUP_PADDING + 8; // +8 для заголовка
    
    // Ряд 1: Refresh, COM Port, Baud Rate, Open/Close
    ::MoveWindow(owner_.buttonRefresh_, x, y, BTN_REFRESH_WIDTH, ROW_HEIGHT, TRUE);
    x += BTN_REFRESH_WIDTH + GAP;
    
    ::MoveWindow(owner_.comboPort_, x, y, COMBO_PORT_WIDTH, COMBO_DROP_HEIGHT, TRUE);
    x += COMBO_PORT_WIDTH + GAP;
    
    ::MoveWindow(owner_.comboBaud_, x, y, COMBO_BAUD_WIDTH, COMBO_DROP_HEIGHT, TRUE);
    x += COMBO_BAUD_WIDTH + GAP;
    
    ::MoveWindow(owner_.buttonOpen_, x, y, BTN_OPEN_WIDTH, ROW_HEIGHT*2, TRUE);
    ::MoveWindow(owner_.buttonClose_, x, y, BTN_CLOSE_WIDTH, ROW_HEIGHT*2, TRUE);
    
    // Ряд 2: Data Bits, Parity, Stop Bits, Flow Control, RTS, DTR
    x = group1Rect.left + GROUP_PADDING;
    y += ROW_HEIGHT + GAP;
    
    ::MoveWindow(owner_.comboDataBits_, x, y, COMBO_DATABITS_WIDTH, COMBO_DROP_HEIGHT, TRUE);
    x += COMBO_DATABITS_WIDTH + GAP;
    
    ::MoveWindow(owner_.comboParity_, x, y, COMBO_PARITY_WIDTH, COMBO_DROP_HEIGHT, TRUE);
    x += COMBO_PARITY_WIDTH + GAP;
    
    ::MoveWindow(owner_.comboStopBits_, x, y, COMBO_STOPBITS_WIDTH, COMBO_DROP_HEIGHT, TRUE);
    x += COMBO_STOPBITS_WIDTH + GAP;
    
    ::MoveWindow(owner_.comboFlow_, x, y, COMBO_FLOW_WIDTH, COMBO_DROP_HEIGHT, TRUE);
    x += COMBO_FLOW_WIDTH + GAP;
    
    ::MoveWindow(owner_.checkRts_, x, y + 3, CHECK_RTS_WIDTH, ROW_HEIGHT, TRUE);
    x += CHECK_RTS_WIDTH + GAP;
    
    ::MoveWindow(owner_.checkDtr_, x, y + 3, CHECK_DTR_WIDTH, ROW_HEIGHT, TRUE);

    // ============ ГРУППА 2: Statistics ============
    RECT group2Rect = {
        group1Rect.right + GAP,
        top,
        right,
        group1Rect.bottom
    };
    
    ::MoveWindow(owner_.groupStats_,
                 group2Rect.left, group2Rect.top,
                 group2Rect.right - group2Rect.left,
                 group2Rect.bottom - group2Rect.top, TRUE);

    // Статистика - резиновая ширина
    x = group2Rect.left + GROUP_PADDING;
    y = group2Rect.top + GROUP_PADDING + 8;
    
    int statsItemWidth = (group2Rect.right - group2Rect.left - (GROUP_PADDING * 2) - (GAP * 3)) / 4;
    
    ::MoveWindow(owner_.textTxTotal_, x, y, statsItemWidth, ROW_HEIGHT, TRUE);
    x += statsItemWidth + GAP;
    ::MoveWindow(owner_.textRxTotal_, x, y, statsItemWidth, ROW_HEIGHT, TRUE);
    x += statsItemWidth + GAP;
    ::MoveWindow(owner_.textTxRate_, x, y, statsItemWidth, ROW_HEIGHT, TRUE);
    x += statsItemWidth + GAP;
    ::MoveWindow(owner_.textRxRate_, x, y, statsItemWidth, ROW_HEIGHT, TRUE);

    // ============ ГРУППА 3: Terminal Log ============
    int logTop = group1Rect.bottom + GAP;
    int logBottom = bottom - (CTRL_GROUP_HEIGHT + GAP + 100); // 100 - высота Send группы
    
    RECT group3Rect = {left, logTop, right, logBottom};
    ::MoveWindow(owner_.groupLog_,
                 group3Rect.left, group3Rect.top,
                 group3Rect.right - group3Rect.left,
                 group3Rect.bottom - group3Rect.top, TRUE);

    // Rich Edit на всю ширину группы
    x = group3Rect.left + GROUP_PADDING;
    y = group3Rect.top + GROUP_PADDING + 8;
    
    ::MoveWindow(owner_.richLog_,
                 x, y,
                 group3Rect.right - x - GROUP_PADDING,
                 group3Rect.bottom - y - GROUP_PADDING, TRUE);

    // ============ ГРУППА 4: Terminal Control ============
    int ctrlTop = group3Rect.bottom + GAP;
    
    RECT group4Rect = {
        left, 
        ctrlTop, 
        right, 
        ctrlTop + CTRL_GROUP_HEIGHT
    };
    
    ::MoveWindow(owner_.groupTerminalCtrl_,
                 group4Rect.left, group4Rect.top,
                 group4Rect.right - group4Rect.left,
                 group4Rect.bottom - group4Rect.top, TRUE);

    // Terminal Control в одну строку
    x = group4Rect.left + GROUP_PADDING;
    y = group4Rect.top + GROUP_PADDING + 8;
    
    ::MoveWindow(owner_.ledStatus_, x, y, LED_STATUS_WIDTH, ROW_HEIGHT, TRUE);
    x += LED_STATUS_WIDTH + GAP;
    
    ::MoveWindow(owner_.comboRxMode_, x, y, COMBO_RXMODE_WIDTH, COMBO_DROP_HEIGHT, TRUE);
    x += COMBO_RXMODE_WIDTH + GAP;
    
    ::MoveWindow(owner_.checkSaveLog_, x, y + 3, CHECK_SAVELOG_WIDTH, ROW_HEIGHT, TRUE);
    x += CHECK_SAVELOG_WIDTH + GAP;
    
    ::MoveWindow(owner_.buttonClear_, x, y, BTN_CLEAR_WIDTH, ROW_HEIGHT, TRUE);

    // ============ ГРУППА 5: Send Data ============
    int sendTop = group4Rect.bottom + GAP;
    // int sendHeight = bottom - sendTop;
    
    RECT group5Rect = {left, sendTop, right, bottom};
    ::MoveWindow(owner_.groupSend_,
                 group5Rect.left, group5Rect.top,
                 group5Rect.right - group5Rect.left,
                 group5Rect.bottom - group5Rect.top, TRUE);

    // Send Data - большое поле ввода
    x = group5Rect.left + GROUP_PADDING;
    y = group5Rect.top + GROUP_PADDING + 8;
    
    int editWidth = group5Rect.right - x - GROUP_PADDING - BTN_SEND_WIDTH - GAP;
    
    ::MoveWindow(owner_.editSend_,
                 x, y,
                 editWidth,
                 group5Rect.bottom - y - GROUP_PADDING, TRUE);
    
    ::MoveWindow(owner_.buttonSend_,
                 x + editWidth + GAP,
                 y,
                 BTN_SEND_WIDTH,
                 group5Rect.bottom - y - GROUP_PADDING, TRUE);
}








void WindowLayout::ApplyMinTrackSize(MINMAXINFO* minMaxInfo) {
    if (minMaxInfo == nullptr) {
        return;
    }
    minMaxInfo->ptMinTrackSize.x = 900;
    minMaxInfo->ptMinTrackSize.y = 700;
}

} // namespace ui
