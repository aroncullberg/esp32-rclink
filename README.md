# esp32-rclink

A C++ SBUS and CRSF receiver component for ESP-IDF 6.1, made for the ESP32-C3 and ESP32-S3.

## Features

- SBUS and CRSF with automatic protocol detection or a fixed protocol.
- 16 channels with raw values, calibrated values, and helpers for sticks and switches.
- Channel timeout, SBUS failsafe flags, and CRSF link statistics.
- CRSF telemetry, including battery, GPS, attitude, and flight mode data.
- UART, GPIOs, channel calibration, and fallback values configurable through Kconfig or C++.

## Examples

- [Receiver monitor](examples/receiver_monitor/README.md): read channels and check link status.
- [CRSF telemetry](examples/crsf_telemetry/README.md): send sample battery, GPS, and flight mode data.

Both examples include build and wiring instructions. The component uses one hardware UART, with UART1 as the default. Check the pins and protocol settings under `Component config → RC link` before connecting a receiver.

## Use as a component

Place this repo in `components/esp32-rclink` or add it to `EXTRA_COMPONENT_DIRS`, then add `REQUIRES esp32-rclink` to your component's CMake registration.

Create a `rclink::Receiver`, call `start()`, and read channels through `snapshot()`. Check the snapshot's `usable` flag before applying inputs; your application is responsible for disarming and setting safe outputs when the link is lost. Test your receiver's failsafe settings, since held channel values can hide radio loss.

Send CRSF telemetry through `submit()`. The public API is in [receiver.hpp](include/rclink/receiver.hpp), with channel and configuration types in [types.hpp](include/rclink/types.hpp) and telemetry types in [telemetry.hpp](include/rclink/telemetry.hpp).

## License

[GPL-3.0-only](LICENSE). The CRSF backend uses CRC code and adapts stream and frame handling from [AlfredoCRSF](https://github.com/AlfredoSystems/AlfredoCRSF), which credits [CapnBry/CRServoF](https://github.com/CapnBry/CRServoF).
