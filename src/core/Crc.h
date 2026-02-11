#pragma once

#include <cstddef>
#include <cstdint>

namespace core {

uint8_t Crc8Dallas(const uint8_t* data, std::size_t size) noexcept;
uint16_t Crc16Ibm(const uint8_t* data, std::size_t size) noexcept;
uint32_t Crc32IsoHdlc(const uint8_t* data, std::size_t size) noexcept;

} // namespace core