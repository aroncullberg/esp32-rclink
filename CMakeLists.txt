cmake_minimum_required(VERSION 3.22)
idf_component_register(
    SRCS
        "src/types.cpp"
        "src/config.cpp"
        "src/engine.cpp"
        "src/receiver.cpp"
        "src/idf_uart_transport.cpp"
        "src/backends/sbus/sbus_backend.cpp"
        "src/backends/sbus/codec.cpp"
        "src/backends/crsf/crsf_backend.cpp"
        "src/backends/crsf/codec.cpp"
        "src/backends/crsf/telemetry.cpp"
        "src/backends/crsf/third_party/alfredocrsf/crc8.cpp"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES esp_common
    PRIV_REQUIRES esp_driver_uart esp_driver_gpio esp_timer freertos pthread
)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
