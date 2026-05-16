#include "hal_input.h"
#include "port_config.h"
#include "driver/gpio.h"
#include "bsp_qmi8658.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

/* 配置是否将 IMU 倾斜映射为方向键 (1:是, 0:否) */
#define PORT_MAP_IMU_TO_KEYS 0

static const char *TAG = "port_input";

static struct {
    hal_key_t key;
    int      gpio;
    bool     active_low;
} key_map[] = {
    { KEY_UP,    PORT_KEY_PIN_UP,    true },
    { KEY_DOWN,  PORT_KEY_PIN_DOWN,  true },
    { KEY_LEFT,  PORT_KEY_PIN_LEFT,  true },
    { KEY_RIGHT, PORT_KEY_PIN_RIGHT, true },
    { KEY_A,     PORT_KEY_PIN_A,     true },
    { KEY_B,     PORT_KEY_PIN_B,     true },
};

static bool key_state[KEY_COUNT];
static bool key_prev[KEY_COUNT];
static int mouse_x = 0, mouse_y = 0;
static bool mouse_pressed = false;

void port_input_init(void)
{
    memset(key_state, 0, sizeof(key_state));
    memset(key_prev, 0, sizeof(key_prev));

    for (size_t i = 0; i < sizeof(key_map) / sizeof(key_map[0]); i++) {
        gpio_set_direction(key_map[i].gpio, GPIO_MODE_INPUT);
        gpio_set_pull_mode(key_map[i].gpio, key_map[i].active_low ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY);
    }

    /* Init I2C and QMI8658 accelerometer */
    bsp_qmi8658_init();
}

void port_input_poll(void)
{
    memcpy(key_prev, key_state, sizeof(key_prev));

    for (size_t i = 0; i < sizeof(key_map) / sizeof(key_map[0]); i++) {
        int level = gpio_get_level(key_map[i].gpio);
        key_state[key_map[i].key] = key_map[i].active_low ? (level == 0) : (level == 1);
    }

#if PORT_MAP_IMU_TO_KEYS
    /* QMI8658 tilt mapping — axes swapped for landscape display */
    qmi8658_data_t data;
    bsp_qmi8658_read_data(&data);

    bool tilt_right = (data.acc_y < -PORT_TILT_THRESHOLD);
    bool tilt_left  = (data.acc_y >  PORT_TILT_THRESHOLD);
    bool tilt_down  = (data.acc_x >  PORT_TILT_THRESHOLD);
    bool tilt_up    = (data.acc_x < -PORT_TILT_THRESHOLD);

    key_state[KEY_LEFT]  |= tilt_left;
    key_state[KEY_RIGHT] |= tilt_right;
    key_state[KEY_UP]    |= tilt_up;
    key_state[KEY_DOWN]  |= tilt_down;
#else
    /* Even if not mapping keys, we might need IMU for mouse. */
    qmi8658_data_t data;
    bsp_qmi8658_read_data(&data);
#endif

    /* map tilt to mouse coords (landscape 320x240) */
    mouse_x = 160 + (data.acc_y * 160) / 16000;
    mouse_y = 120 + (data.acc_x * 120) / 16000;
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_x > 319) mouse_x = 319;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_y > 239) mouse_y = 239;

    mouse_pressed = (abs(data.acc_z) > 16000);
}

bool port_input_is_down(hal_key_t key)     { return key_state[key]; }
bool port_input_is_pressed(hal_key_t key)  { return key_state[key] && !key_prev[key]; }
bool port_input_is_released(hal_key_t key) { return !key_state[key] && key_prev[key]; }
int  port_input_mouse_x(void)              { return mouse_x; }
int  port_input_mouse_y(void)              { return mouse_y; }
bool port_input_mouse_down(int button)     { (void)button; return mouse_pressed; }
