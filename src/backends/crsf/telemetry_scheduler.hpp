// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#pragma once

#include "rclink/types.hpp"
#include "telemetry_store.hpp"
#include <cstring>

namespace rclink::detail::crsf {

template <size_t Capacity> class TelemetryScheduler {
    TelemetryStore<Capacity> samples_;
    uint64_t last_tx_us_{0};
    uint64_t last_heartbeat_us_{0};
    bool tx_started_{false};
    bool last_was_heartbeat_{false};

  public:
    void clear() {
        samples_.clear();
        last_tx_us_ = 0;
        last_heartbeat_us_ = 0;
        tx_started_ = false;
        last_was_heartbeat_ = false;
    }

    bool put(const telemetry::Encoded &packet, telemetry::Options options, uint64_t now) {
        return samples_.put(packet, options, now);
    }

    void respond_to_ping(uint8_t origin, const Config &config, uint64_t now) {
        telemetry::DeviceInfo info;
        info.destination = origin;
        std::strncpy(info.name.data(), "esp32-rclink", info.name.size() - 1);

        telemetry::Encoded response;
        telemetry::encode(info, response);
        const telemetry::Options options{config.telemetry_interval_ms, config.telemetry_ttl_ms};
        samples_.put(response, options, now);
    }

    bool take(uint64_t now, const Config &config, telemetry::Encoded &packet, uint64_t &dropped) {
        const uint64_t tx_age_us = now - last_tx_us_;
        const uint64_t tx_interval_us = uint64_t(config.telemetry_interval_ms) * 1000;
        if (tx_started_ && tx_age_us < tx_interval_us) {
            return false;
        }

        const uint64_t heartbeat_age_us = now - last_heartbeat_us_;
        const uint64_t heartbeat_interval_us = uint64_t(config.heartbeat_ms) * 1000;
        const bool heartbeat_enabled = config.heartbeat_ms != 0;
        const bool heartbeat_due = heartbeat_age_us >= heartbeat_interval_us;
        bool send = false;
        if (heartbeat_enabled && heartbeat_due && !last_was_heartbeat_) {
            telemetry::encode(telemetry::Heartbeat{}, packet);
            last_heartbeat_us_ = now;
            last_was_heartbeat_ = true;
            send = true;
        } else {
            send = samples_.take(now, packet, dropped);
            last_was_heartbeat_ = false;
        }

        if (send) {
            last_tx_us_ = now;
            tx_started_ = true;
        }
        return send;
    }
};

} // namespace rclink::detail::crsf
