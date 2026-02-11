#include "serial/PortScanner.h"

#include <windows.h>
#include <setupapi.h>
#include <devguid.h>

#include <algorithm>
#include <set>
#include <regex>

namespace {

constexpr GUID kGuidDevinterfaceComport = {
    0x86E0D1E0, 0x8089, 0x11D0, {0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73}
};

std::wstring ExtractComName(const std::wstring& friendlyName) {
    const std::wregex pattern(LR"((COM\d+))", std::regex_constants::icase);
    std::wsmatch match;
    if (std::regex_search(friendlyName, match, pattern) && match.size() > 1) {
        return match[1].str();
    }
    return L"";
}

std::wstring ReadFriendlyName(HDEVINFO deviceInfoSet, SP_DEVINFO_DATA* deviceData) {
    DWORD dataType = 0;
    DWORD requiredSize = 0;
    ::SetupDiGetDeviceRegistryPropertyW(
        deviceInfoSet,
        deviceData,
        SPDRP_FRIENDLYNAME,
        &dataType,
        nullptr,
        0,
        &requiredSize);

    if (requiredSize == 0) {
        return L"";
    }

    std::wstring value;
    value.resize(requiredSize / sizeof(wchar_t));
    if (!::SetupDiGetDeviceRegistryPropertyW(
            deviceInfoSet,
            deviceData,
            SPDRP_FRIENDLYNAME,
            &dataType,
            reinterpret_cast<PBYTE>(value.data()),
            requiredSize,
            nullptr)) {
        return L"";
    }

    while (!value.empty() && value.back() == L'\0') {
        value.pop_back();
    }

    return value;
}

void EnumerateByPortsClass(std::vector<serial::PortInfo>* ports) {
    const HDEVINFO infoSet = ::SetupDiGetClassDevsW(
        &GUID_DEVCLASS_PORTS,
        nullptr,
        nullptr,
        DIGCF_PRESENT);

    if (infoSet == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD index = 0;
    SP_DEVINFO_DATA deviceData{};
    deviceData.cbSize = sizeof(deviceData);

    while (::SetupDiEnumDeviceInfo(infoSet, index, &deviceData) == TRUE) {
        const std::wstring friendly = ReadFriendlyName(infoSet, &deviceData);
        const std::wstring portName = ExtractComName(friendly);
        if (!portName.empty()) {
            ports->push_back(serial::PortInfo{portName, friendly});
        }
        ++index;
    }

    ::SetupDiDestroyDeviceInfoList(infoSet);
}

void EnumerateByInterfaceGuid(std::vector<serial::PortInfo>* ports) {
    const HDEVINFO infoSet = ::SetupDiGetClassDevsW(
        &kGuidDevinterfaceComport,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (infoSet == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD index = 0;
    SP_DEVINFO_DATA deviceData{};
    deviceData.cbSize = sizeof(deviceData);

    while (::SetupDiEnumDeviceInfo(infoSet, index, &deviceData) == TRUE) {
        const std::wstring friendly = ReadFriendlyName(infoSet, &deviceData);
        const std::wstring portName = ExtractComName(friendly);
        if (!portName.empty()) {
            ports->push_back(serial::PortInfo{portName, friendly});
        }
        ++index;
    }

    ::SetupDiDestroyDeviceInfoList(infoSet);
}

void EnumerateByQueryDosDevice(std::vector<serial::PortInfo>* ports) {
    wchar_t target[1024] = {};
    for (int i = 1; i <= 256; ++i) {
        wchar_t name[16] = {};
        ::swprintf_s(name, L"COM%d", i);
        if (::QueryDosDeviceW(name, target, static_cast<DWORD>(_countof(target))) != 0) {
            ports->push_back(serial::PortInfo{name, name});
        }
    }
}

void RemoveDuplicates(std::vector<serial::PortInfo>* ports) {
    std::set<std::wstring> seen;
    std::vector<serial::PortInfo> unique;
    unique.reserve(ports->size());

    for (const auto& port : *ports) {
        if (seen.insert(port.portName).second) {
            unique.push_back(port);
        }
    }

    *ports = std::move(unique);
}

} // namespace

namespace serial {

std::vector<PortInfo> PortScanner::Scan() {
    std::vector<PortInfo> ports;
    EnumerateByPortsClass(&ports);
    EnumerateByInterfaceGuid(&ports);
    EnumerateByQueryDosDevice(&ports);
    RemoveDuplicates(&ports);

    std::sort(ports.begin(), ports.end(), [](const PortInfo& lhs, const PortInfo& rhs) {
        return lhs.portName < rhs.portName;
    });

    return ports;
}

} // namespace serial
