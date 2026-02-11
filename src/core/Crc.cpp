#include "core/Crc.h"

namespace {

uint8_t Reflect8(uint8_t value) noexcept {
    uint8_t result = 0;
    for (int i = 0; i < 8; ++i) {
        result <<= 1U;
        result |= (value & 1U);
        value >>= 1U;
    }
    return result;
}

uint16_t Reflect16(uint16_t value) noexcept {
    uint16_t result = 0;
    for (int i = 0; i < 16; ++i) {
        result <<= 1U;
        result |= static_cast<uint16_t>(value & 1U);
        value >>= 1U;
    }
    return result;
}

uint32_t Reflect32(uint32_t value) noexcept {
    uint32_t result = 0;
    for (int i = 0; i < 32; ++i) {
        result <<= 1U;
        result |= (value & 1U);
        value >>= 1U;
    }
    return result;
}

} // namespace

namespace core {

uint8_t Crc8Dallas(const uint8_t* data, std::size_t size) noexcept {
    if (data == nullptr || size == 0U) {
        return 0U;
    }

    uint8_t crc = 0x00U;
    for (std::size_t i = 0; i < size; ++i) {
        uint8_t current = Reflect8(data[i]);
        crc ^= current;
        for (int bit = 0; bit < 8; ++bit) {
            const bool msb = (crc & 0x80U) != 0U;
            crc = static_cast<uint8_t>(crc << 1U);
            if (msb) {
                crc ^= 0x31U;
            }
        }
    }
    return Reflect8(crc);
}

uint16_t Crc16Ibm(const uint8_t* data, std::size_t size) noexcept {
    if (data == nullptr || size == 0U) {
        return 0U;
    }

    uint16_t crc = 0x0000U;
    for (std::size_t i = 0; i < size; ++i) {
        uint16_t current = Reflect8(data[i]);
        crc ^= static_cast<uint16_t>(current << 8U);
        for (int bit = 0; bit < 8; ++bit) {
            const bool msb = (crc & 0x8000U) != 0U;
            crc <<= 1U;
            if (msb) {
                crc ^= 0x8005U;
            }
        }
    }
    return Reflect16(crc);
}

uint32_t Crc32IsoHdlc(const uint8_t* data, std::size_t size) noexcept {
    if (data == nullptr || size == 0U) {
        return 0U;
    }

    uint32_t crc = 0xFFFFFFFFU;
    for (std::size_t i = 0; i < size; ++i) {
        uint32_t current = Reflect8(data[i]);
        crc ^= (current << 24U);
        for (int bit = 0; bit < 8; ++bit) {
            const bool msb = (crc & 0x80000000U) != 0U;
            crc <<= 1U;
            if (msb) {
                crc ^= 0x04C11DB7U;
            }
        }
    }

    crc = Reflect32(crc);
    return crc ^ 0xFFFFFFFFU;
}

} // namespace core