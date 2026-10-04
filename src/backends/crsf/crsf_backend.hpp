// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#pragma once
#include "backends/backend.hpp"
#include "codec.hpp"
namespace rclink::detail {
class CrsfBackend final : public Backend {
    std::array<uint8_t, 64> buffer_{};
    size_t used_{0};
    Calibration calibration_{172, 992, 1810};
    BackendDiagnostics diagnostics_;
    void shift(size_t n);
    bool parse(uint8_t byte, crsf::Packet &packet);

  public:
    Protocol protocol() const noexcept override {
        return Protocol::Crsf;
    }
    UartSettings settings(const Config &config) const noexcept override {
        return {config.crsf_baud, false, false, false};
    }
    void configure(const Config &config) noexcept override {
        calibration_ = config.crsf;
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
