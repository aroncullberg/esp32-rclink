// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#include "codec.hpp"

namespace rclink::detail::sbus {
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
} // namespace rclink::detail::sbus
