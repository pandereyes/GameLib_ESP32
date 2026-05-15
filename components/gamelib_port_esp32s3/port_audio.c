#include "hal_audio.h"
#include "port_config.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <string.h>

#ifdef PORT_AUDIO_USE_I2S
#include "driver/i2s.h"
#endif

static const char *TAG = "port_audio";
static esp_timer_handle_t beep_timer = NULL;
static bool beeping = false;
static int master_volume = 1000;
static bool pcm_playing = false;

/* ============== Beeper (unchanged logic) ============== */

static void beep_timer_cb(void *arg)
{
    ledc_set_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH, 0);
    ledc_update_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH);
    beeping = false;
}

/* ============== PWM DAC (default) ============== */

#ifndef PORT_AUDIO_USE_I2S

#define PWM_DAC_SAMPLE_RATE 22050

static const uint8_t *pcm_data;
static int pcm_len;
static int pcm_pos;
static int pcm_bits;
static int pcm_channels;
static esp_timer_handle_t pcm_timer = NULL;
static uint32_t pcm_fractional_pos = 0;
static uint32_t pcm_step = 0;

/* forward declarations */
void port_audio_stop_pcm(void);
bool port_audio_is_pcm_playing(void);

static void pcm_timer_cb(void *arg)
{
    if (!pcm_playing || pcm_pos >= pcm_len) {
        ledc_set_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH, 0);
        ledc_update_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH);
        if (pcm_pos >= pcm_len) pcm_playing = false;
        return;
    }
    uint16_t sample = 128; // default middle
    if (pcm_bits == 8) {
        sample = pcm_data[pcm_pos];
    } else if (pcm_bits == 16) {
        /* PCM 16-bit is signed little-endian, map to 0-255 */
        if (pcm_pos + 1 < pcm_len) {
            int16_t s = (int16_t)(pcm_data[pcm_pos] | (pcm_data[pcm_pos + 1] << 8));
            sample = (uint16_t)((s + 32768) >> 8);
        }
    } 
    
    /* Advance pcm_pos properly (skipping samples if timer_hz < sample_rate) */
    pcm_fractional_pos += pcm_step;
    uint32_t steps = pcm_fractional_pos >> 12;
    pcm_fractional_pos &= 0xFFF;
    
    if (pcm_bits == 8) pcm_pos += steps * pcm_channels;
    else if (pcm_bits == 16) pcm_pos += steps * pcm_channels * 2;
    else pcm_pos += steps;

    uint32_t duty = ((uint32_t)sample * 4) * master_volume / 1000;
    ledc_set_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH, duty);
    ledc_update_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH);
}

int port_audio_play_pcm(const uint8_t *data, int len, int sample_rate, int bits, int channels)
{
    port_audio_stop_pcm();
    pcm_data = data;
    pcm_len = len;
    pcm_pos = 0;
    pcm_bits = bits;
    pcm_channels = channels;
    pcm_playing = true;

    /* Cap the timer frequency to ~11KHz to save CPU and restore game FPS! */
    uint32_t timer_hz = sample_rate;
    if (timer_hz > 11025) timer_hz = 11025;
    /* Fixed point step size for advancing the PCM data array */
    pcm_step = (sample_rate << 12) / timer_hz;
    pcm_fractional_pos = 0;

    /* Set a high carrier frequency for PWM DAC */
    ledc_set_freq(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_TIMER, 64000);

    const esp_timer_create_args_t timer_args = {
        .callback = pcm_timer_cb, 
        .name = "pcm_timer",
        .skip_unhandled_events = true
    };
    if (!pcm_timer) esp_timer_create(&timer_args, &pcm_timer);
    esp_timer_start_periodic(pcm_timer, 1000000 / timer_hz);
    return 0;
}

void port_audio_stop_pcm(void)
{
    pcm_playing = false;
    if (pcm_timer) {
        esp_timer_stop(pcm_timer);
        esp_timer_delete(pcm_timer);
        pcm_timer = NULL;
    }
    ledc_set_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH, 0);
    ledc_update_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH);
}

bool port_audio_is_pcm_playing(void) { return pcm_playing; }

/* ============== I2S DAC ============== */

#else

/* forward declarations */
void port_audio_stop_pcm(void);
bool port_audio_is_pcm_playing(void);

int port_audio_play_pcm(const uint8_t *data, int len, int sample_rate, int bits, int channels)
{
    port_audio_stop_pcm();

    i2s_config_t i2s_cfg = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = (uint32_t)sample_rate,
        .bits_per_sample = (i2s_bits_per_sample_t)bits,
        .channel_format = (channels == 2) ? I2S_CHANNEL_FMT_RIGHT_LEFT : I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 512,
        .use_apll = false,
    };
    i2s_driver_install(PORT_I2S_NUM, &i2s_cfg, 0, NULL);

    i2s_pin_config_t pin_cfg = {
        .bck_io_num = PORT_I2S_BCK_PIN,
        .ws_io_num  = PORT_I2S_WS_PIN,
        .data_out_num = PORT_I2S_DOUT_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE,
    };
    i2s_set_pin(PORT_I2S_NUM, &pin_cfg);

    size_t written;
    pcm_playing = true;
    i2s_write(PORT_I2S_NUM, data, (size_t)len, &written, portMAX_DELAY);
    pcm_playing = false;
    i2s_driver_uninstall(PORT_I2S_NUM);
    return 0;
}

void port_audio_stop_pcm(void) { pcm_playing = false; }

bool port_audio_is_pcm_playing(void) { return pcm_playing; }

#endif /* PORT_AUDIO_USE_I2S */

/* ============== Volume ============== */

static void port_audio_set_volume(int vol)
{
    if (vol < 0) vol = 0;
    if (vol > 1000) vol = 1000;
    master_volume = vol;
}

/* ============== Init / Deinit ============== */

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
        .callback = beep_timer_cb, .name = "beep_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &beep_timer));

#ifdef PORT_AUDIO_USE_I2S
    return 44100;
#else
    return PWM_DAC_SAMPLE_RATE;
#endif
}

void port_audio_deinit(void)
{
    if (beep_timer) {
        esp_timer_stop(beep_timer);
        esp_timer_delete(beep_timer);
        beep_timer = NULL;
    }
#ifndef PORT_AUDIO_USE_I2S
    if (pcm_timer) {
        esp_timer_stop(pcm_timer);
        esp_timer_delete(pcm_timer);
        pcm_timer = NULL;
    }
#else
    i2s_driver_uninstall(PORT_I2S_NUM);
#endif
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

bool port_audio_is_busy(void) { return beeping; }
