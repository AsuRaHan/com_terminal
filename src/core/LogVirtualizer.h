#pragma once

#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <fstream>
#include <string>
#include <vector>

namespace core {

struct VirtualLogLine {
    std::wstring text;
    COLORREF color;
};

class LogVirtualizer final {
public:
    LogVirtualizer(std::size_t maxBufferedLines, std::uint64_t rewriteLineThreshold, std::size_t rewriteByteThreshold);
    ~LogVirtualizer();

    LogVirtualizer(const LogVirtualizer&) = delete;
    LogVirtualizer& operator=(const LogVirtualizer&) = delete;

    bool Initialize(const std::wstring& logDirectory);
    bool AppendLine(const std::wstring& line, COLORREF color, bool persistToDisk);

    [[nodiscard]] bool ShouldRewrite() const noexcept;
    [[nodiscard]] std::vector<VirtualLogLine> SnapshotBuffer() const;
    void MarkRewriteDone() noexcept;

    [[nodiscard]] std::wstring SessionFilePath() const;

private:
    bool AppendUtf8LineToDisk(const std::wstring& line);
    static std::string ToUtf8(const std::wstring& text);

    std::size_t maxBufferedLines_;
    std::uint64_t rewriteLineThreshold_;
    std::size_t rewriteByteThreshold_;

    std::deque<VirtualLogLine> bufferedLines_;
    std::size_t bufferedChars_;

    std::uint64_t displayedLinesSinceRewrite_;
    std::size_t displayedCharsSinceRewrite_;

    std::ofstream file_;
    std::wstring sessionFilePath_;
};

} // namespace core
