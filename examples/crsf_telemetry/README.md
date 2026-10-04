# CRSF telemetry

Send sample battery, GPS, and flight mode data through a CRSF receiver. The serial console shows link status and telemetry counters. Values are fixed test data; the flight mode is `BENCH`.

## Build

With ESP-IDF 6.1 activated, run from the repo root:

```sh
cd examples/crsf_telemetry
idf.py -B ../../build set-target esp32s3 # or esp32c3
idf.py -B ../../build menuconfig
idf.py -B ../../build build
idf.py -B ../../build -p YOUR_SERIAL_PORT flash monitor
```

Both examples share the root `build/` directory. Clear it before switching examples.

## Wiring

Check the UART, GPIOs, and protocol under `Component config → RC link`.

| Chip | ESP32 RX | ESP32 TX |
|---|---|---|
| ESP32-C3 | GPIO5 | GPIO4 |
| ESP32-S3 | GPIO16 | GPIO17 |

Select CRSF or Auto. Connect receiver TX to ESP32 RX, receiver RX to ESP32 TX, and use a common ground.

Use 3.3 V signals on the ESP32 GPIOs. Once the link is usable, check for the sample values on your transmitter.
