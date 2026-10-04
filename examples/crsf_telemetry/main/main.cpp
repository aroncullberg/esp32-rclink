// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rclink/receiver.hpp"
extern "C" void app_main() {
    static rclink::Receiver receiver;
    const esp_err_t result = receiver.start();
    if (result != ESP_OK) {
        ESP_LOGE("telemetry", "Receiver startup failed (%s)", esp_err_to_name(result));
        return;
    }
    while (true) {
        rclink::telemetry::Battery battery;
        battery.voltage_mv = 12000;
        battery.current_ma = 1500;
        battery.remaining_pct = 75;
        rclink::telemetry::Gps gps;
        gps.latitude_e7 = 593293000;
        gps.longitude_e7 = 180686000;
        gps.altitude_m = 25;
        gps.satellites = 10;
        gps.speed_cms = 100;
        gps.heading_cd = 9000;
        rclink::telemetry::FlightMode mode;
        const char label[] = "BENCH";
        std::copy(std::begin(label), std::end(label), mode.text.begin());
        const esp_err_t sent = receiver.submit(battery, {500, 2000});
        receiver.submit(gps, {500, 2000});
        receiver.submit(mode, {1000, 2000});
        const auto state = receiver.snapshot();
        ESP_LOGI("telemetry", "%s usable=%d submit=%s sent=%llu dropped=%llu",
                 rclink::name(state.state), state.usable, esp_err_to_name(sent),
                 static_cast<unsigned long long>(state.diagnostics.telemetry_sent),
                 static_cast<unsigned long long>(state.diagnostics.telemetry_dropped));
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
