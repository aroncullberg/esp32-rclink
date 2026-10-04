// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#include "codec.hpp"
// CRSF framing is adapted from AlfredoCRSF as documented in README.md.
#include "third_party/alfredocrsf/crc8.h"
#include <cstring>

namespace rclink::detail::crsf {
uint8_t crc(const uint8_t *data, size_t size) {
    return Crc8::calcPoly(data, static_cast<uint8_t>(size), 0xd5);
}

uint16_t unpack11(const uint8_t *packed, size_t channel) {
    uint16_t value = 0;
    for (size_t i = 0; i < 11; ++i) {
        const size_t bit = channel * 11 + i;
        value |= ((packed[bit / 8] >> (bit % 8)) & 1u) << i;
    }
    return value;
}

uint16_t normalize(uint16_t raw, Calibration calibration) {
    const uint16_t clamped = std::clamp(raw, calibration.minimum, calibration.maximum);
    if (clamped <= calibration.center) {
        const uint32_t offset = clamped - calibration.minimum;
        const uint32_t travel = calibration.center - calibration.minimum;
        return (offset * 1000 + travel / 2) / travel;
    }

    const uint32_t offset = clamped - calibration.center;
    const uint32_t travel = calibration.maximum - calibration.center;
    return 1000 + (offset * 1000 + travel / 2) / travel;
}

size_t frame(uint8_t type, const uint8_t *payload, size_t size, std::array<uint8_t, 64> &out) {
    if (size > 60 || (!payload && size)) {
        return 0;
    }
    out[0] = 0xc8;
    out[1] = static_cast<uint8_t>(size + 2);
    out[2] = type;
    if (size) {
        std::memcpy(out.data() + 3, payload, size);
    }
    out[size + 3] = crc(out.data() + 2, size + 1);
    return size + 4;
}
} // namespace rclink::detail::crsf
