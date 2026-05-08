#include "hal_timer.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void port_timer_init(void) { }

uint32_t port_timer_millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

uint32_t port_timer_micros(void)
{
    return (uint32_t)esp_timer_get_time();
}

void port_timer_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void port_timer_sleep_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}
