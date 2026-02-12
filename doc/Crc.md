# CRC Utilities

`core::Crc8Dallas`, `core::Crc16Ibm`, и `core::Crc32IsoHdlc` – простые функции расчёта контрольных сумм, используемые в приложении для проверки целостности данных.

## Функции
| Функция | Описание |
|---------|----------|
| `uint8_t Crc8Dallas(const uint8_t* data, std::size_t size)` | Вычисляет 8‑битный CRC по протоколу Dallas (i.e., Maximintegrated). |
| `uint16_t Crc16Ibm(const uint8_t* data, std::size_t size)` | Вычисляет 16‑битный CRC‑IBM (Modbus). |
| `uint32_t Crc32IsoHdlc(const uint8_t* data, std::size_t size)` | Вычисляет 32‑битный CRC‑ISO/HDLC. |

## Пример использования
```cpp
#include "core/Crc.h"
using namespace core;

int main(){
    const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    std::size_t len = sizeof(data);
    auto crc8  = Crc8Dallas(data, len);
    auto crc16 = Crc16Ibm(data, len);
    auto crc32 = Crc32IsoHdlc(data, len);

    // выводим результаты
    printf("CRC8: 0x%02X\n", crc8);
    printf("CRC16: 0x%04X\n", crc16);
    printf("CRC32: 0x%08X\n", crc32);
}
```
