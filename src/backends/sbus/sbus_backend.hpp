// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#pragma once
#include "backends/backend.hpp"
#include "codec.hpp"
namespace rclink::detail {
class SbusBackend final : public Backend {
    std::array<uint8_t, 25> buffer_{};
    size_t used_{0};
    Calibration calibration_{192, 992, 1792};
    BackendDiagnostics diagnostics_;

  public:
    Protocol protocol() const noexcept override {
        return Protocol::Sbus;
    }
    UartSettings settings(const Config &config) const noexcept override {
        return {100000, true, true, config.sbus_inverted};
    }
    void configure(const Config &config) noexcept override {
        calibration_ = config.sbus;
    }
    void reset() noexcept override {
        used_ = 0;
    }
    bool feed(uint8_t byte, uint64_t now, BackendEvent &event) override;
    BackendDiagnostics take_diagnostics() noexcept override {
        auto d = diagnostics_;
        diagnostics_ = {};
        return d;
    }
};
} // namespace rclink::detail
