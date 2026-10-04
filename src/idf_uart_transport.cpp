// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#include "idf_uart_transport.hpp"
#include "driver/gpio.h"
#include "freertos/task.h"
#include <mutex>
namespace rclink::detail {
namespace {
std::mutex ownership;
std::array<bool, UART_NUM_MAX> uarts{};
std::array<bool, GPIO_NUM_MAX> pins{};
TickType_t ticks(uint32_t ms) {
    return std::max<TickType_t>(1, (uint64_t(ms) * configTICK_RATE_HZ + 999) / 1000);
}
} // namespace
UartTransport::~UartTransport() {
    stop();
}

esp_err_t UartTransport::start(const Config &config, UartSettings settings) {
    if (installed_ || claimed_) {
        return ESP_ERR_INVALID_STATE;
    }
    if (config.uart < 0 || config.uart >= UART_NUM_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    const bool valid_tx = GPIO_IS_VALID_OUTPUT_GPIO(config.tx_pin);
    const bool valid_rx = GPIO_IS_VALID_GPIO(config.rx_pin);
    if (!valid_tx || !valid_rx || config.tx_pin == config.rx_pin) {
        return ESP_ERR_INVALID_ARG;
    }
    {
        std::lock_guard<std::mutex> lock(ownership);
        const auto port = static_cast<uart_port_t>(config.uart);
        const bool uart_claimed = uarts[config.uart] || uart_is_driver_installed(port);
        const bool pin_claimed = pins[config.tx_pin] || pins[config.rx_pin];
        if (uart_claimed || pin_claimed) {
            return ESP_ERR_INVALID_STATE;
        }
        uarts[config.uart] = true;
        pins[config.tx_pin] = true;
        pins[config.rx_pin] = true;
        claimed_ = true;
    }
    config_ = config;
    esp_err_t result = configure(settings);
    if (result == ESP_OK) {
        const auto port = static_cast<uart_port_t>(config_.uart);
        result = uart_driver_install(port, 2048, 256, 32, &events_, 0);
        installed_ = result == ESP_OK;
    }
    if (result != ESP_OK) {
        stop();
    }
    return result;
}

esp_err_t UartTransport::configure(UartSettings settings) {
    uart_config_t uart{};
    uart.baud_rate = settings.baud;
    uart.data_bits = UART_DATA_8_BITS;
    uart.parity = settings.even_parity ? UART_PARITY_EVEN : UART_PARITY_DISABLE;
    uart.stop_bits = settings.two_stop_bits ? UART_STOP_BITS_2 : UART_STOP_BITS_1;
    uart.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart.source_clk = UART_SCLK_DEFAULT;
    const auto port = static_cast<uart_port_t>(config_.uart);
    esp_err_t err = uart_param_config(port, &uart);
    if (err != ESP_OK) {
        return err;
    }
    constexpr int unchanged = UART_PIN_NO_CHANGE;
    err = uart_set_pin(port, config_.tx_pin, config_.rx_pin, unchanged, unchanged);
    if (err != ESP_OK) {
        return err;
    }
    err = uart_set_line_inverse(port, settings.inverted ? UART_SIGNAL_RXD_INV : 0);
    if (err != ESP_OK) {
        return err;
    }
    if (installed_) {
        uart_flush_input(port);
        xQueueReset(events_);
    }
    return ESP_OK;
}

void UartTransport::stop() {
    if (installed_) {
        uart_driver_delete(static_cast<uart_port_t>(config_.uart));
        installed_ = false;
        events_ = nullptr;
    }
    if (claimed_) {
        gpio_reset_pin(static_cast<gpio_num_t>(config_.tx_pin));
        gpio_reset_pin(static_cast<gpio_num_t>(config_.rx_pin));
        std::lock_guard<std::mutex> lock(ownership);
        uarts[config_.uart] = false;
        pins[config_.tx_pin] = false;
        pins[config_.rx_pin] = false;
        claimed_ = false;
    }
}
ReadResult UartTransport::read(uint8_t *buffer, size_t capacity, uint32_t timeout_ms) {
    ReadResult result;
    const auto port = static_cast<uart_port_t>(config_.uart);
    uart_event_t event;
    while (xQueueReceive(events_, &event, 0) == pdTRUE) {
        switch (event.type) {
        case UART_FIFO_OVF:
        case UART_BUFFER_FULL:
            ++result.overflows;
            break;
        case UART_PARITY_ERR:
        case UART_FRAME_ERR:
        case UART_BREAK:
            ++result.line_errors;
            break;
        default:
            break;
        }
    }
    if (result.overflows || result.line_errors) {
        uart_flush_input(port);
        xQueueReset(events_);
    }
    result.bytes = uart_read_bytes(port, buffer, capacity, ticks(timeout_ms));
    return result;
}

bool UartTransport::tx_ready() const {
    return uart_wait_tx_done(static_cast<uart_port_t>(config_.uart), 0) == ESP_OK;
}

int UartTransport::write(const uint8_t *buffer, size_t size) {
    return uart_write_bytes(static_cast<uart_port_t>(config_.uart), buffer, size);
}
} // namespace rclink::detail
