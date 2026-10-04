// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
// Stream parsing is adapted from AlfredoCRSF as documented in README.md.
#include "crsf_backend.hpp"
#include <cstring>
namespace rclink::detail {
void CrsfBackend::shift(size_t n) {
    used_ -= n;
    std::memmove(buffer_.data(), buffer_.data() + n, used_);
}

bool CrsfBackend::parse(uint8_t byte, crsf::Packet &packet) {
    if (used_ == buffer_.size()) {
        shift(1);
        ++diagnostics_.malformed;
    }
    buffer_[used_++] = byte;
    while (used_ >= 2) {
        // Accept the flight controller, broadcast and receiver addresses as sync bytes.
        const uint8_t sync = buffer_[0];
        const uint8_t length = buffer_[1];
        const bool valid_sync = sync == 0xc8 || sync == 0x00 || sync == 0xec;
        const bool valid_length = length >= 2 && length <= 62;
        if (!valid_sync || !valid_length) {
            shift(1);
            ++diagnostics_.malformed;
            continue;
        }
        const size_t total = buffer_[1] + 2;
        if (used_ < total) {
            return false;
        }
        if (crsf::crc(buffer_.data() + 2, total - 3) != buffer_[total - 1]) {
            ++diagnostics_.crc_errors;
            shift(1);
            continue;
        }
        packet.type = buffer_[2];
        packet.size = total - 4;
        std::memcpy(packet.payload.data(), buffer_.data() + 3, packet.size);
        shift(total);
        return true;
    }
    return false;
}

bool CrsfBackend::feed(uint8_t byte, uint64_t now, BackendEvent &event) {
    crsf::Packet packet;
    if (!parse(byte, packet)) {
        return false;
    }
    if (packet.type == 0x16 && packet.size >= 22) {
        event = {};
        event.kind = BackendEvent::Kind::Channels;
        for (size_t c = 0; c < 16; ++c) {
            ChannelSample &channel = event.channels[c];
            channel.raw = crsf::unpack11(packet.payload.data(), c);
            channel.scaled = crsf::normalize(channel.raw, calibration_);
        }
        event.has_status = packet.size >= 23;
        event.status = packet.size >= 23 ? packet.payload[22] : 0;
        return true;
    }
    if (packet.type == 0x14 && packet.size >= 10) {
        event = {};
        event.kind = BackendEvent::Kind::RfStatistics;
        event.rf.available = true;
        event.rf.timestamp_us = now;
        event.rf.rssi1 = packet.payload[0];
        event.rf.rssi2 = packet.payload[1];
        event.rf.quality = packet.payload[2];
        event.rf.snr = static_cast<int8_t>(packet.payload[3]);
        event.rf.rf_mode = packet.payload[5];
        event.rf.tx_power = packet.payload[6];
        return true;
    }
    if (packet.type == 0x1d && packet.size >= 6) {
        event = {};
        event.kind = BackendEvent::Kind::RfStatistics;
        event.rf.available = true;
        event.rf.timestamp_us = now;
        event.rf.rssi1 = packet.payload[0];
        event.rf.quality = packet.payload[2];
        event.rf.snr = static_cast<int8_t>(packet.payload[3]);
        if (packet.size >= 8) {
            event.rf.rssi2 = packet.payload[6];
        }
        return true;
    }
    if (packet.type == 0x28 && packet.size >= 2) {
        const uint8_t destination = packet.payload[0];
        if (destination == 0 || destination == 0xc8) {
            event = {};
            event.kind = BackendEvent::Kind::Discovery;
            event.discovery_origin = packet.payload[1];
            return true;
        }
    }
    if (packet.type == 0x16 || packet.type == 0x14 || packet.type == 0x1d) {
        ++diagnostics_.malformed;
    }
    return false;
}
} // namespace rclink::detail
