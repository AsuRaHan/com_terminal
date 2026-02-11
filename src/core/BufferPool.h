#pragma once

#include <windows.h>

#include <array>

namespace core {

class BufferPool final {
public:
    BufferPool();
    ~BufferPool();

    BufferPool(const BufferPool&) = delete;
    BufferPool& operator=(const BufferPool&) = delete;

    bool Write(const char* buffer, DWORD size);
    bool Read(char* buffer, DWORD size, DWORD* readBytes);
    bool Peek(char* buffer, DWORD size, DWORD* readBytes);
    void DiscardOldest();

private:
    static constexpr DWORD kCapacity = 65536;

    bool ReadInternal(char* buffer, DWORD size, DWORD* readBytes, bool consume);
    [[nodiscard]] DWORD SizeLocked() const noexcept;

    CRITICAL_SECTION cs_;
    std::array<char, kCapacity> data_;
    DWORD head_;
    DWORD tail_;
};

} // namespace core