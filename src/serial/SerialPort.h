#pragma once

#include <windows.h>

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "core/SafeHandle.h"

namespace serial {

enum class ParityMode {
    None,
    Odd,
    Even,
    Mark,
    Space
};

enum class StopBitsMode {
    One,
    OnePointFive,
    Two
};

enum class FlowControlMode {
    None,
    Hardware,
    Software
};

struct PortSettings {
    DWORD baudRate;
    BYTE dataBits;
    ParityMode parity;
    StopBitsMode stopBits;
    FlowControlMode flowControl;
    bool rts;
    bool dtr;
};

class SerialPort final {
public:
    using DataCallback = std::function<void(const std::vector<uint8_t>&)>;

    SerialPort();
    ~SerialPort();

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    bool Open(const std::wstring& portName, const PortSettings& settings);
    void Close();
    bool IsOpen() const noexcept;
    bool Write(const uint8_t* data, DWORD size, DWORD* writtenBytes);
    bool GetModemStatus(DWORD* modemStatus);
    bool SetRts(bool enabled);
    bool SetDtr(bool enabled);

    void SetDataCallback(DataCallback callback);

private:
    static DWORD WINAPI ReadThreadProc(LPVOID param);
    DWORD ReadThreadMain();

    core::SafeHandle port_;
    core::SafeHandle readEvent_;
    core::SafeHandle writeEvent_;
    core::SafeHandle shutdownEvent_;
    core::SafeHandle threadHandle_;

    OVERLAPPED readOverlapped_;
    OVERLAPPED writeOverlapped_;
    std::atomic<bool> running_;
    DataCallback callback_;
};

} // namespace serial
