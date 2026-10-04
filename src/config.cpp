// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#include "rclink/types.hpp"
#include "sdkconfig.h"
namespace rclink {
Config Config::defaults() {
    Config config;
#ifdef CONFIG_RCLINK_PROTOCOL_SBUS
    config.protocol = Protocol::Sbus;
#elif defined(CONFIG_RCLINK_PROTOCOL_CRSF)
    config.protocol = Protocol::Crsf;
#endif
#ifdef CONFIG_RCLINK_SBUS_INVERTED
    config.sbus_inverted = true;
#else
    config.sbus_inverted = false;
#endif
#ifdef CONFIG_RCLINK_ZERO_LQ_FAILSAFE
    config.failsafe_on_zero_lq = true;
#endif
    config.uart = CONFIG_RCLINK_UART;
    config.tx_pin = CONFIG_RCLINK_TX_PIN;
    config.rx_pin = CONFIG_RCLINK_RX_PIN;
    config.crsf_baud = CONFIG_RCLINK_CRSF_BAUD;
    config.timeout_ms = CONFIG_RCLINK_TIMEOUT_MS;
    config.probe_ms = CONFIG_RCLINK_PROBE_MS;
    config.parser_gap_ms = CONFIG_RCLINK_PARSER_GAP_MS;
    config.evaluation_ms = CONFIG_RCLINK_EVALUATION_MS;
    config.recovery_frames = CONFIG_RCLINK_RECOVERY_FRAMES;
    config.detection_frames = CONFIG_RCLINK_DETECTION_FRAMES;
    config.low_threshold = CONFIG_RCLINK_LOW_THRESHOLD;
    config.high_threshold = CONFIG_RCLINK_HIGH_THRESHOLD;
    config.deadband = CONFIG_RCLINK_DEADBAND;
    config.rf_timeout_ms = CONFIG_RCLINK_RF_TIMEOUT_MS;
    config.telemetry_interval_ms = CONFIG_RCLINK_TELEMETRY_INTERVAL_MS;
    config.telemetry_ttl_ms = CONFIG_RCLINK_TELEMETRY_TTL_MS;
    config.heartbeat_ms = CONFIG_RCLINK_HEARTBEAT_MS;
    config.sbus = {CONFIG_RCLINK_SBUS_MIN, CONFIG_RCLINK_SBUS_CENTER, CONFIG_RCLINK_SBUS_MAX};
    config.crsf = {CONFIG_RCLINK_CRSF_MIN, CONFIG_RCLINK_CRSF_CENTER, CONFIG_RCLINK_CRSF_MAX};
    config.fallback[0] = CONFIG_RCLINK_FALLBACK_CH1;
    config.fallback[1] = CONFIG_RCLINK_FALLBACK_CH2;
    config.fallback[2] = CONFIG_RCLINK_FALLBACK_CH3;
    config.fallback[3] = CONFIG_RCLINK_FALLBACK_CH4;
    config.fallback[4] = CONFIG_RCLINK_FALLBACK_CH5;
    config.fallback[5] = CONFIG_RCLINK_FALLBACK_CH6;
    config.fallback[6] = CONFIG_RCLINK_FALLBACK_CH7;
    config.fallback[7] = CONFIG_RCLINK_FALLBACK_CH8;
    config.fallback[8] = CONFIG_RCLINK_FALLBACK_CH9;
    config.fallback[9] = CONFIG_RCLINK_FALLBACK_CH10;
    config.fallback[10] = CONFIG_RCLINK_FALLBACK_CH11;
    config.fallback[11] = CONFIG_RCLINK_FALLBACK_CH12;
    config.fallback[12] = CONFIG_RCLINK_FALLBACK_CH13;
    config.fallback[13] = CONFIG_RCLINK_FALLBACK_CH14;
    config.fallback[14] = CONFIG_RCLINK_FALLBACK_CH15;
    config.fallback[15] = CONFIG_RCLINK_FALLBACK_CH16;
    return config;
}
} // namespace rclink
