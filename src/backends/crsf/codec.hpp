// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#pragma once
#include "rclink/types.hpp"

namespace rclink::detail::crsf {
uint16_t unpack11(const uint8_t *packed, size_t channel);
uint16_t normalize(uint16_t raw, Calibration calibration);
struct Packet {
    uint8_t type{0};
    std::array<uint8_t, 60> payload{};
    size_t size{0};
};
uint8_t crc(const uint8_t *data, size_t size);
size_t frame(uint8_t type, const uint8_t *payload, size_t size, std::array<uint8_t, 64> &out);
} // namespace rclink::detail::crsf
