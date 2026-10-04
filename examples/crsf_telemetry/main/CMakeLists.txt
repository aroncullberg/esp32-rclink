get_filename_component(RCLINK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
get_filename_component(RCLINK_COMPONENT_NAME "${RCLINK_ROOT}" NAME)
idf_component_register(SRCS "main.cpp" INCLUDE_DIRS "."
    REQUIRES ${RCLINK_COMPONENT_NAME} freertos esp_timer)
