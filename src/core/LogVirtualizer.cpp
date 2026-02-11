#include "core/LogVirtualizer.h"

#include <algorithm>
#include <filesystem>
#include <system_error>

namespace core {

LogVirtualizer::LogVirtualizer(
    std::size_t maxBufferedLines,
    std::uint64_t rewriteLineThreshold,
    std::size_t rewriteByteThreshold)
    : maxBufferedLines_(maxBufferedLines),
      rewriteLineThreshold_(rewriteLineThreshold),
      rewriteByteThreshold_(rewriteByteThreshold),
      bufferedChars_(0),
      displayedLinesSinceRewrite_(0),
      displayedCharsSinceRewrite_(0) {
}

LogVirtualizer::~LogVirtualizer() = default;

bool LogVirtualizer::Initialize(const std::wstring& logDirectory) {
    std::error_code ec;
    std::filesystem::create_directories(logDirectory, ec);
    if (ec) {
        return false;
    }

    SYSTEMTIME st{};
    ::GetLocalTime(&st);

    wchar_t fileName[64] = {};
    ::swprintf_s(
        fileName,
        L"log_%04u%02u%02u_%02u%02u%02u.txt",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute,
        st.wSecond);

    const std::filesystem::path path = std::filesystem::path(logDirectory) / fileName;
    sessionFilePath_ = path.wstring();

    file_.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!file_.is_open()) {
        return false;
    }

    static constexpr unsigned char kUtf8Bom[] = {0xEFU, 0xBBU, 0xBFU};
    file_.write(reinterpret_cast<const char*>(kUtf8Bom), sizeof(kUtf8Bom));
    file_.flush();
    return true;
}

bool LogVirtualizer::AppendLine(const std::wstring& line, COLORREF color, bool persistToDisk) {
    bufferedLines_.push_back(VirtualLogLine{line, color});
    bufferedChars_ += line.size();

    while (bufferedLines_.size() > maxBufferedLines_) {
        bufferedChars_ -= bufferedLines_.front().text.size();
        bufferedLines_.pop_front();
    }

    ++displayedLinesSinceRewrite_;
    displayedCharsSinceRewrite_ += line.size() * sizeof(wchar_t);

    if (!persistToDisk) {
        return true;
    }

    return AppendUtf8LineToDisk(line);
}

bool LogVirtualizer::ShouldRewrite() const noexcept {
    return displayedLinesSinceRewrite_ >= rewriteLineThreshold_ ||
           displayedCharsSinceRewrite_ >= rewriteByteThreshold_;
}

std::vector<VirtualLogLine> LogVirtualizer::SnapshotBuffer() const {
    return std::vector<VirtualLogLine>(bufferedLines_.begin(), bufferedLines_.end());
}

void LogVirtualizer::MarkRewriteDone() noexcept {
    displayedLinesSinceRewrite_ = static_cast<std::uint64_t>(bufferedLines_.size());
    displayedCharsSinceRewrite_ = bufferedChars_ * sizeof(wchar_t);
}

std::wstring LogVirtualizer::SessionFilePath() const {
    return sessionFilePath_;
}

bool LogVirtualizer::AppendUtf8LineToDisk(const std::wstring& line) {
    if (!file_.is_open()) {
        return false;
    }

    const std::string utf8 = ToUtf8(line);
    file_.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
    file_.flush();
    return static_cast<bool>(file_);
}

std::string LogVirtualizer::ToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }

    const int bytes = ::WideCharToMultiByte(
        CP_UTF8,
        0,
        text.c_str(),
        static_cast<int>(text.size()),
        nullptr,
        0,
        nullptr,
        nullptr);
    if (bytes <= 0) {
        return {};
    }

    std::string utf8;
    utf8.resize(static_cast<std::size_t>(bytes));

    ::WideCharToMultiByte(
        CP_UTF8,
        0,
        text.c_str(),
        static_cast<int>(text.size()),
        utf8.data(),
        bytes,
        nullptr,
        nullptr);

    return utf8;
}

} // namespace core
