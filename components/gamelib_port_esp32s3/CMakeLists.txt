idf_component_register(
    SRCS "port_display.c" "port_input.c" "port_audio.c" "port_timer.c" "bsp_qmi8658.c" "port_hal_init.c"
    INCLUDE_DIRS "."
    REQUIRES gamelib_hal gamelib_core driver esp_timer esp_lcd
)
