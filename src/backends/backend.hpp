// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#pragma once
#include "rclink/types.hpp"
namespace rclink::detail {
struct UartSettings {
    uint32_t baud;
    bool even_parity;
    bool two_stop_bits;
    bool inverted;
};
struct ChannelSample {
    uint16_t raw{0};
    uint16_t scaled{0};
};

struct BackendEvent {
    enum class Kind {
        Channels,
        RfStatistics,
        Discovery
    };
    Kind kind{Kind::Channels};
    std::array<ChannelSample, 16> channels{};
    bool frame_lost{false};
    bool failsafe{false};
    bool digital17{false};
    bool digital18{false};
    bool has_status{false};
    uint8_t status{0};
    RfStatistics rf;
    uint8_t discovery_origin{0xea};
};
struct BackendDiagnostics {
    uint64_t malformed{0};
    uint64_t crc_errors{0};
};
// Decode received bytes into channel samples and receiver status.
class Backend {
  public:
    virtual ~Backend() = default;
    virtual Protocol protocol() const noexcept = 0;
    virtual UartSettings settings(const Config &config) const noexcept = 0;
    virtual void configure(const Config &config) noexcept = 0;
    virtual void reset() noexcept = 0;
    virtual bool feed(uint8_t byte, uint64_t now, BackendEvent &event) = 0;
    virtual BackendDiagnostics take_diagnostics() noexcept = 0;
};
} // namespace rclink::detail
