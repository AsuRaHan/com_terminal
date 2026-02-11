#include "core/BufferPool.h"

#include <algorithm>

namespace core {

BufferPool::BufferPool() : head_(0), tail_(0) {
    ::InitializeCriticalSection(&cs_);
}

BufferPool::~BufferPool() {
    ::DeleteCriticalSection(&cs_);
}

bool BufferPool::Write(const char* buffer, DWORD size) {
    if (buffer == nullptr || size == 0) {
        return false;
    }

    ::EnterCriticalSection(&cs_);

    for (DWORD i = 0; i < size; ++i) {
        const DWORD next = (head_ + 1U) % kCapacity;
        if (next == tail_) {
            DiscardOldest();
        }
        data_[head_] = buffer[i];
        head_ = next;
    }

    ::LeaveCriticalSection(&cs_);
    return true;
}

bool BufferPool::Read(char* buffer, DWORD size, DWORD* readBytes) {
    return ReadInternal(buffer, size, readBytes, true);
}

bool BufferPool::Peek(char* buffer, DWORD size, DWORD* readBytes) {
    return ReadInternal(buffer, size, readBytes, false);
}

void BufferPool::DiscardOldest() {
    if (tail_ != head_) {
        tail_ = (tail_ + 1U) % kCapacity;
    }
}

bool BufferPool::ReadInternal(char* buffer, DWORD size, DWORD* readBytes, bool consume) {
    if (buffer == nullptr || size == 0 || readBytes == nullptr) {
        return false;
    }

    *readBytes = 0;

    ::EnterCriticalSection(&cs_);

    DWORD tail = tail_;
    const DWORD available = SizeLocked();
    const DWORD toRead = std::min(size, available);

    for (DWORD i = 0; i < toRead; ++i) {
        buffer[i] = data_[tail];
        tail = (tail + 1U) % kCapacity;
    }

    if (consume) {
        tail_ = tail;
    }

    *readBytes = toRead;

    ::LeaveCriticalSection(&cs_);
    return true;
}

DWORD BufferPool::SizeLocked() const noexcept {
    if (head_ >= tail_) {
        return head_ - tail_;
    }
    return kCapacity - (tail_ - head_);
}

} // namespace core