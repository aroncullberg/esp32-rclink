// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#include "rclink/types.hpp"
namespace rclink {
const char *name(Protocol p) noexcept {
    switch (p) {
    case Protocol::Auto:
        return "auto";
    case Protocol::Sbus:
        return "sbus";
    case Protocol::Crsf:
        return "crsf";
    }
    return "unknown";
}
const char *name(LinkState state) noexcept {
    switch (state) {
    case LinkState::Stopped:
        return "Stopped";
    case LinkState::Detecting:
        return "Detecting";
    case LinkState::NoData:
        return "NoData";
    case LinkState::Recovering:
        return "Recovering";
    case LinkState::Active:
        return "Active";
    case LinkState::ChannelTimeout:
        return "ChannelTimeout";
    case LinkState::ReceiverFailsafe:
        return "ReceiverFailsafe";
    case LinkState::RfLoss:
        return "RfLoss";
    case LinkState::TransportError:
        return "TransportError";
    }
    return "unknown";
}

bool Config::valid() const noexcept {
    const bool auto_protocol = protocol == Protocol::Auto;
    const bool sbus_protocol = protocol == Protocol::Sbus;
    const bool crsf_protocol = protocol == Protocol::Crsf;
    const bool known_protocol = auto_protocol || sbus_protocol || crsf_protocol;
    if (!known_protocol) {
        return false;
    }
    if (uart < 0 || tx_pin < 0 || rx_pin < 0 || tx_pin == rx_pin) {
        return false;
    }
    if (crsf_baud < 100000 || crsf_baud > 1000000) {
        return false;
    }
    if (timeout_ms == 0 || probe_ms == 0 || parser_gap_ms == 0) {
        return false;
    }
    if (evaluation_ms == 0 || evaluation_ms > timeout_ms) {
        return false;
    }
    if (recovery_frames == 0 || recovery_frames > 1000) {
        return false;
    }
    if (detection_frames == 0 || detection_frames > 1000) {
        return false;
    }
    if (protocol == Protocol::Auto && detection_frames < 2) {
        return false;
    }

    const auto valid_calibration = [](Calibration calibration) {
        if (calibration.minimum >= calibration.center) {
            return false;
        }
        if (calibration.center >= calibration.maximum) {
            return false;
        }
        return calibration.maximum <= 2047;
    };
    if (!valid_calibration(sbus) || !valid_calibration(crsf)) {
        return false;
    }
    if (low_threshold >= high_threshold || high_threshold > 2000) {
        return false;
    }
    if (deadband >= 1000 || rf_timeout_ms == 0) {
        return false;
    }
    if (telemetry_interval_ms == 0 || telemetry_ttl_ms < telemetry_interval_ms) {
        return false;
    }
    for (const uint16_t value : fallback) {
        if (value > 2000) {
            return false;
        }
    }
    return true;
}
} // namespace rclink
