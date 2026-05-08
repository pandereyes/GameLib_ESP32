#include "hal_audio.h"
#include "port_config.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "port_audio";
static esp_timer_handle_t beep_timer = NULL;
static bool beeping = false;

static void beep_timer_cb(void *arg)
{
    ledc_set_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH, 0);
    ledc_update_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH);
    beeping = false;
}

int port_audio_init(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = PORT_BUZZER_LEDC_MODE,
        .timer_num = PORT_BUZZER_LEDC_TIMER,
        .duty_resolution = PORT_BUZZER_DUTY_RES,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode = PORT_BUZZER_LEDC_MODE,
        .channel = PORT_BUZZER_LEDC_CH,
        .timer_sel = PORT_BUZZER_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PORT_BUZZER_GPIO,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    const esp_timer_create_args_t timer_args = {
        .callback = beep_timer_cb,
        .name = "beep_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &beep_timer));
    return 44100;
}

void port_audio_deinit(void)
{
    if (beep_timer) {
        esp_timer_stop(beep_timer);
        esp_timer_delete(beep_timer);
        beep_timer = NULL;
    }
}

void port_audio_beep(int freq_hz, int duration_ms)
{
    if (freq_hz <= 0) return;

    ledc_set_freq(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_TIMER, (uint32_t)freq_hz);
    uint32_t duty_max = (1 << PORT_BUZZER_DUTY_RES) - 1;
    ledc_set_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH, duty_max / 2);
    ledc_update_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH);
    beeping = true;

    if (duration_ms > 0 && beep_timer) {
        esp_timer_stop(beep_timer);
        ESP_ERROR_CHECK(esp_timer_start_once(beep_timer, duration_ms * 1000));
    }
}

void port_audio_stop(void)
{
    ledc_set_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH, 0);
    ledc_update_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH);
    beeping = false;
    if (beep_timer) esp_timer_stop(beep_timer);
}

bool port_audio_is_busy(void)
{
    return beeping;
}
