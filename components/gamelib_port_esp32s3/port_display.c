#include "hal_display.h"
#include "port_config.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"

static const char *TAG = "port_display";
static esp_lcd_panel_handle_t panel_handle = NULL;

static void port_backlight_init(void)
{
    gpio_set_direction(PORT_LCD_PIN_BK_LIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level(PORT_LCD_PIN_BK_LIGHT, 1);

    ledc_timer_config_t ledc_timer = {
        .speed_mode = PORT_LCD_BL_LEDC_MODE,
        .timer_num = PORT_LCD_BL_LEDC_TIMER,
        .duty_resolution = PORT_LCD_BL_DUTY_RES,
        .freq_hz = PORT_LCD_BL_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode = PORT_LCD_BL_LEDC_MODE,
        .channel = PORT_LCD_BL_LEDC_CH,
        .timer_sel = PORT_LCD_BL_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PORT_LCD_PIN_BK_LIGHT,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static void port_backlight_set(uint8_t level)
{
    if (level > 100) level = 100;
    uint32_t duty_max = (1 << PORT_LCD_BL_DUTY_RES) - 1;
    uint32_t duty = (level * duty_max) / 100;
    ledc_set_duty(PORT_LCD_BL_LEDC_MODE, PORT_LCD_BL_LEDC_CH, duty);
    ledc_update_duty(PORT_LCD_BL_LEDC_MODE, PORT_LCD_BL_LEDC_CH);
}

int port_display_init(void)
{
    ESP_LOGI(TAG, "SPI BUS init");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PORT_LCD_PIN_SCLK,
        .mosi_io_num = PORT_LCD_PIN_MOSI,
        .miso_io_num = PORT_LCD_PIN_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = PORT_LCD_H_RES * PORT_LCD_V_RES * 2 + 8,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(PORT_LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PORT_LCD_PIN_DC,
        .cs_gpio_num = PORT_LCD_PIN_CS,
        .pclk_hz = PORT_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = PORT_LCD_CMD_BITS,
        .lcd_param_bits = PORT_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)PORT_LCD_SPI_HOST,
                                             &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PORT_LCD_PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_LOGI(TAG, "Install ST7789 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

    port_backlight_init();
    port_backlight_set(80);
    return 0;
}

void port_display_deinit(void)
{
    if (panel_handle) {
        esp_lcd_panel_disp_on_off(panel_handle, false);
        esp_lcd_panel_del(panel_handle);
        panel_handle = NULL;
    }
}

void port_display_flush(const void *fb, int x, int y, int w, int h)
{
    if (panel_handle) {
        esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + w, y + h, (const void *)fb);
    }
}

void port_display_wait_vsync(void) { }

void port_display_backlight(uint8_t level)
{
    port_backlight_set(level);
}
