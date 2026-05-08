#include "gamelib.h"
#include "port_config.h"

/* port function declarations */
int  port_display_init(void);
void port_display_deinit(void);
void port_display_flush(const void *fb, int x, int y, int w, int h);
void port_display_wait_vsync(void);
void port_display_backlight(uint8_t level);

void port_input_init(void);
void port_input_poll(void);
bool port_input_is_down(hal_key_t key);
bool port_input_is_pressed(hal_key_t key);
bool port_input_is_released(hal_key_t key);
int  port_input_mouse_x(void);
int  port_input_mouse_y(void);
bool port_input_mouse_down(int button);

int  port_audio_init(void);
void port_audio_deinit(void);
void port_audio_beep(int freq_hz, int duration_ms);
void port_audio_stop(void);
bool port_audio_is_busy(void);

void port_timer_init(void);
uint32_t port_timer_millis(void);
uint32_t port_timer_micros(void);
void port_timer_delay_ms(uint32_t ms);
void port_timer_sleep_ms(uint32_t ms);

void gamelib_port_register_hal(void)
{
    g_hal.display.width  = PORT_LCD_H_RES;
    g_hal.display.height = PORT_LCD_V_RES;
    g_hal.display.bpp    = 16;
    g_hal.display.init       = port_display_init;
    g_hal.display.deinit     = port_display_deinit;
    g_hal.display.flush      = port_display_flush;
    g_hal.display.wait_vsync = port_display_wait_vsync;
    g_hal.display.backlight  = port_display_backlight;

    g_hal.input.init       = port_input_init;
    g_hal.input.poll       = port_input_poll;
    g_hal.input.is_down    = port_input_is_down;
    g_hal.input.is_pressed = port_input_is_pressed;
    g_hal.input.is_released = port_input_is_released;
    g_hal.input.mouse_x    = port_input_mouse_x;
    g_hal.input.mouse_y    = port_input_mouse_y;
    g_hal.input.mouse_down = port_input_mouse_down;

    g_hal.audio.init   = port_audio_init;
    g_hal.audio.deinit = port_audio_deinit;
    g_hal.audio.beep   = port_audio_beep;
    g_hal.audio.stop   = port_audio_stop;
    g_hal.audio.is_busy = port_audio_is_busy;

    g_hal.timer.init     = port_timer_init;
    g_hal.timer.millis   = port_timer_millis;
    g_hal.timer.micros   = port_timer_micros;
    g_hal.timer.delay_ms = port_timer_delay_ms;
    g_hal.timer.sleep_ms = port_timer_sleep_ms;
}
