#include "core/SafeHandle.h"

namespace core {

SafeHandle::SafeHandle() noexcept : handle_(INVALID_HANDLE_VALUE) {}

SafeHandle::SafeHandle(HANDLE handle) noexcept : handle_(handle) {}

SafeHandle::~SafeHandle() noexcept {
    Reset();
}

SafeHandle::SafeHandle(SafeHandle&& other) noexcept : handle_(other.Release()) {}

SafeHandle& SafeHandle::operator=(SafeHandle&& other) noexcept {
    if (this != &other) {
        Reset(other.Release());
    }
    return *this;
}

bool SafeHandle::IsValid() const noexcept {
    return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE;
}

HANDLE SafeHandle::Get() const noexcept {
    return handle_;
}

HANDLE SafeHandle::Release() noexcept {
    HANDLE current = handle_;
    handle_ = INVALID_HANDLE_VALUE;
    return current;
}

void SafeHandle::Reset(HANDLE handle) noexcept {
    if (IsValid()) {
        ::CloseHandle(handle_);
    }
    handle_ = handle;
}

} // namespace core