# COMTerminal — Документация

Полная документация всех компонентов проекта COMTerminal.

## Содержание

### Пользовательский интерфейс
- [UI](UI.md) — компоненты интерфейса (MainWindow, WindowBuilder, WindowLayout, WindowActions)

### Работа с последовательными портами
- [SerialPort](SerialPort.md) — обёртка над Windows‑API для управления COM‑портами
- [PortScanner](PortScanner.md) — поиск доступных последовательных портов

### Утилиты и вспомогательные компоненты
- [BufferPool](BufferPool.md) — управление пулом буферов для эффективного использования памяти
- [SafeHandle](SafeHandle.md) — безопасное управление HANDLE с автоматическим закрытием
- [LogVirtualizer](LogVirtualizer.md) — виртуализация логирования
- [Crc](Crc.md) — вычисление контрольной суммы CRC

---

**COMTerminal** — приложение для взаимодействия с устройствами через последовательный порт.
