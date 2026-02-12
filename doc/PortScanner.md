# PortScanner

`serial::PortScanner` – утилита для enumeration всех доступных последовательных портов в системе.

## Основной метод
| Метод | Описание |
|-------|----------|
| `static std::vector<PortInfo> Scan()` | Возвращает список найденных портов. Для каждого порта возвращается структура `PortInfo`, содержащая внутреннее имя (`portName`) и пользовательское название (`friendlyName`).

## Структура `PortInfo`
```cpp
struct PortInfo {
    std::wstring portName;     // например, "\\\.\COM3"
    std::wstring friendlyName; // пользовательское описание, если доступно
};
```

## Пример использования
```cpp
#include "serial/PortScanner.h"
using namespace serial;

int main(){
    auto ports = PortScanner::Scan();
    for(const auto& p : ports){
        wprintf(L"%s – %s\n", p.portName.c_str(), p.friendlyName.c_str());
    }
    return 0;
}
```

## Технические детали
- Реализация использует WinAPI `SetupDiGetClassDevs`, `SetupDiEnumDeviceInfo` и `SetupDiGetDeviceRegistryProperty`. Это гарантирует, что даже скрытые или отключённые порты могут быть обнаружены.
- Для получения пользовательского имени используется свойство `SPDRP_F441ех доступных последовательных портов в системе.

## Основной метод
| Метод | Описание |
|-------|----------|
| `static std::vector<PortInfo> Scan()` | Возвращает список найденных портов. Для каждого порта возвращается структура `PortInfo`, содержащая внутреннее имя (`portName`) и пользовательское название (`friendlyName`).

## Структура `PortInfo`
```cpp
struct PortInfo {
    std::wstring portName;     // например, "\\\.\COM3"
    std::wstring friendlyName; // пользовательское описание, если доступно
};
```

## Пример использования
```cpp
#include "serial/PortScanner.h"
using namespace serial;

int main(){
    auto ports = PortScanner::Scan();
    for(const auto& p : ports){
        wprintf(L"%s – %s\n", p.portName.c_str(), p.friendlyName.c_str());
    }
    return 0;
}
```

## Технические детали
- Реализация использует WinAPI `SetupDiGetClassDevs`, `SetupDiEnumDeviceInfo` и `SetupDiGetDeviceRegistryProperty`. Это гарантирует, что даже скрытые или отключённые порты могут быть обнаружены.
- Для получения пользовательского имени используется свойство `SPDRP_FRIENDLYNAME`.

---
