// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#include "sbus_backend.hpp"
#include "codec.hpp"
#include <cstring>
namespace rclink::detail {
bool SbusBackend::feed(uint8_t byte, uint64_t, BackendEvent &event) {
    if (!used_ && byte != 0x0f) {
        return false;
    }
    buffer_[used_++] = byte;
    if (used_ < 25) {
        return false;
    }
    const uint8_t footer = buffer_[24];
    const bool standard_footer = footer == 0;
    const bool sbus2_footer = footer == 0x04 || footer == 0x14 || footer == 0x24 || footer == 0x34;
    const bool reserved_flags_set = (buffer_[23] & 0xf0) != 0;
    if (reserved_flags_set || !(standard_footer || sbus2_footer)) {
        ++diagnostics_.malformed;
        size_t next = 1;
        while (next < 25 && buffer_[next] != 0x0f) {
            ++next;
        }
        used_ = 25 - next;
        std::memmove(buffer_.data(), buffer_.data() + next, used_);
        return false;
    }
    event = {};
    for (size_t i = 0; i < 16; ++i) {
        ChannelSample &channel = event.channels[i];
        channel.raw = sbus::unpack11(buffer_.data() + 1, i);
        channel.scaled = sbus::normalize(channel.raw, calibration_);
    }
    event.kind = BackendEvent::Kind::Channels;
    event.digital17 = buffer_[23] & 1;
    event.digital18 = buffer_[23] & 2;
    event.frame_lost = buffer_[23] & 4;
    event.failsafe = buffer_[23] & 8;
    used_ = 0;
    return true;
}
} // namespace rclink::detail
