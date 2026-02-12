# SafeHandle

`core::SafeHandle` – простая обёртка над Windows‑HANDLE, обеспечивающая безопасное освобождение ресурса в деструкторе и поддержку перемещения.

## Конструкторы
- `SafeHandle() noexcept` – создаёт невалидный хэндл (`INVALID_HANDLE_VALUE`).
- `explicit SafeHandle(HANDLE handle) noexcept` – оборачивает существующий HANDLE.
- Move‑конструктор/оператор присваивания – перемещает владение.

## Методы
| Метод | Описание |
|-------|----------|
| `bool IsValid() const noexcept` | Проверяет, что хэндл не равен `INVALID_HANDLE_VALUE`. |
| `HANDLE Get() const noexcept` | Возвращает внутренний HANDLE без изменения владения. |
| `HANDLE Release() noexcept` | Отдаёт управление внешнему коду и делает внутренний HANDLE недействительным. |
| `void Reset(HANDLE handle = INVALID_HANDLE_VALUE) noexcept` | Заменяет текущий хэндл новым и освобождает старый, если он валиден. |

## Пример использования
```cpp
#include "core/SafeHandle.h"
using namespace core;

int main(){
    SafeHandle h(CreateFileW(L"\\\.\COM3", GENERIC_READ | GENERIC_WRITE,
                             0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if(!h.IsValid()) return -1; // открыть не удалось

    // Работа с h.Get()…

    // Отдаём управление и очищаем самостоятельно
    HANDLE raw = h.Release();
    CloseHandle(raw);
}
```
