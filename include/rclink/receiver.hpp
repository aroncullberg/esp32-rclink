// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#pragma once
#include "esp_err.h"
#include "rclink/telemetry.hpp"
#include "rclink/types.hpp"
namespace rclink {
// Receiver methods run in task context and serialize start and stop.
class Receiver {
    struct Impl;
    Impl *impl_;
    esp_err_t submit_encoded(const telemetry::Encoded &, telemetry::Options);

  public:
    Receiver() noexcept;
    ~Receiver();
    Receiver(const Receiver &) = delete;
    Receiver &operator=(const Receiver &) = delete;
    esp_err_t start();
    esp_err_t start(const Config &config);
    // Wait for the worker to finish before releasing the UART and GPIOs.
    esp_err_t stop();
    Snapshot snapshot() const;
    esp_err_t wait_for_link(uint32_t timeout_ms) const;
    template <class T> esp_err_t submit(const T &value, telemetry::Options options = {}) {
        telemetry::Encoded packet;
        if (!telemetry::encode(value, packet)) {
            return ESP_ERR_INVALID_ARG;
        }
        return submit_encoded(packet, options);
    }
    esp_err_t submit_payload(uint8_t type, const uint8_t *data, size_t size,
                             telemetry::Options options = {}, bool extended = false,
                             uint8_t destination = 0xea, uint8_t origin = 0xc8);
};
} // namespace rclink
