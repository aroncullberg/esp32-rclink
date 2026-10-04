// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#include "rclink/receiver.hpp"
#include "backends/crsf/telemetry_scheduler.hpp"
#include "engine.hpp"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "idf_uart_transport.hpp"
#include "sdkconfig.h"
#include <atomic>
#include <cstring>
#include <mutex>
#include <new>
namespace rclink {
namespace {
uint64_t now_us() {
    return static_cast<uint64_t>(esp_timer_get_time());
}
} // namespace
struct Receiver::Impl {
    std::mutex lifecycle;
    std::mutex mutex;
    Config config;
    detail::Engine engine;
    detail::crsf::TelemetryScheduler<CONFIG_RCLINK_TELEMETRY_SLOTS> telemetry;
    std::atomic<bool> running{false};
    SemaphoreHandle_t done{nullptr};
    TaskHandle_t task{nullptr};
    detail::UartTransport transport;
    static void entry(void *self) {
        static_cast<Impl *>(self)->run();
    }
    void run() {
        Protocol active = config.protocol == Protocol::Sbus ? Protocol::Sbus : Protocol::Crsf;
        std::array<uint8_t, 256> buffer{};
        while (running.load()) {
            Protocol candidate;
            detail::UartSettings settings;
            {
                std::lock_guard<std::mutex> lock(mutex);
                engine.tick(now_us());
                candidate = engine.candidate();
                settings = engine.backend().settings(config);
            }
            if (candidate != active) {
                const esp_err_t result = transport.configure(settings);
                active = candidate;
                if (result != ESP_OK) {
                    std::lock_guard<std::mutex> lock(mutex);
                    engine.transport_error(now_us());
                }
            }
            const auto read = transport.read(buffer.data(), buffer.size(), config.evaluation_ms);
            const uint64_t now = now_us();
            telemetry::Encoded packet;
            bool send = false;
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (read.overflows || read.line_errors) {
                    engine.discontinuity();
                }
                engine.diagnostics().overflows += read.overflows;
                engine.diagnostics().malformed_frames += read.line_errors;
                engine.tick(now);
                if (read.bytes > 0 && engine.candidate() == active) {
                    engine.feed(buffer.data(), read.bytes, now);
                }
                if (read.bytes < 0) {
                    engine.transport_error(now);
                }
                const auto state = engine.snapshot(now);
                // Discard this read if a snapshot advanced detection to another protocol.
                if (engine.candidate() != active) {
                    continue;
                }
                uint8_t origin;
                if (engine.take_ping(origin)) {
                    telemetry.respond_to_ping(origin, config, now);
                }
                const bool crsf_active = state.protocol == Protocol::Crsf && state.usable;
                if (crsf_active && transport.tx_ready()) {
                    uint64_t &dropped = engine.diagnostics().telemetry_dropped;
                    send = telemetry.take(now, config, packet, dropped);
                }
            }
            if (send) {
                const int written = transport.write(packet.bytes.data(), packet.size);
                std::lock_guard<std::mutex> lock(mutex);
                if (written == static_cast<int>(packet.size)) {
                    ++engine.diagnostics().telemetry_sent;
                } else {
                    ++engine.diagnostics().tx_errors;
                }
            }
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            engine.stop(now_us());
        }
        xSemaphoreGive(done);
        vTaskDelete(nullptr);
    }
};
Receiver::Receiver() noexcept
    : impl_(new (std::nothrow) Impl) {}
Receiver::~Receiver() {
    stop();
    if (impl_) {
        if (impl_->done) {
            vSemaphoreDelete(impl_->done);
        }
        delete impl_;
    }
}

esp_err_t Receiver::start() {
    return start(Config::defaults());
}

esp_err_t Receiver::start(const Config &config) {
    if (!impl_) {
        return ESP_ERR_NO_MEM;
    }
    std::lock_guard<std::mutex> lifecycle(impl_->lifecycle);
    if (impl_->running) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!config.valid()) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!impl_->done) {
        impl_->done = xSemaphoreCreateBinary();
        if (!impl_->done) {
            return ESP_ERR_NO_MEM;
        }
    }
    while (xSemaphoreTake(impl_->done, 0) == pdTRUE) {
    }
    detail::UartSettings settings;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->config = config;
        impl_->engine.start(config, now_us());
        impl_->telemetry.clear();
        settings = impl_->engine.backend().settings(config);
    }
    esp_err_t result = impl_->transport.start(config, settings);
    if (result != ESP_OK) {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->engine.stop(now_us());
        return result;
    }
    impl_->running = true;
    const BaseType_t task_result = xTaskCreate(Impl::entry, "rclink", CONFIG_RCLINK_TASK_STACK,
                                               impl_, CONFIG_RCLINK_TASK_PRIORITY, &impl_->task);
    if (task_result != pdPASS) {
        impl_->running = false;
        {
            std::lock_guard<std::mutex> lock(impl_->mutex);
            impl_->engine.stop(now_us());
        }
        impl_->transport.stop();
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t Receiver::stop() {
    if (!impl_) {
        return ESP_OK;
    }
    std::lock_guard<std::mutex> lifecycle(impl_->lifecycle);
    if (impl_->running.exchange(false)) {
        xSemaphoreTake(impl_->done, portMAX_DELAY);
    }
    impl_->task = nullptr;
    impl_->transport.stop();
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->telemetry.clear();
    }
    return ESP_OK;
}

Snapshot Receiver::snapshot() const {
    if (!impl_) {
        return {};
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->engine.snapshot(now_us());
}

esp_err_t Receiver::wait_for_link(uint32_t timeout_ms) const {
    const uint64_t end = now_us() + uint64_t(timeout_ms) * 1000;
    do {
        const auto state = snapshot();
        if (state.usable) {
            return ESP_OK;
        }
        if (state.state == LinkState::Stopped || state.state == LinkState::TransportError) {
            return ESP_ERR_INVALID_STATE;
        }
        if (now_us() >= end) {
            break;
        }
        vTaskDelay(1);
    } while (true);
    return ESP_ERR_TIMEOUT;
}

esp_err_t Receiver::submit_encoded(const telemetry::Encoded &packet, telemetry::Options options) {
    if (!impl_) {
        return ESP_ERR_NO_MEM;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->running) {
        return ESP_ERR_INVALID_STATE;
    }
    const Protocol configured = impl_->config.protocol;
    const Protocol selected = impl_->engine.snapshot(now_us()).protocol;
    if (configured == Protocol::Sbus || selected == Protocol::Sbus) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (!options.interval_ms) {
        options.interval_ms = impl_->config.telemetry_interval_ms;
    }
    if (!options.ttl_ms) {
        options.ttl_ms = impl_->config.telemetry_ttl_ms;
    }
    if (options.ttl_ms < options.interval_ms) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!impl_->telemetry.put(packet, options, now_us())) {
        ++impl_->engine.diagnostics().telemetry_dropped;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t Receiver::submit_payload(uint8_t type, const uint8_t *data, size_t size,
                                   telemetry::Options options, bool extended, uint8_t destination,
                                   uint8_t origin) {
    telemetry::Encoded packet;
    if (!telemetry::encode_payload(type, data, size, packet, extended, destination, origin)) {
        return ESP_ERR_INVALID_ARG;
    }
    return submit_encoded(packet, options);
}
} // namespace rclink
