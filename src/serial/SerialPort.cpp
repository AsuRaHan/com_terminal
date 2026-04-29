#include "serial/SerialPort.h"

#include <array>

namespace {

BYTE ToParity(serial::ParityMode mode) {
    switch (mode) {
    case serial::ParityMode::None:
        return NOPARITY;
    case serial::ParityMode::Odd:
        return ODDPARITY;
    case serial::ParityMode::Even:
        return EVENPARITY;
    case serial::ParityMode::Mark:
        return MARKPARITY;
    case serial::ParityMode::Space:
        return SPACEPARITY;
    }
    return NOPARITY;
}

BYTE ToStopBits(serial::StopBitsMode mode) {
    switch (mode) {
    case serial::StopBitsMode::One:
        return ONESTOPBIT;
    case serial::StopBitsMode::OnePointFive:
        return ONE5STOPBITS;
    case serial::StopBitsMode::Two:
        return TWOSTOPBITS;
    }
    return ONESTOPBIT;
}

bool ConfigurePort(HANDLE port, const serial::PortSettings& settings) {
    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!::GetCommState(port, &dcb)) {
        return false;
    }

    dcb.BaudRate = settings.baudRate;
    dcb.ByteSize = settings.dataBits;
    dcb.Parity = ToParity(settings.parity);
    dcb.StopBits = ToStopBits(settings.stopBits);
    dcb.fBinary = TRUE;
    dcb.fParity = (settings.parity != serial::ParityMode::None) ? TRUE : FALSE;

    dcb.fOutxCtsFlow = FALSE;
    dcb.fRtsControl = settings.rts ? RTS_CONTROL_ENABLE : RTS_CONTROL_DISABLE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = settings.dtr ? DTR_CONTROL_ENABLE : DTR_CONTROL_DISABLE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;

    if (settings.flowControl == serial::FlowControlMode::Hardware) {
        dcb.fOutxCtsFlow = TRUE;
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    } else if (settings.flowControl == serial::FlowControlMode::Software) {
        dcb.fOutX = TRUE;
        dcb.fInX = TRUE;
    }

    if (!::SetCommState(port, &dcb)) {
        return false;
    }

    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 200;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    return ::SetCommTimeouts(port, &timeouts) == TRUE;
}

} // namespace

namespace serial {

SerialPort::SerialPort() : readOverlapped_{}, writeOverlapped_{}, running_(false) {
    readOverlapped_.hEvent = nullptr;
    writeOverlapped_.hEvent = nullptr;
}

SerialPort::~SerialPort() {
    Close();
}

bool SerialPort::Open(const std::wstring& portName, const PortSettings& settings) {
    Close();

    // 1. Открываем порт с overlapped режимом
    std::wstring path = L"\\\\.\\" + portName;
    HANDLE rawPort = ::CreateFileW(
        path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,  // Критично!
        nullptr);

    if (rawPort == INVALID_HANDLE_VALUE) {
        return false;
    }
    port_.Reset(rawPort);

    // 2. Создаем все необходимые события
    HANDLE rawReadEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE rawWriteEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE rawWaitEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE rawShutdownEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

    if (!rawReadEvent || !rawWriteEvent || !rawWaitEvent || !rawShutdownEvent) {
        Close();
        return false;
    }

    readEvent_.Reset(rawReadEvent);
    writeEvent_.Reset(rawWriteEvent);
    waitEvent_.Reset(rawWaitEvent);
    shutdownEvent_.Reset(rawShutdownEvent);

    // 3. Настраиваем OVERLAPPED структуры
    readOverlapped_ = OVERLAPPED{};
    writeOverlapped_ = OVERLAPPED{};
    waitOverlapped_ = OVERLAPPED{};
    
    readOverlapped_.hEvent = readEvent_.Get();
    writeOverlapped_.hEvent = writeEvent_.Get();
    waitOverlapped_.hEvent = waitEvent_.Get();

    // 4. Настраиваем порт (DCB, таймауты)
    if (!ConfigurePort(port_.Get(), settings)) {
        Close();
        return false;
    }

    // 5. Устанавливаем маску событий - ждем только прихода данных
    if (!::SetCommMask(port_.Get(), EV_RXCHAR)) {
        Close();
        return false;
    }

    // 6. Запускаем поток чтения
    running_.store(true);
    HANDLE rawThread = ::CreateThread(nullptr, 0, &SerialPort::ReadThreadProc, this, 0, nullptr);
    if (rawThread == nullptr) {
        Close();
        return false;
    }

    threadHandle_.Reset(rawThread);
    return true;
}

void SerialPort::Close() {
    const bool wasRunning = running_.exchange(false);

    if (wasRunning) {
        // Прерываем все ожидающие операции
        if (port_.IsValid()) {
            ::CancelIoEx(port_.Get(), nullptr);
        }
        
        // Будим поток чтения
        if (shutdownEvent_.IsValid()) {
            ::SetEvent(shutdownEvent_.Get());
        }

        // Ждем завершения потока (с таймаутом)
        if (threadHandle_.IsValid()) {
            ::WaitForSingleObject(threadHandle_.Get(), 3000);
        }
    }

    // Освобождаем ресурсы
    threadHandle_.Reset();
    readEvent_.Reset();
    writeEvent_.Reset();
    waitEvent_.Reset();
    shutdownEvent_.Reset();
    port_.Reset();
    
    // Сбрасываем overlapped структуры
    readOverlapped_ = OVERLAPPED{};
    writeOverlapped_ = OVERLAPPED{};
    waitOverlapped_ = OVERLAPPED{};
}

bool SerialPort::IsOpen() const noexcept {
    return running_.load() && port_.IsValid();
}

bool SerialPort::Write(const uint8_t* data, DWORD size, DWORD* writtenBytes) {
    if (!IsOpen() || data == nullptr || size == 0 || writtenBytes == nullptr) {
        return false;
    }

    *writtenBytes = 0;
    ::ResetEvent(writeEvent_.Get());

    const BOOL ok = ::WriteFile(port_.Get(), data, size, writtenBytes, &writeOverlapped_);
    if (ok == TRUE) {
        return true;
    }

    const DWORD error = ::GetLastError();
    if (error != ERROR_IO_PENDING) {
        return false;
    }

    HANDLE waits[2] = {writeEvent_.Get(), shutdownEvent_.Get()};
    const DWORD wait = ::WaitForMultipleObjects(2, waits, FALSE, 3000);
    if (wait != WAIT_OBJECT_0) {
        return false;
    }

    return ::GetOverlappedResult(port_.Get(), &writeOverlapped_, writtenBytes, FALSE) == TRUE;
}

bool SerialPort::GetModemStatus(DWORD* modemStatus) {
    if (!IsOpen() || modemStatus == nullptr) {
        return false;
    }
    return ::GetCommModemStatus(port_.Get(), modemStatus) == TRUE;
}

bool SerialPort::SetRts(bool enabled) {
    if (!IsOpen()) {
        return false;
    }
    return ::EscapeCommFunction(port_.Get(), enabled ? SETRTS : CLRRTS) == TRUE;
}

bool SerialPort::SetDtr(bool enabled) {
    if (!IsOpen()) {
        return false;
    }
    return ::EscapeCommFunction(port_.Get(), enabled ? SETDTR : CLRDTR) == TRUE;
}

void SerialPort::SetDataCallback(DataCallback callback) {
    callback_ = std::move(callback);
}

DWORD WINAPI SerialPort::ReadThreadProc(LPVOID param) {
    SerialPort* self = static_cast<SerialPort*>(param);
    return self->ReadThreadMain();
}

// DWORD SerialPort::ReadThreadMain() {
//     std::array<uint8_t, 1024> readBuffer{};
//     HANDLE waits[2] = {readEvent_.Get(), shutdownEvent_.Get()};

//     while (running_.load()) {
//         DWORD readBytes = 0;
//         ::ResetEvent(readEvent_.Get());

//         const BOOL ok = ::ReadFile(
//             port_.Get(),
//             readBuffer.data(),
//             static_cast<DWORD>(readBuffer.size()),
//             &readBytes,
//             &readOverlapped_);

//         if (!ok) {
//             const DWORD error = ::GetLastError();
//             if (error != ERROR_IO_PENDING) {
//                 break;
//             }

//             const DWORD wait = ::WaitForMultipleObjects(2, waits, FALSE, INFINITE);
//             if (wait == WAIT_OBJECT_0 + 1U) {
//                 break;
//             }

//             if (!::GetOverlappedResult(port_.Get(), &readOverlapped_, &readBytes, FALSE)) {
//                 break;
//             }
//         }

//         if (readBytes > 0 && callback_) {
//             std::vector<uint8_t> packet(readBuffer.begin(), readBuffer.begin() + readBytes);
//             callback_(packet);
//         }
//     }

//     return 0;
// }



void SerialPort::ProcessCommEvent(DWORD evtMask, std::vector<uint8_t>& readBuffer) {
    // Проверяем разные типы событий
    if (evtMask & EV_RXCHAR) {
        // Пришли данные
        ReadAllAvailableData(readBuffer);
    }
    
    if (evtMask & EV_CTS) {
        // Изменился CTS
        DWORD modemStatus = 0;
        GetModemStatus(&modemStatus);
        // Можно оповестить о смене CTS если нужно
    }
    
    if (evtMask & EV_DSR) {
        // Изменился DSR
        DWORD modemStatus = 0;
        GetModemStatus(&modemStatus);
        // Можно оповестить о смене DSR
    }
    
    // Обработка ошибок
    if (evtMask & EV_ERR) {
        DWORD errors = 0;
        COMSTAT status = {0};
        ::ClearCommError(port_.Get(), &errors, &status);
        // Логируем ошибку если нужно
    }
}

void SerialPort::ReadAllAvailableData(std::vector<uint8_t>& readBuffer) {
    DWORD errors = 0;
    COMSTAT status = {0};
    
    if (!::ClearCommError(port_.Get(), &errors, &status)) {
        return;
    }
    
    if (status.cbInQue == 0) {
        return;
    }
    
    // Увеличиваем буфер если нужно
    if (readBuffer.size() < status.cbInQue) {
        readBuffer.resize(status.cbInQue);
    }
    
    DWORD bytesRead = 0;
    ::ResetEvent(readEvent_.Get());
    readOverlapped_ = OVERLAPPED{};
    readOverlapped_.hEvent = readEvent_.Get();
    
    BOOL readOk = ::ReadFile(
        port_.Get(),
        readBuffer.data(),
        status.cbInQue,
        &bytesRead,
        &readOverlapped_);
    
    if (!readOk) {
        DWORD readError = ::GetLastError();
        if (readError == ERROR_IO_PENDING) {
            // Ждем завершения чтения (но недолго)
            HANDLE readHandles[2] = { readEvent_.Get(), shutdownEvent_.Get() };
            DWORD readWait = ::WaitForMultipleObjects(2, readHandles, FALSE, 100);
            
            if (readWait == WAIT_OBJECT_0) {
                ::GetOverlappedResult(port_.Get(), &readOverlapped_, &bytesRead, FALSE);
            }
        }
    }
    
    if (bytesRead > 0 && callback_) {
        std::vector<uint8_t> packet(
            readBuffer.begin(),
            readBuffer.begin() + bytesRead
        );
        callback_(packet);
    }
}

DWORD SerialPort::ReadThreadMain() {
    std::vector<uint8_t> readBuffer(8192); // Увеличил буфер
    HANDLE waitHandles[2] = { shutdownEvent_.Get(), waitEvent_.Get() };
    DWORD evtMask = 0;
    bool waitPending = false;

    while (running_.load()) {
        // Если нет ожидающей операции WaitCommEvent - запускаем новую
        if (!waitPending) {
            ::ResetEvent(waitEvent_.Get());
            // Важно: только устанавливаем hEvent, не перезаписываем всю структуру
            waitOverlapped_.hEvent = waitEvent_.Get();

            BOOL waitStatus = ::WaitCommEvent(port_.Get(), &evtMask, &waitOverlapped_);
            
            if (!waitStatus) {
                DWORD error = ::GetLastError();
                if (error == ERROR_IO_PENDING) {
                    waitPending = true; // Операция в процессе
                } else {
                    // Реальная ошибка
                    if (running_.load()) {
                        // Небольшая задержка чтобы не спамить ошибками
                        ::Sleep(10);
                    }
                    continue;
                }
            } else {
                // Событие произошло синхронно (редкий случай)
                waitPending = false;
                // Обрабатываем событие сразу
                ProcessCommEvent(evtMask, readBuffer);
            }
        }

        // Ждем либо завершения WaitCommEvent, либо сигнала закрытия
        if (waitPending) {
            DWORD waitResult = ::WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
            
            if (waitResult == WAIT_OBJECT_0) {
                // Сигнал закрытия
                break;
            }
            else if (waitResult == WAIT_OBJECT_0 + 1) {
                // WaitCommEvent завершился
                waitPending = false;
                
                DWORD bytesTransferred = 0;
                if (::GetOverlappedResult(port_.Get(), &waitOverlapped_, &bytesTransferred, FALSE)) {
                    ProcessCommEvent(evtMask, readBuffer);
                } else {
                    // Ошибка получения результата
                    DWORD err = ::GetLastError();
                    if (err != ERROR_OPERATION_ABORTED) {
                        // Не критичная ошибка, продолжаем работу
                    }
                }
            }
        }
    }

    return 0;
}



} // namespace serial
