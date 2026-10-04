# Receiver monitor

Print channels and link status from an SBUS or CRSF receiver to the serial console.

## Build

With ESP-IDF 6.1 activated, run from the repo root:

```sh
cd examples/receiver_monitor
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

Connect receiver TX to ESP32 RX and use a common ground. SBUS only needs this connection, but the component also reserves the TX GPIO. For CRSF telemetry, connect receiver RX to ESP32 TX too.

Use 3.3 V signals on the ESP32 GPIOs.
