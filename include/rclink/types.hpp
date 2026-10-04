// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
namespace rclink {
enum class Protocol : uint8_t {
    Auto,
    Sbus,
    Crsf
};
enum class LinkState : uint8_t {
    Stopped,
    Detecting,
    NoData,
    Recovering,
    Active,
    ChannelTimeout,
    ReceiverFailsafe,
    RfLoss,
    TransportError
};
const char *name(Protocol p) noexcept;
const char *name(LinkState s) noexcept;
struct Channel {
    uint16_t raw{0};
    uint16_t scaled{0};
    uint16_t low_threshold{667};
    uint16_t high_threshold{1333};
    uint16_t deadband{0};
    bool low() const noexcept {
        return scaled < low_threshold;
    }
    bool high() const noexcept {
        return scaled >= high_threshold;
    }
    bool mid() const noexcept {
        return !low() && !high();
    }
    float unit() const noexcept {
        return std::min<uint16_t>(scaled, 2000) / 2000.f;
    }
    float bipolar() const noexcept {
        const int offset = int(std::min<uint16_t>(scaled, 2000)) - 1000;
        if (std::abs(offset) <= deadband) {
            return 0.f;
        }

        const float travel = 1000 - deadband;
        if (offset > 0) {
            return float(offset - deadband) / travel;
        }
        return float(offset + deadband) / travel;
    }
};
class Channels {
    std::array<Channel, 16> values_{};

  public:
    const Channel &ch1;
    const Channel &ch2;
    const Channel &ch3;
    const Channel &ch4;
    const Channel &ch5;
    const Channel &ch6;
    const Channel &ch7;
    const Channel &ch8;
    const Channel &ch9;
    const Channel &ch10;
    const Channel &ch11;
    const Channel &ch12;
    const Channel &ch13;
    const Channel &ch14;
    const Channel &ch15;
    const Channel &ch16;
    const Channel &aux1;
    const Channel &aux2;
    const Channel &aux3;
    const Channel &aux4;
    const Channel &aux5;
    const Channel &aux6;
    const Channel &aux7;
    const Channel &aux8;
    const Channel &aux9;
    const Channel &aux10;
    const Channel &aux11;
    const Channel &aux12;
    Channels() noexcept
        : ch1(values_[0]),
          ch2(values_[1]),
          ch3(values_[2]),
          ch4(values_[3]),
          ch5(values_[4]),
          ch6(values_[5]),
          ch7(values_[6]),
          ch8(values_[7]),
          ch9(values_[8]),
          ch10(values_[9]),
          ch11(values_[10]),
          ch12(values_[11]),
          ch13(values_[12]),
          ch14(values_[13]),
          ch15(values_[14]),
          ch16(values_[15]),
          aux1(values_[4]),
          aux2(values_[5]),
          aux3(values_[6]),
          aux4(values_[7]),
          aux5(values_[8]),
          aux6(values_[9]),
          aux7(values_[10]),
          aux8(values_[11]),
          aux9(values_[12]),
          aux10(values_[13]),
          aux11(values_[14]),
          aux12(values_[15]) {}
    Channels(const Channels &other) noexcept
        : Channels() {
        values_ = other.values_;
    }
    Channels &operator=(const Channels &other) noexcept {
        values_ = other.values_;
        return *this;
    }
    // Return the channel at a zero-based index, or nullptr when it is out of range.
    const Channel *at(size_t index) const noexcept {
        if (index >= size()) {
            return nullptr;
        }
        return &values_[index];
    }

    Channel *at(size_t index) noexcept {
        if (index >= size()) {
            return nullptr;
        }
        return &values_[index];
    }

    // Look up ch1 through ch16 by number, returning nullptr for an invalid number.
    const Channel *channel(size_t one_based) const noexcept {
        if (one_based == 0) {
            return nullptr;
        }
        return at(one_based - 1);
    }

    Channel *channel(size_t one_based) noexcept {
        if (one_based == 0) {
            return nullptr;
        }
        return at(one_based - 1);
    }

    static constexpr size_t size() noexcept {
        return 16;
    }
};
struct Calibration {
    uint16_t minimum;
    uint16_t center;
    uint16_t maximum;
};
struct Config {
    Protocol protocol{Protocol::Auto};
    int uart{1};
    int tx_pin{17};
    int rx_pin{16};
    bool sbus_inverted{true};
    uint32_t crsf_baud{420000};
    uint32_t timeout_ms{300};
    uint32_t probe_ms{1000};
    uint32_t parser_gap_ms{10};
    uint32_t evaluation_ms{5};
    uint32_t recovery_frames{3};
    uint32_t detection_frames{2};
    Calibration sbus{192, 992, 1792};
    Calibration crsf{172, 992, 1810};
    uint16_t low_threshold{667};
    uint16_t high_threshold{1333};
    uint16_t deadband{0};
    std::array<uint16_t, 16> fallback{0, 1000, 1000, 1000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // Treat zero uplink quality as receiver-specific evidence of RF loss.
    bool failsafe_on_zero_lq{false};
    uint32_t rf_timeout_ms{1000};
    uint32_t telemetry_interval_ms{20};
    uint32_t telemetry_ttl_ms{2000};
    uint32_t heartbeat_ms{1000};
    static Config defaults();
    bool valid() const noexcept;
};
struct RfStatistics {
    bool available{false};
    uint64_t timestamp_us{0};
    uint8_t rssi1{0};
    uint8_t rssi2{0};
    uint8_t quality{0};
    uint8_t rf_mode{0};
    uint8_t tx_power{0};
    int8_t snr{0};
    bool fresh{false};
};
struct Diagnostics {
    uint64_t channel_frames{0};
    uint64_t malformed_frames{0};
    uint64_t crc_errors{0};
    uint64_t overflows{0};
    uint64_t failures{0};
    uint64_t recoveries{0};
    uint64_t transitions{0};
    uint64_t telemetry_sent{0};
    uint64_t telemetry_dropped{0};
    uint64_t tx_errors{0};
};
struct Snapshot {
    // Keep the last received values available during failsafe.
    Channels channels;
    Channels fallback_channels;
    Protocol protocol{Protocol::Auto};
    LinkState state{LinkState::Stopped};
    bool usable{false};
    bool frame_lost{false};
    bool receiver_failsafe{false};
    bool digital17{false};
    bool digital18{false};
    bool has_crsf_status{false};
    // Some ELRS channel frames include an additional arm status byte.
    uint8_t crsf_status{0};
    uint64_t sequence{0};
    uint64_t last_channel_us{0};
    uint64_t last_good_us{0};
    uint64_t transition_us{0};
    uint64_t last_failure_us{0};
    uint64_t last_recovery_us{0};
    LinkState last_failure{LinkState::Stopped};
    RfStatistics rf;
    Diagnostics diagnostics;
    const Channels &effective_channels() const noexcept {
        return usable ? channels : fallback_channels;
    }
    uint64_t channel_age_us(uint64_t now) const noexcept {
        return sequence ? now - last_channel_us : UINT64_MAX;
    }
};
} // namespace rclink
