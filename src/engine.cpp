// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Aron Cullberg
#include "engine.hpp"
namespace rclink::detail {
void Engine::transition(LinkState next_state, uint64_t now) {
    if (state_.state == next_state) {
        return;
    }
    const bool was_usable = state_.usable;
    state_.state = next_state;
    state_.usable = next_state == LinkState::Active;
    state_.transition_us = now;
    ++state_.diagnostics.transitions;
    const bool timed_out = next_state == LinkState::ChannelTimeout;
    const bool receiver_failed = next_state == LinkState::ReceiverFailsafe;
    const bool rf_lost = next_state == LinkState::RfLoss;
    const bool transport_failed = next_state == LinkState::TransportError;
    if (timed_out || receiver_failed || rf_lost || transport_failed) {
        state_.last_failure = next_state;
        state_.last_failure_us = now;
        ++state_.diagnostics.failures;
    }
    if (state_.usable && !was_usable) {
        state_.last_recovery_us = now;
        ++state_.diagnostics.recoveries;
    }
}

void Engine::start(const Config &config, uint64_t now) {
    config_ = config;
    state_ = Snapshot{};
    crsf_ = CrsfBackend{};
    sbus_ = SbusBackend{};
    crsf_.configure(config);
    sbus_.configure(config);
    ping_pending_ = false;
    running_ = true;
    has_good_ = false;
    rf_blocked_ = false;
    detected_ = 0;
    healthy_ = 0;
    last_byte_ = 0;
    probe_start_ = now;
    candidate_ = config.protocol == Protocol::Sbus ? Protocol::Sbus : Protocol::Crsf;
    locked_ = config.protocol != Protocol::Auto;
    state_.protocol = locked_ ? candidate_ : Protocol::Auto;
    for (size_t i = 0; i < Channels::size(); ++i) {
        Channel &channel = *state_.channels.at(i);
        channel.low_threshold = config.low_threshold;
        channel.high_threshold = config.high_threshold;
        channel.deadband = config.deadband;

        Channel &fallback = *state_.fallback_channels.at(i);
        fallback = channel;
        fallback.scaled = config.fallback[i];
    }
    transition(locked_ ? LinkState::NoData : LinkState::Detecting, now);
}

void Engine::stop(uint64_t now) {
    running_ = false;
    transition(LinkState::Stopped, now);
    crsf_.reset();
    sbus_.reset();
}

void Engine::tick(uint64_t now) {
    if (!running_ || state_.state == LinkState::TransportError) {
        return;
    }

    expire_partial_frame(now);
    advance_probe(now);
    evaluate_channel_timeout(now);
    update_rf_freshness(now);

    // Keep RF loss latched until the receiver reports nonzero link quality.
    if (locked_ && rf_blocked_) {
        healthy_ = 0;
        transition(LinkState::RfLoss, now);
    }
}

void Engine::expire_partial_frame(uint64_t now) {
    if (last_byte_ == 0) {
        return;
    }

    const uint64_t byte_age_us = now - last_byte_;
    const uint64_t parser_gap_us = uint64_t(config_.parser_gap_ms) * 1000;
    if (byte_age_us < parser_gap_us) {
        return;
    }

    crsf_.reset();
    sbus_.reset();
    last_byte_ = 0;
}

void Engine::advance_probe(uint64_t now) {
    if (locked_) {
        return;
    }

    const uint64_t probe_age_us = now - probe_start_;
    const uint64_t probe_duration_us = uint64_t(config_.probe_ms) * 1000;
    if (probe_age_us < probe_duration_us) {
        return;
    }

    candidate_ = candidate_ == Protocol::Crsf ? Protocol::Sbus : Protocol::Crsf;
    probe_start_ = now;
    detected_ = 0;
    healthy_ = 0;
    has_good_ = false;
    state_.sequence = 0;
    state_.has_crsf_status = false;
    state_.rf = RfStatistics{};
    rf_blocked_ = false;
    crsf_.reset();
    sbus_.reset();
    transition(LinkState::Detecting, now);
}

void Engine::evaluate_channel_timeout(uint64_t now) {
    if (rf_blocked_ || !locked_ || state_.sequence == 0) {
        return;
    }

    const uint64_t channel_age_us = now - state_.last_good_us;
    const uint64_t timeout_us = uint64_t(config_.timeout_ms) * 1000;
    if (has_good_ && channel_age_us < timeout_us) {
        return;
    }

    healthy_ = 0;
    if (state_.state == LinkState::ReceiverFailsafe) {
        return;
    }

    transition(LinkState::ChannelTimeout, now);
}

void Engine::update_rf_freshness(uint64_t now) {
    const uint64_t rf_age_us = now - state_.rf.timestamp_us;
    const uint64_t rf_timeout_us = uint64_t(config_.rf_timeout_ms) * 1000;
    state_.rf.fresh = state_.rf.available && rf_age_us < rf_timeout_us;
}

void Engine::channels(const std::array<ChannelSample, 16> &samples, bool lost, bool failsafe,
                      uint64_t now) {
    ++state_.sequence;
    ++state_.diagnostics.channel_frames;
    state_.last_channel_us = now;
    state_.frame_lost = lost;
    state_.receiver_failsafe = failsafe;
    if (!lost) {
        for (size_t i = 0; i < Channels::size(); ++i) {
            Channel &channel = *state_.channels.at(i);
            channel.raw = samples[i].raw;
            channel.scaled = samples[i].scaled;
        }
    }
    if (!locked_) {
        if (++detected_ < config_.detection_frames) {
            return;
        }
        locked_ = true;
        state_.protocol = candidate_;
    }
    if (failsafe) {
        healthy_ = 0;
        transition(LinkState::ReceiverFailsafe, now);
        return;
    }
    // Lost frames do not refresh the channel timeout.
    if (lost) {
        healthy_ = 0;
        return;
    }
    has_good_ = true;
    state_.last_good_us = now;
    if (rf_blocked_) {
        healthy_ = 0;
        transition(LinkState::RfLoss, now);
        return;
    }
    if (state_.state == LinkState::Active) {
        return;
    }
    if (++healthy_ >= config_.recovery_frames) {
        transition(LinkState::Active, now);
    } else {
        transition(LinkState::Recovering, now);
    }
}

void Engine::feed(const uint8_t *data, size_t size, uint64_t now) {
    tick(now);
    if (!running_ || state_.state == LinkState::TransportError) {
        return;
    }
    Backend &selected = backend();
    for (size_t i = 0; i < size; ++i) {
        BackendEvent event;
        if (!selected.feed(data[i], now, event)) {
            continue;
        }
        switch (event.kind) {
        case BackendEvent::Kind::Channels:
            state_.digital17 = event.digital17;
            state_.digital18 = event.digital18;
            state_.has_crsf_status = event.has_status;
            state_.crsf_status = event.status;
            channels(event.channels, event.frame_lost, event.failsafe, now);
            break;
        case BackendEvent::Kind::RfStatistics:
            state_.rf = event.rf;
            if (config_.failsafe_on_zero_lq) {
                rf_blocked_ = state_.rf.quality == 0;
                if (locked_ && rf_blocked_) {
                    healthy_ = 0;
                    transition(LinkState::RfLoss, now);
                }
            }
            break;
        case BackendEvent::Kind::Discovery:
            ping_pending_ = true;
            ping_origin_ = event.discovery_origin;
            break;
        }
    }
    last_byte_ = now;
    const auto diagnostics = selected.take_diagnostics();
    state_.diagnostics.malformed_frames += diagnostics.malformed;
    state_.diagnostics.crc_errors += diagnostics.crc_errors;
}

void Engine::discontinuity() {
    crsf_.reset();
    sbus_.reset();
    healthy_ = 0;
    if (!locked_) {
        detected_ = 0;
    }
}

void Engine::overflow(uint64_t now) {
    ++state_.diagnostics.overflows;
    discontinuity();
    tick(now);
}

void Engine::transport_error(uint64_t now) {
    healthy_ = 0;
    transition(LinkState::TransportError, now);
}
} // namespace rclink::detail
