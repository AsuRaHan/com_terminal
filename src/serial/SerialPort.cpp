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

    std::wstring path = L"\\\\.\\" + portName;
    HANDLE rawPort = ::CreateFileW(
        path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (rawPort == INVALID_HANDLE_VALUE) {
        return false;
    }

    port_.Reset(rawPort);

    HANDLE rawReadEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (rawReadEvent == nullptr) {
        Close();
        return false;
    }

    HANDLE rawShutdownEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (rawShutdownEvent == nullptr) {
        Close();
        return false;
    }
    HANDLE rawWriteEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (rawWriteEvent == nullptr) {
        Close();
        return false;
    }

    readEvent_.Reset(rawReadEvent);
    writeEvent_.Reset(rawWriteEvent);
    shutdownEvent_.Reset(rawShutdownEvent);
    readOverlapped_ = OVERLAPPED{};
    writeOverlapped_ = OVERLAPPED{};
    readOverlapped_.hEvent = readEvent_.Get();
    writeOverlapped_.hEvent = writeEvent_.Get();

    if (!ConfigurePort(port_.Get(), settings)) {
        Close();
        return false;
    }

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
        if (port_.IsValid()) {
            ::CancelIoEx(port_.Get(), nullptr);
        }
        if (shutdownEvent_.IsValid()) {
            ::SetEvent(shutdownEvent_.Get());
        }
        if (threadHandle_.IsValid()) {
            ::WaitForSingleObject(threadHandle_.Get(), 3000);
        }
    }

    threadHandle_.Reset();
    readEvent_.Reset();
    writeEvent_.Reset();
    shutdownEvent_.Reset();
    port_.Reset();
    readOverlapped_ = OVERLAPPED{};
    writeOverlapped_ = OVERLAPPED{};
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

DWORD SerialPort::ReadThreadMain() {
    std::array<uint8_t, 1024> readBuffer{};
    HANDLE waits[2] = {readEvent_.Get(), shutdownEvent_.Get()};

    while (running_.load()) {
        DWORD readBytes = 0;
        ::ResetEvent(readEvent_.Get());

        const BOOL ok = ::ReadFile(
            port_.Get(),
            readBuffer.data(),
            static_cast<DWORD>(readBuffer.size()),
            &readBytes,
            &readOverlapped_);

        if (!ok) {
            const DWORD error = ::GetLastError();
            if (error != ERROR_IO_PENDING) {
                break;
            }

            const DWORD wait = ::WaitForMultipleObjects(2, waits, FALSE, INFINITE);
            if (wait == WAIT_OBJECT_0 + 1U) {
                break;
            }

            if (!::GetOverlappedResult(port_.Get(), &readOverlapped_, &readBytes, FALSE)) {
                break;
            }
        }

        if (readBytes > 0 && callback_) {
            std::vector<uint8_t> packet(readBuffer.begin(), readBuffer.begin() + readBytes);
            callback_(packet);
        }
    }

    return 0;
}

} // namespace serial
