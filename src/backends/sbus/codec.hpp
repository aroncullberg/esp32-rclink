// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#pragma once
#include "rclink/types.hpp"

namespace rclink::detail::sbus {
uint16_t unpack11(const uint8_t *packed, size_t channel);
uint16_t normalize(uint16_t raw, Calibration calibration);
} // namespace rclink::detail::sbus
