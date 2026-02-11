#pragma once

#include <windows.h>

namespace core {

class SafeHandle final {
public:
    SafeHandle() noexcept;
    explicit SafeHandle(HANDLE handle) noexcept;
    ~SafeHandle() noexcept;

    SafeHandle(const SafeHandle&) = delete;
    SafeHandle& operator=(const SafeHandle&) = delete;

    SafeHandle(SafeHandle&& other) noexcept;
    SafeHandle& operator=(SafeHandle&& other) noexcept;

    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] HANDLE Get() const noexcept;

    HANDLE Release() noexcept;
    void Reset(HANDLE handle = INVALID_HANDLE_VALUE) noexcept;

private:
    HANDLE handle_;
};

} // namespace core