// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#pragma once
#include "backends/backend.hpp"
#include "driver/uart.h"
#include "esp_err.h"
namespace rclink::detail {
struct ReadResult {
    int bytes{0};
    uint32_t overflows{0};
    uint32_t line_errors{0};
};
class UartTransport {
    Config config_;
    QueueHandle_t events_{nullptr};
    bool claimed_{false};
    bool installed_{false};

  public:
    ~UartTransport();
    esp_err_t start(const Config &config, UartSettings settings);
    esp_err_t configure(UartSettings settings);
    void stop();
    ReadResult read(uint8_t *buffer, size_t capacity, uint32_t timeout_ms);
    bool tx_ready() const;
    int write(const uint8_t *buffer, size_t size);
};
} // namespace rclink::detail
