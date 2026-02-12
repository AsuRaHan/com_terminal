# LogVirtualizer

`core::LogVirtualizer` – класс, реализующий виртуальный лог с буферизацией. Он хранит строки в памяти до достижения порога переписывания и при необходимости сохраняет их на диск.

## Конструктор
```
LogVirtualizer(std::size_t maxBufferedLines,
               std::uint64_t rewriteLineThreshold,
               std::size_t rewriteByteThreshold);
```
* `maxBufferedLines` – максимальное количество строк, которые можно хранить в памяти.
* `rewriteLineThreshold` – число строк, после которого считается необходимым переписать лог на диск.
* `rewriteByteThreshold` – порог в байтах для переписывания.

## Методы
| Метод | Описание |
|-------|----------|
| `bool Initialize(const std::wstring& logDirectory)` | Создаёт директорию и открывает файл‑сессию. Возвращает true при успешной инициализации. |
| `bool AppendLine(const std::wstring& line, COLORREF color, bool persistToDisk)` | Добавляет строку в буфер (и опционально сразу на диск). Цвет используется для подсветки в UI. |
| `bool ShouldRewrite() const noexcept` | Проверяет условия переписывания логов: превышены ли пороги строк/байт. |
| `std::vector<VirtualLogLine> SnapshotBuffer() const` | Возвращает копию текущего буфера для отображения в окне. |
| `void MarkRewriteDone() noexcept` | Очищает внутренние счётчики после переписывания. |
| `std::wstring SessionFilePath() const` | Путь к файлу‑сессии, где находятся накопленные логи. |

## Пример использования
```cpp
#include "core/LogVirtualizer.h"
using namespace core;

int main(){
    LogVirtualizer log(1000, 5000, 1024 * 10);
    if(!log.Initialize(L"C:\\Logs\\App1")) return -1;

    log.AppendLine(L"Start application", RGB(255,255,255), false);

    if(log.ShouldRewrite()){
        auto buffer = log.SnapshotBuffer();
        // вывести в UI...
        log.MarkRewriteDone();
    }
}
```
