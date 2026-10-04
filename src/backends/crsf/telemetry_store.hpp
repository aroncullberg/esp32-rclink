// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#pragma once
#include "rclink/telemetry.hpp"
#include <array>
namespace rclink::detail::crsf {
// Keep the latest sample for each frame type and source in a bounded queue.
template <size_t Capacity> class TelemetryStore {
    struct Slot {
        bool used{false};
        telemetry::Encoded packet{};
        uint64_t submitted{0};
        uint64_t sent{0};
        bool was_sent{false};
        uint32_t interval_ms{0};
        uint32_t ttl_ms{0};
    };
    std::array<Slot, Capacity> slots_{};
    size_t cursor_{0};

  public:
    void clear() {
        slots_ = {};
        cursor_ = 0;
    }
    bool put(const telemetry::Encoded &packet, telemetry::Options options, uint64_t now) {
        Slot *free = nullptr;
        for (auto &slot : slots_) {
            if (slot.used && now - slot.submitted >= uint64_t(slot.ttl_ms) * 1000) {
                slot.used = false;
            }
            if (slot.used && slot.packet.key == packet.key) {
                slot.packet = packet;
                slot.submitted = now;
                slot.interval_ms = options.interval_ms;
                slot.ttl_ms = options.ttl_ms;
                return true;
            }
            if (!slot.used && !free) {
                free = &slot;
            }
        }
        if (!free) {
            return false;
        }
        free->used = true;
        free->packet = packet;
        free->submitted = now;
        free->sent = 0;
        free->was_sent = false;
        free->interval_ms = options.interval_ms;
        free->ttl_ms = options.ttl_ms;
        return true;
    }
    bool take(uint64_t now, telemetry::Encoded &packet, uint64_t &expired) {
        for (size_t n = 0; n < Capacity; ++n) {
            const size_t i = (cursor_ + n) % Capacity;
            auto &slot = slots_[i];
            if (!slot.used) {
                continue;
            }
            if (now - slot.submitted >= uint64_t(slot.ttl_ms) * 1000) {
                slot.used = false;
                ++expired;
                continue;
            }
            if (slot.was_sent && now - slot.sent < uint64_t(slot.interval_ms) * 1000) {
                continue;
            }
            packet = slot.packet;
            slot.sent = now;
            slot.was_sent = true;
            cursor_ = (i + 1) % Capacity;
            // Remove selected MAVLink chunks because repeating them can corrupt reassembly.
            if (packet.bytes[2] == 0xaa) {
                slot.used = false;
            }
            return true;
        }
        return false;
    }
};
} // namespace rclink::detail::crsf
