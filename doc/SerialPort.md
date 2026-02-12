# SerialPort

`serial::SerialPort` – обёртка над Windows‑API для работы с последовательными портами. Управляет открытием/закрытием порта, чтением и записью данных, а также настройками уровня передачи.

## Конструктор / Деструктор
- `SerialPort()` – создаёт объект без активного соединения.
- `~SerialPort()` – закрывает порт и освобождает ресурсы.

## Публичные методы
| Метод | Описание |
|-------|----------|
| `bool Open(const std::wstring& portName, const PortSettings& settings)` | Открывает указанный COM‑порт с заданными настройками. Возвращает `true` при успехе.
| `void Close()` | Закрывает открытый порт и освобождает события/треды.
| `bool IsOpen() const noexcept` | Проверяет, открыт ли порт.
| `bool Write(const uint8_t* data, DWORD size, DWORD* writtenBytes)` | Писает данные в порт. Возвращает `true`, если операция завершена успешно; `writtenBytes` содержит фактическое количество записанных байт.
| `bool GetModemStatus(DWORD* modemStatus)` | Получает статус модема (CTS, DSR и т.д.).
| `bool SetRts(bool enabled)` | Устанавливает/снимает RTS‑флаг.
| `bool SetDtr(bool enabled)` | Устанавливает/снимает DTR‑флаг.
| `void SetDataCallback(DataCallback callback)` | Регистрирует коллбэк, который будет вызван при поступлении данных из порта.

## Поле `PortSettings`
```cpp
struct PortSettings {
    DWORD baudRate;          // Скорость передачи (bps)
    BYTE  dataBits;          // Кол‑во битов данных (5–8)
    ParityMode parity;      // Режим четности
    StopBitsMode stopBits;  // Кол‑во стоповых битов
    FlowControlMode flowControl; // Управление потоком
    bool rts;
    bool dtr;
};
```

## Пример использования
```cpp
#include "serial/SerialPort.h"
using namespace serial;

int main(){
    SerialPort port;
    PortSettings s{9600, 8, ParityMode::None,
                    StopBitsMode::One, FlowControlMode::None, true, false};
    if(!port.Open(L"\\\\.\\COM3", s)){
        // открыть не удалось
        return -1;
    }

    // Установим коллбэк для чтения данных
    port.SetDataCallback([](const std::vector<uint8_t>& data){
        // обработка полученных байтов
    });

    const uint8_t msg[] = {0x01, 0x02, 0x03};
    DWORD written = 0;
    if(!port.Write(msg, sizeof(msg), &written)){
        // ошибка записи
    }

    // ... работа с портом …

    port.Close();
    return 0;
}
```

## Технические детали
- Для чтения используется отдельный поток (`ReadThreadProc`). Он ждёт события `readEvent_` и читает данные через `ReadFile`. После получения данных вызывается пользовательский коллбэк.
- Операции записи используют асинхронный режим с событием `writeEvent_`.
- Управление потоком осуществляется через `OVERLAPPED` структуры и функции WinAPI (`CreateEvent`, `CloseHandle`).

---

*См. также: [PortScanner.md](PortScanner.md).*
