// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rclink/receiver.hpp"
extern "C" void app_main() {
    static rclink::Receiver receiver;
    const esp_err_t result = receiver.start();
    if (result != ESP_OK) {
        ESP_LOGE("monitor", "Receiver startup failed (%s)", esp_err_to_name(result));
        return;
    }
    uint64_t transitions = UINT64_MAX;
    while (true) {
        const auto state = receiver.snapshot();
        if (transitions != state.diagnostics.transitions) {
            transitions = state.diagnostics.transitions;
            ESP_LOGI("monitor", "protocol=%s state=%s usable=%d last_failure=%s failures=%llu",
                     rclink::name(state.protocol), rclink::name(state.state), state.usable,
                     rclink::name(state.last_failure),
                     static_cast<unsigned long long>(state.diagnostics.failures));
        }
        if (state.sequence) {
            ESP_LOGI(
                "monitor",
                "seq=%llu age=%llu ms ch1=%u %.3f ch2=%.3f aux1=%u high=%d lost=%d failsafe=%d "
                "CRC=%llu",
                static_cast<unsigned long long>(state.sequence),
                static_cast<unsigned long long>(state.channel_age_us(esp_timer_get_time()) / 1000),
                state.channels.ch1.scaled, state.channels.ch1.unit(), state.channels.ch2.bipolar(),
                state.channels.aux1.scaled, state.channels.aux1.high(), state.frame_lost,
                state.receiver_failsafe,
                static_cast<unsigned long long>(state.diagnostics.crc_errors));
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
