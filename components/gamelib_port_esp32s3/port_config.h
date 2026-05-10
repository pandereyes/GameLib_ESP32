#ifndef PORT_CONFIG_H
#define PORT_CONFIG_H

/* === Display: ST7789 via SPI2 === */
#define PORT_LCD_SPI_HOST       SPI2_HOST
#define PORT_LCD_PIN_SCLK       39
#define PORT_LCD_PIN_MOSI       38
#define PORT_LCD_PIN_MISO       40
#define PORT_LCD_PIN_DC         42
#define PORT_LCD_PIN_RST        -1
#define PORT_LCD_PIN_CS         45
#define PORT_LCD_PIXEL_CLOCK_HZ (80 * 1000 * 1000)
#define PORT_LCD_CMD_BITS       8
#define PORT_LCD_PARAM_BITS     8
#define PORT_LCD_H_RES          240
#define PORT_LCD_V_RES          320

/* === Backlight === */
#define PORT_LCD_PIN_BK_LIGHT   1
#define PORT_LCD_BL_LEDC_TIMER  LEDC_TIMER_0
#define PORT_LCD_BL_LEDC_MODE   LEDC_LOW_SPEED_MODE
#define PORT_LCD_BL_LEDC_CH     LEDC_CHANNEL_0
#define PORT_LCD_BL_DUTY_RES    LEDC_TIMER_10_BIT
#define PORT_LCD_BL_FREQ_HZ     10000

/* === Input: GPIO Keys === */
#define PORT_KEY_PIN_UP         2
#define PORT_KEY_PIN_DOWN       4
#define PORT_KEY_PIN_LEFT       6
#define PORT_KEY_PIN_RIGHT      16
#define PORT_KEY_PIN_A          8
#define PORT_KEY_PIN_B          18

/* === Input: QMI8658 (I2C) === */
#define PORT_I2C_NUM            I2C_NUM_0
#define PORT_I2C_PIN_SDA        48
#define PORT_I2C_PIN_SCL        47
#define PORT_TILT_THRESHOLD     500    /* raw accel threshold */

/* === Audio: PWM Buzzer === */
#define PORT_BUZZER_GPIO        21
#define PORT_BUZZER_LEDC_TIMER  LEDC_TIMER_1
#define PORT_BUZZER_LEDC_MODE   LEDC_LOW_SPEED_MODE
#define PORT_BUZZER_LEDC_CH     LEDC_CHANNEL_2
#define PORT_BUZZER_DUTY_RES    LEDC_TIMER_10_BIT

/* === Audio Mode: PWM DAC (default) or I2S DAC === */
/* #define PORT_AUDIO_USE_I2S  ← uncomment for I2S DAC */

/* I2S pins (only used when PORT_AUDIO_USE_I2S is defined) */
#define PORT_I2S_BCK_PIN   7
#define PORT_I2S_WS_PIN    5
#define PORT_I2S_DOUT_PIN  9
#define PORT_I2S_NUM       I2S_NUM_0

#endif
