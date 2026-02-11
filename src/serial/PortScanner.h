#pragma once

#include <string>
#include <vector>

namespace serial {

struct PortInfo {
    std::wstring portName;
    std::wstring friendlyName;
};

class PortScanner final {
public:
    static std::vector<PortInfo> Scan();
};

} // namespace serial