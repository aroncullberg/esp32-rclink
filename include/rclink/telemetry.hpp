// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace rclink::telemetry {
struct Battery {
    uint32_t voltage_mv{0};
    uint32_t current_ma{0};
    uint32_t consumed_mah{0};
    uint8_t remaining_pct{0};
};
struct Gps {
    int32_t latitude_e7{0};
    int32_t longitude_e7{0};
    uint32_t speed_cms{0};
    uint16_t heading_cd{0};
    int32_t altitude_m{0};
    uint8_t satellites{0};
};
struct GpsTime {
    uint16_t year{2026};
    uint8_t month{1};
    uint8_t day{1};
    uint8_t hour{0};
    uint8_t minute{0};
    uint8_t second{0};
    uint16_t millisecond{0};
};
struct ExtendedGps {
    uint8_t fix_type{0};
    int16_t north_cms{0};
    int16_t east_cms{0};
    int16_t vertical_cms{0};
    int16_t speed_accuracy_cms{0};
    int16_t track_accuracy_ddeg{0};
    int16_t ellipsoid_m{0};
    int16_t horizontal_accuracy_cm{0};
    int16_t vertical_accuracy_cm{0};
    uint8_t hdop_d{0};
    uint8_t vdop_d{0};
};
struct Vario {
    int16_t vertical_cms{0};
};
enum class BaroVariant {
    AltitudeOnly,
    TbsLogSpeed,
    ElrsSpeed
};
struct BaroAltitude {
    int32_t altitude_dm{0};
    int16_t vertical_cms{0};
    BaroVariant variant{BaroVariant::ElrsSpeed};
};
struct Airspeed {
    uint32_t speed_cms{0};
};
struct Rpm {
    uint8_t source_id{0};
    uint8_t count{0};
    std::array<int32_t, 19> rpm{};
};
struct Temperature {
    uint8_t source_id{0};
    uint8_t count{0};
    std::array<int16_t, 20> deci_celsius{};
};
struct Voltages {
    uint8_t source_id{0};
    uint8_t count{0};
    std::array<uint16_t, 29> millivolts{};
};
struct Vtx {
    uint8_t origin{0xc8};
    uint8_t power_dbm{0};
    uint16_t frequency_mhz{0};
    bool pit_mode{false};
    uint8_t pit_control{0};
    uint8_t pit_switch{0};
};
struct Barometer {
    int32_t pressure_pa{0};
    int32_t temperature_cc{0};
};
struct Magnetometer {
    float x_milligauss{0};
    float y_milligauss{0};
    float z_milligauss{0};
};
struct AccelGyro {
    uint32_t sample_time_us{0};
    std::array<float, 3> gyro_dps{};
    std::array<float, 3> acceleration_g{};
    int16_t temperature_cc{0};
};
struct Attitude {
    float pitch_rad{0};
    float roll_rad{0};
    float yaw_rad{0};
};
struct MavlinkFc {
    int16_t airspeed_wire{0};
    uint8_t base_mode{0};
    uint32_t custom_mode{0};
    uint8_t autopilot_type{0};
    uint8_t vehicle_type{0};
};
struct FlightMode {
    std::array<char, 59> text{};
};
struct EspNow {
    uint8_t seat{0};
    uint8_t lap{0};
    std::array<char, 15> current{};
    std::array<char, 15> split{};
    std::array<char, 20> text{};
};
struct MavlinkSensors {
    uint32_t present{0};
    uint32_t enabled{0};
    uint32_t healthy{0};
};
// Split MAVLink messages into chunks before submitting them.
struct MavlinkChunk {
    uint8_t last_chunk{0};
    uint8_t index{0};
    uint8_t size{0};
    std::array<uint8_t, 58> data{};
};
struct Heartbeat {
    uint8_t origin{0xc8};
};
struct DeviceInfo {
    uint8_t destination{0xea};
    std::array<char, 33> name{};
    uint32_t serial{0};
    uint32_t hardware{0};
    uint32_t firmware{0};
};
struct Encoded {
    std::array<uint8_t, 64> bytes{};
    size_t size{0};
    uint16_t key{0};
};
// Use Kconfig defaults when interval_ms or ttl_ms is zero.
struct Options {
    uint32_t interval_ms{0};
    uint32_t ttl_ms{0};
};
bool encode(const Battery &value, Encoded &out) noexcept;
bool encode(const Gps &value, Encoded &out) noexcept;
bool encode(const GpsTime &value, Encoded &out) noexcept;
bool encode(const ExtendedGps &value, Encoded &out) noexcept;
bool encode(const Vario &value, Encoded &out) noexcept;
bool encode(const BaroAltitude &value, Encoded &out) noexcept;
bool encode(const Airspeed &value, Encoded &out) noexcept;
bool encode(const Rpm &value, Encoded &out) noexcept;
bool encode(const Temperature &value, Encoded &out) noexcept;
bool encode(const Voltages &value, Encoded &out) noexcept;
bool encode(const Vtx &value, Encoded &out) noexcept;
bool encode(const Barometer &value, Encoded &out) noexcept;
bool encode(const Magnetometer &value, Encoded &out) noexcept;
bool encode(const AccelGyro &value, Encoded &out) noexcept;
bool encode(const Attitude &value, Encoded &out) noexcept;
bool encode(const MavlinkFc &value, Encoded &out) noexcept;
bool encode(const FlightMode &value, Encoded &out) noexcept;
bool encode(const EspNow &value, Encoded &out) noexcept;
bool encode(const MavlinkSensors &value, Encoded &out) noexcept;
bool encode(const MavlinkChunk &value, Encoded &out) noexcept;
bool encode(const Heartbeat &value, Encoded &out) noexcept;
bool encode(const DeviceInfo &value, Encoded &out) noexcept;

// Set extended when the payload requires CRSF destination and origin addresses.
bool encode_payload(uint8_t type, const uint8_t *payload, size_t size, Encoded &out,
                    bool extended = false, uint8_t destination = 0xea,
                    uint8_t origin = 0xc8) noexcept;
} // namespace rclink::telemetry
