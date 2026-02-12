# BufferPool

`core::BufferPool` – простая кольцевая буферная структура для хранения байтов. Используется в приложении для временного хранения данных перед записью/чтением.

## Конструктор / Деструктор
- `BufferPool()` – инициализирует внутренний массив размером 65536 байт.
- `~BufferPool()` – освобождает ресурсы (CRITICAL_SECTION).

## Методы
| Метод | Описание |
|-------|----------|
| `bool Write(const char* buffer, DWORD size)` | Добавляет данные в конец буфера. Если буфер заполнен, старые байты удаляются с начала.
| `bool Read(char* buffer, DWORD size, DWORD* readBytes)` | Считывает до `size` байт из начала буфера и убирает их из структуры. `readBytes` – фактическое число прочитанных байтов.
| `bool Peek(char* buffer, DWORD size, DWORD* readBytes)` | Аналогично Read, но не удаляет данные из буфера.
| `void DiscardOldest()` | Удаляет самую старую запись (старший элемент). Полезно при переполнении.

## Пример использования
```cpp
#include "core/BufferPool.h"
using namespace core;

int main(){
    BufferPool pool;
    const char data[] = {1,2,3,4,5};
    DWORD written = 0;
    if(pool.Write(data, sizeof(data))){
        // данные записаны в буфер
    }
    char out[10];
    DWORD read = 0;
    if(pool.Read(out, sizeof(out), &read)){
        // `out` теперь содержит первые 5 байт, read == 5
    }
}
```
