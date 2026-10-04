// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#pragma once
#include "backends/crsf/crsf_backend.hpp"
#include "backends/sbus/sbus_backend.hpp"
namespace rclink::detail {
// Evaluate link state using the caller's monotonic clock and synchronization.
class Engine {
    Config config_{};
    Snapshot state_{};
    Protocol candidate_{Protocol::Crsf};
    bool locked_{false};
    bool running_{false};
    bool has_good_{false};
    bool rf_blocked_{false};
    uint32_t detected_{0};
    uint32_t healthy_{0};
    uint64_t probe_start_{0};
    uint64_t last_byte_{0};
    CrsfBackend crsf_;
    SbusBackend sbus_;
    bool ping_pending_{false};
    uint8_t ping_origin_{0xea};
    void expire_partial_frame(uint64_t now);
    void advance_probe(uint64_t now);
    void evaluate_channel_timeout(uint64_t now);
    void update_rf_freshness(uint64_t now);
    void transition(LinkState state, uint64_t now);
    void channels(const std::array<ChannelSample, 16> &samples, bool lost, bool failsafe,
                  uint64_t now);

  public:
    void start(const Config &config, uint64_t now);
    void stop(uint64_t now);
    void tick(uint64_t now);
    void feed(const uint8_t *data, size_t size, uint64_t now);
    void discontinuity();
    void overflow(uint64_t now);
    void transport_error(uint64_t now);
    Protocol candidate() const {
        return candidate_;
    }
    Backend &backend() {
        if (candidate_ == Protocol::Sbus) {
            return sbus_;
        }
        return crsf_;
    }
    bool locked() const {
        return locked_;
    }
    bool take_ping(uint8_t &origin) {
        if (!ping_pending_) {
            return false;
        }
        origin = ping_origin_;
        ping_pending_ = false;
        return true;
    }
    Snapshot snapshot(uint64_t now) {
        tick(now);
        return state_;
    }
    Diagnostics &diagnostics() {
        return state_.diagnostics;
    }
};
} // namespace rclink::detail
