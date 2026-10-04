// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
// Frame construction is adapted from AlfredoCRSF's packet writers.
#include "rclink/telemetry.hpp"
#include "codec.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace rclink::telemetry {
namespace {
struct Writer {
    std::array<uint8_t, 60> data{};
    size_t size{0};
    void write_u8(uint8_t value) {
        data[size++] = value;
    }
    void write_u16(uint16_t value) {
        write_u8(value >> 8);
        write_u8(value);
    }
    void write_u24(uint32_t value) {
        write_u8(value >> 16);
        write_u8(value >> 8);
        write_u8(value);
    }
    void write_u32(uint32_t value) {
        write_u16(value >> 16);
        write_u16(value);
    }
    bool finish(uint8_t type, Encoded &out, uint8_t source = 0) {
        out = {};
        out.size = detail::crsf::frame(type, data.data(), size, out.bytes);
        out.key = (uint16_t(type) << 8) | source;
        return out.size != 0;
    }
};
bool finite(float value) {
    return std::isfinite(value);
}

int16_t scaled(float value, float scale) {
    const double wire_value = double(value) * scale;
    const double clamped = std::clamp(wire_value, -32768., 32767.);
    return static_cast<int16_t>(std::lround(clamped));
}

uint16_t speed(uint32_t cms) {
    const uint64_t tenths_kmh = (uint64_t(cms) * 36 + 50) / 100;
    return static_cast<uint16_t>(std::min<uint64_t>(tenths_kmh, 65535));
}
} // namespace
bool encode(const Battery &value, Encoded &out) noexcept {
    if (value.remaining_pct > 100 || value.consumed_mah > 0xffffff) {
        return false;
    }
    Writer writer;
    writer.write_u16(std::min<uint32_t>(value.voltage_mv / 100, 65535));
    writer.write_u16(std::min<uint32_t>(value.current_ma / 100, 65535));
    writer.write_u24(value.consumed_mah);
    writer.write_u8(value.remaining_pct);
    return writer.finish(0x08, out);
}

bool encode(const Gps &value, Encoded &out) noexcept {
    const bool valid_latitude = std::abs(int64_t(value.latitude_e7)) <= 900000000;
    const bool valid_longitude = std::abs(int64_t(value.longitude_e7)) <= 1800000000;
    if (!valid_latitude || !valid_longitude || value.heading_cd >= 36000) {
        return false;
    }
    Writer writer;
    writer.write_u32(value.latitude_e7);
    writer.write_u32(value.longitude_e7);
    writer.write_u16(speed(value.speed_cms));
    writer.write_u16(value.heading_cd);
    const int64_t offset_altitude = int64_t(value.altitude_m) + 1000;
    const auto wire_altitude = std::clamp<int64_t>(offset_altitude, 0, 65535);
    writer.write_u16(static_cast<uint16_t>(wire_altitude));
    writer.write_u8(value.satellites);
    return writer.finish(0x02, out);
}

bool encode(const GpsTime &value, Encoded &out) noexcept {
    if (value.year > 32767 || value.month < 1 || value.month > 12 || value.day < 1) {
        return false;
    }
    if (value.hour > 23 || value.minute > 59 || value.second > 60 || value.millisecond > 999) {
        return false;
    }

    const bool leap_year = value.year % 4 == 0 && (value.year % 100 != 0 || value.year % 400 == 0);
    constexpr uint8_t month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint8_t days_in_month = month_days[value.month - 1];
    if (value.month == 2 && leap_year) {
        ++days_in_month;
    }
    if (value.day > days_in_month) {
        return false;
    }
    Writer writer;
    writer.write_u16(value.year);
    writer.write_u8(value.month);
    writer.write_u8(value.day);
    writer.write_u8(value.hour);
    writer.write_u8(value.minute);
    writer.write_u8(value.second);
    writer.write_u16(value.millisecond);
    return writer.finish(0x03, out);
}

bool encode(const ExtendedGps &value, Encoded &out) noexcept {
    Writer writer;
    writer.write_u8(value.fix_type);
    writer.write_u16(value.north_cms);
    writer.write_u16(value.east_cms);
    writer.write_u16(value.vertical_cms);
    writer.write_u16(value.speed_accuracy_cms);
    writer.write_u16(value.track_accuracy_ddeg);
    writer.write_u16(value.ellipsoid_m);
    writer.write_u16(value.horizontal_accuracy_cm);
    writer.write_u16(value.vertical_accuracy_cm);
    writer.write_u8(0);
    writer.write_u8(value.hdop_d);
    writer.write_u8(value.vdop_d);
    return writer.finish(0x06, out);
}

bool encode(const Vario &value, Encoded &out) noexcept {
    Writer writer;
    writer.write_u16(value.vertical_cms);
    return writer.finish(0x07, out);
}

bool encode(const BaroAltitude &value, Encoded &out) noexcept {
    Writer writer;
    const int32_t dm = std::clamp<int32_t>(value.altitude_dm, -10000, 327660);
    if (dm < 22768) {
        writer.write_u16(dm + 10000);
    } else {
        const uint16_t meters = (dm + 5) / 10;
        writer.write_u16(0x8000 | meters);
    }
    if (value.variant == BaroVariant::TbsLogSpeed) {
        const double magnitude = std::log(std::abs(int(value.vertical_cms)) / 100. + 1) / .026;
        const int packed = std::min(127, int(magnitude));
        writer.write_u8(value.vertical_cms < 0 ? -packed : packed);
    } else if (value.variant == BaroVariant::ElrsSpeed) {
        writer.write_u16(value.vertical_cms);
    } else if (value.variant != BaroVariant::AltitudeOnly) {
        return false;
    }
    return writer.finish(0x09, out);
}

bool encode(const Airspeed &value, Encoded &out) noexcept {
    Writer writer;
    writer.write_u16(speed(value.speed_cms));
    return writer.finish(0x0a, out);
}

bool encode(const Rpm &value, Encoded &out) noexcept {
    if (!value.count || value.count > value.rpm.size()) {
        return false;
    }
    Writer writer;
    writer.write_u8(value.source_id);
    for (size_t i = 0; i < value.count; ++i) {
        if (value.rpm[i] < -8388608 || value.rpm[i] > 8388607) {
            return false;
        }
        writer.write_u24(value.rpm[i]);
    }
    return writer.finish(0x0c, out, value.source_id);
}

bool encode(const Temperature &value, Encoded &out) noexcept {
    if (!value.count || value.count > value.deci_celsius.size()) {
        return false;
    }
    Writer writer;
    writer.write_u8(value.source_id);
    for (size_t i = 0; i < value.count; ++i) {
        writer.write_u16(value.deci_celsius[i]);
    }
    return writer.finish(0x0d, out, value.source_id);
}

bool encode(const Voltages &value, Encoded &out) noexcept {
    if (!value.count || value.count > value.millivolts.size()) {
        return false;
    }
    Writer writer;
    writer.write_u8(value.source_id);
    for (size_t i = 0; i < value.count; ++i) {
        writer.write_u16(value.millivolts[i]);
    }
    return writer.finish(0x0e, out, value.source_id);
}

bool encode(const Vtx &value, Encoded &out) noexcept {
    if (value.pit_control > 3 || value.pit_switch > 15) {
        return false;
    }
    Writer writer;
    writer.write_u8(value.origin);
    writer.write_u8(value.power_dbm);
    writer.write_u16(value.frequency_mhz);
    writer.write_u8(uint8_t(value.pit_mode) | (value.pit_control << 1) | (value.pit_switch << 3));
    return writer.finish(0x10, out, value.origin);
}

bool encode(const Barometer &value, Encoded &out) noexcept {
    Writer writer;
    writer.write_u32(value.pressure_pa);
    writer.write_u32(value.temperature_cc);
    return writer.finish(0x11, out);
}

bool encode(const Magnetometer &value, Encoded &out) noexcept {
    if (!finite(value.x_milligauss) || !finite(value.y_milligauss) || !finite(value.z_milligauss)) {
        return false;
    }
    Writer writer;
    writer.write_u16(scaled(value.x_milligauss, 3));
    writer.write_u16(scaled(value.y_milligauss, 3));
    writer.write_u16(scaled(value.z_milligauss, 3));
    return writer.finish(0x12, out);
}

bool encode(const AccelGyro &value, Encoded &out) noexcept {
    Writer writer;
    writer.write_u32(value.sample_time_us);
    for (float x : value.gyro_dps) {
        if (!finite(x)) {
            return false;
        }
        writer.write_u16(scaled(x, 32767.f / 2000));
    }
    for (float x : value.acceleration_g) {
        if (!finite(x)) {
            return false;
        }
        writer.write_u16(scaled(x, 32767.f / 16));
    }
    writer.write_u16(value.temperature_cc);
    return writer.finish(0x13, out);
}

bool encode(const Attitude &value, Encoded &out) noexcept {
    Writer writer;
    for (float x : {value.pitch_rad, value.roll_rad, value.yaw_rad}) {
        if (!finite(x) || std::abs(x) > 3.141593f) {
            return false;
        }
        writer.write_u16(scaled(x, 10000));
    }
    return writer.finish(0x1e, out);
}

bool encode(const MavlinkFc &value, Encoded &out) noexcept {
    Writer writer;
    writer.write_u16(value.airspeed_wire);
    writer.write_u8(value.base_mode);
    writer.write_u32(value.custom_mode);
    writer.write_u8(value.autopilot_type);
    writer.write_u8(value.vehicle_type);
    return writer.finish(0x1f, out);
}

bool encode(const FlightMode &value, Encoded &out) noexcept {
    Writer writer;
    for (char x : value.text) {
        writer.write_u8(x);
        if (!x) {
            return writer.finish(0x21, out);
        }
    }
    return false;
}

bool encode(const EspNow &value, Encoded &out) noexcept {
    Writer writer;
    writer.write_u8(value.seat);
    writer.write_u8(value.lap);
    for (char x : value.current) {
        writer.write_u8(x);
    }
    for (char x : value.split) {
        writer.write_u8(x);
    }
    for (char x : value.text) {
        writer.write_u8(x);
    }
    return writer.finish(0x22, out);
}

bool encode(const MavlinkSensors &value, Encoded &out) noexcept {
    Writer writer;
    writer.write_u32(value.present);
    writer.write_u32(value.enabled);
    writer.write_u32(value.healthy);
    return writer.finish(0xac, out);
}

bool encode(const MavlinkChunk &value, Encoded &out) noexcept {
    if (value.last_chunk > 15 || value.index > value.last_chunk || value.size > 58 || !value.size) {
        return false;
    }
    Writer writer;
    writer.write_u8((value.last_chunk << 4) | value.index);
    writer.write_u8(value.size);
    for (size_t i = 0; i < value.size; ++i) {
        writer.write_u8(value.data[i]);
    }
    return writer.finish(0xaa, out, value.index);
}

bool encode(const Heartbeat &value, Encoded &out) noexcept {
    Writer writer;
    writer.write_u16(value.origin);
    return writer.finish(0x0b, out);
}

bool encode(const DeviceInfo &value, Encoded &out) noexcept {
    Writer writer;
    writer.write_u8(value.destination);
    writer.write_u8(0xc8);
    bool terminated = false;
    for (char x : value.name) {
        writer.write_u8(x);
        if (!x) {
            terminated = true;
            break;
        }
    }
    if (!terminated) {
        return false;
    }
    writer.write_u32(value.serial);
    writer.write_u32(value.hardware);
    writer.write_u32(value.firmware);
    writer.write_u8(0);
    writer.write_u8(0);
    return writer.finish(0x29, out, value.destination);
}

bool encode_payload(uint8_t type, const uint8_t *payload, size_t size, Encoded &out, bool extended,
                    uint8_t destination, uint8_t origin) noexcept {
    if ((size && !payload) || size > (extended ? 58u : 60u)) {
        return false;
    }
    Writer writer;
    if (extended) {
        writer.write_u8(destination);
        writer.write_u8(origin);
    }
    for (size_t i = 0; i < size; ++i) {
        writer.write_u8(payload[i]);
    }
    return writer.finish(type, out, extended ? destination : 0);
}
} // namespace rclink::telemetry
