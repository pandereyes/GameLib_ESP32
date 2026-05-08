# GameLib ESP32-S3 移植设计文档

**日期**: 2026-05-08
**状态**: 设计完成，待实施
**范围**: MVE（最小可验证示例）

---

## 1. 项目目标

将 GameLib 游戏引擎的 SDL 版本移植到 ESP32-S3 芯片，基于现有 LVGL demo 开发板实现。
采用分层 HAL 架构，核心库与硬件解耦，方便移植到其他开发板或 MCU。

## 2. 关键决策

| 维度 | 决策 |
|------|------|
| 语言 | C 核心 + C++ 封装可选 |
| 功能范围 | MVE：基础绘图 + 内置8x8文字 + 精灵(BMP加载) + 输入 + PWM蜂鸣 |
| 渲染 | 独立 framebuffer + LVGL Canvas 双后端，通过 HAL 抽象 |
| 输入 | GPIO 物理按键 + QMI8658 加速度计倾斜控制 |
| 音频 | LEDC PWM 蜂鸣器 PlayBeep |
| 项目结构 | ESP-IDF Component 分离 |

## 3. 整体架构

```
main/app_main.c           ← 用户游戏代码
─────────────────────────
gamelib_core              ← 纯逻辑层，零硬件依赖
  gamelib.h/c             - 公开 API / 主循环 / FPS
  framebuffer.c            - 软件 framebuffer 操作 (RGB565)
  draw.c                   - 点/线/圆/椭圆/三角/矩形
  text.c + font_8x8.h     - 内置 8x8 位图字体
  sprite.c                 - 精灵管理 (创建/绘制/释放)
  bmp.c                    - BMP 解码器 (从内存加载)
  audio.c                  - 音频队列管理
─────────────────────────
gamelib_hal               ← HAL 接口定义 (纯头文件)
  hal_types.h              - 颜色类型、键码枚举
  hal_display.h            - 显示操作接口
  hal_input.h              - 输入操作接口
  hal_audio.h              - 音频操作接口
  hal_timer.h              - 定时器接口
─────────────────────────
gamelib_port_esp32s3      ← 开发板具体实现
  port_display.c           - SPI ST7789 (esp_lcd_panel)
  port_input.c             - GPIO 按键 + QMI8658 加速度计
  port_audio.c             - LEDC PWM 蜂鸣器
  port_timer.c             - esp_timer + FreeRTOS
  port_config.h            - 引脚/参数宏定义汇总
─────────────────────────
ESP-IDF 驱动层             ← SPI / GPIO / I2C / LEDC / FreeRTOS
```

### 数据流

```
用户代码 → core 层写 framebuffer (uint16_t[N])
         → core 层调用 hal_display.flush()
         → port 层 esp_lcd_panel_draw_bitmap() DMA 刷屏

用户代码 ← core 层调用 hal_input.poll()
         ← port 层读取 GPIO 电平 + I2C QMI8658
         ← 翻译为标准化键码 (is_down/pressed/released)
```

### 核心约束

- **core 层零外部依赖**：不 include 任何 ESP-IDF 头文件，只依赖 C 标准库
- **HAL 只定义接口 struct**：每个模块一个函数指针结构体，编译期绑定
- **port 层对接硬件**：持有 HAL 实现函数，在 hal_init() 时注册到全局 g_hal
- **core 层通过 `g_hal.display.flush(...)` 方式调用硬件**

## 4. HAL 接口设计

### 4.1 hal_display.h — 显示

```c
typedef struct {
    int   width;
    int   height;
    int   bpp;                        // 16 = RGB565

    int  (*init)(void);
    void (*deinit)(void);
    void (*flush)(const void *fb, int x, int y, int w, int h);
    void (*wait_vsync)(void);
    void (*backlight)(uint8_t level);
} hal_display_t;
```

### 4.2 hal_input.h — 输入

```c
typedef enum {
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_A, KEY_B, KEY_START, KEY_SELECT,
    KEY_COUNT
} hal_key_t;

typedef struct {
    void (*init)(void);
    void (*poll)(void);
    bool (*is_down)(hal_key_t key);
    bool (*is_pressed)(hal_key_t key);
    bool (*is_released)(hal_key_t key);
    int  (*mouse_x)(void);
    int  (*mouse_y)(void);
    bool (*mouse_down)(int button);
} hal_input_t;
```

### 4.3 hal_audio.h — 音频

```c
typedef struct {
    int  (*init)(void);
    void (*deinit)(void);
    void (*beep)(int freq_hz, int duration_ms);
    void (*stop)(void);
    bool (*is_busy)(void);
} hal_audio_t;
```

### 4.4 hal_timer.h — 定时器

```c
typedef struct {
    void     (*init)(void);
    uint32_t (*millis)(void);
    uint32_t (*micros)(void);
    void     (*delay_ms)(uint32_t ms);
    void     (*sleep_ms)(uint32_t ms);
} hal_timer_t;
```

### 4.5 绑定机制

```c
typedef struct {
    hal_display_t display;
    hal_input_t   input;
    hal_audio_t   audio;
    hal_timer_t   timer;
} gamelib_hal_t;

extern gamelib_hal_t g_hal;
// port 层在初始化时填充 g_hal 各字段
// core 层通过 g_hal.display.flush(...) 等方式调用
```

## 5. gamelib_core API

### 5.1 类型

```c
typedef uint16_t gamelib_color_t;       // RGB565
#define MAX_SPRITES 64
```

### 5.2 主上下文

```c
typedef struct {
    framebuffer_t  fb;
    bool           running;
    double         delta_time;
    double         fps;
    int            target_fps;

    sprite_t       sprites[MAX_SPRITES];
    uint8_t        keystate[KEY_COUNT];
    uint8_t        key_prev[KEY_COUNT];
    int            mouse_x, mouse_y;
} gamelib_t;
```

### 5.3 公开 API

**生命周期**:
```c
int    gamelib_init(gamelib_t *g, int w, int h, int target_fps);
void   gamelib_deinit(gamelib_t *g);
bool   gamelib_is_closed(gamelib_t *g);
void   gamelib_update(gamelib_t *g);
void   gamelib_wait_frame(gamelib_t *g);
double gamelib_get_fps(gamelib_t *g);
```

**绘图**:
```c
void gamelib_clear(gamelib_t *g, gamelib_color_t color);
void gamelib_set_pixel(gamelib_t *g, int x, int y, gamelib_color_t c);
void gamelib_draw_line(gamelib_t *g, int x1,int y1,int x2,int y2, gamelib_color_t c);
void gamelib_draw_rect(gamelib_t *g, int x,int y,int w,int h, gamelib_color_t c);
void gamelib_fill_rect(gamelib_t *g, int x,int y,int w,int h, gamelib_color_t c);
void gamelib_draw_circle(gamelib_t *g, int cx,int cy,int r, gamelib_color_t c);
void gamelib_fill_circle(gamelib_t *g, int cx,int cy,int r, gamelib_color_t c);
void gamelib_draw_ellipse(gamelib_t *g, int cx,int cy,int rx,int ry, gamelib_color_t c);
void gamelib_fill_ellipse(gamelib_t *g, int cx,int cy,int rx,int ry, gamelib_color_t c);
void gamelib_draw_triangle(gamelib_t *g, int x1,int y1,int x2,int y2,
                           int x3,int y3, gamelib_color_t c);
void gamelib_fill_triangle(gamelib_t *g, int x1,int y1,int x2,int y2,
                           int x3,int y3, gamelib_color_t c);
```

**裁剪**:
```c
void gamelib_set_clip(gamelib_t *g, int x, int y, int w, int h);
void gamelib_clear_clip(gamelib_t *g);
```

**内置文字 (8x8)**:
```c
void gamelib_draw_text(gamelib_t *g, int x,int y, const char *s, gamelib_color_t c);
void gamelib_draw_text_scale(gamelib_t *g, int x,int y, const char *s,
                             gamelib_color_t c, int scale_w, int scale_h);
void gamelib_draw_number(gamelib_t *g, int x,int y, int n, gamelib_color_t c);
```

**精灵**:
```c
int  gamelib_sprite_create(gamelib_t *g, int w, int h);
int  gamelib_sprite_load_bmp(gamelib_t *g, const uint8_t *data, size_t len);
void gamelib_sprite_free(gamelib_t *g, int id);
void gamelib_sprite_set_pixel(gamelib_t *g, int id, int x,int y, gamelib_color_t c);
void gamelib_draw_sprite(gamelib_t *g, int id, int x,int y);
void gamelib_draw_sprite_region(gamelib_t *g, int id, int x,int y,
                                int sx,int sy,int sw,int sh);
int  gamelib_sprite_width(gamelib_t *g, int id);
int  gamelib_sprite_height(gamelib_t *g, int id);
void gamelib_sprite_set_color_key(gamelib_t *g, int id, gamelib_color_t c);
```

**输入**:
```c
bool gamelib_is_key_down(gamelib_t *g, int key);
bool gamelib_is_key_pressed(gamelib_t *g, int key);
bool gamelib_is_key_released(gamelib_t *g, int key);
int  gamelib_mouse_x(gamelib_t *g);
int  gamelib_mouse_y(gamelib_t *g);
```

**音频**:
```c
void gamelib_play_beep(gamelib_t *g, int freq, int duration_ms);
void gamelib_stop_beep(gamelib_t *g);
```

### 5.4 关键设计决策

- 每个 API 第一个参数是 `gamelib_t*`，替代 C++ 的 `this`，支持多实例
- framebuffer 格式 RGB565 跟随 HAL 报告的 bpp，一次 DMA 提交无转换
- 精灵表使用固定数组（MAX_SPRITES=64），避免动态内存碎片
- 绘制算法复用 Bresenham、中点圆等经典实现，与 GameLib.SDL.h 数学行为一致
- BMP 解码器从内存加载（`const uint8_t *data, size_t len`），资源文件通过 xxd 转换为 C 数组嵌入

## 6. gamelib_port_esp32s3

### 6.1 port_display.c — ST7789 SPI

- SPI2_HOST，SCLK=39，MOSI=38，MISO=40
- ST7789 240×320，RGB565 (bpp=16)
- Pixel clock 80MHz
- `flush()` 调用 `esp_lcd_panel_draw_bitmap()` 提交 framebuffer
- `backlight()` 通过 LEDC PWM 控制 GPIO 1
- 初始化流程: `spi_bus_initialize → esp_lcd_new_panel_io_spi → esp_lcd_new_panel_st7789 → panel_init`

### 6.2 port_input.c — GPIO 按键 + QMI8658

- 按键 GPIO: UP=4, DOWN=5, LEFT=6, RIGHT=7, A=8, B=9
- `poll()` 每帧读取 GPIO 电平，维护当前/上一帧状态表
- QMI8658 通过 I2C0 (SDA=48, SCL=47) 读取加速度数据
- 倾斜阈值映射: acc_x < -阈值 → KEY_LEFT, acc_x > 阈值 → KEY_RIGHT 等
- `mouse_x/y()` 由倾斜角度线性映射到屏幕坐标

### 6.3 port_audio.c — LEDC PWM 蜂鸣

- LEDC PWM 输出到蜂鸣器 GPIO
- `beep(freq, duration)`: 设置 PWM 频率=贝奥频率，占空比 50%
- 使用 esp_timer 回调自动停止
- `is_busy()` 检查计时器是否活跃

### 6.4 port_timer.c — 时间服务

- `millis()`: `xTaskGetTickCount() * portTICK_PERIOD_MS` (或使用 esp_timer_get_time()/1000)
- `micros()`: `esp_timer_get_time()`
- `sleep_ms()`: `vTaskDelay(pdMS_TO_TICKS(ms))`

## 7. 目录结构

```
gamelib_for_esp32/
├── CMakeLists.txt
├── sdkconfig / sdkconfig.defaults
├── main/
│   ├── CMakeLists.txt
│   ├── main.c
│   └── assets/
│       └── player.bmp.c           # xxd -i 嵌入
└── components/
    ├── gamelib_hal/
    │   ├── CMakeLists.txt
    │   ├── hal_display.h
    │   ├── hal_input.h
    │   ├── hal_audio.h
    │   ├── hal_timer.h
    │   └── hal_types.h
    ├── gamelib_core/
    │   ├── CMakeLists.txt
    │   ├── gamelib.h
    │   ├── gamelib.c
    │   ├── framebuffer.c
    │   ├── draw.c
    │   ├── text.c
    │   ├── font_8x8.h
    │   ├── sprite.c
    │   ├── bmp.c
    │   └── audio.c
    └── gamelib_port_esp32s3/
        ├── CMakeLists.txt
        ├── port_display.c
        ├── port_input.c
        ├── port_audio.c
        ├── port_timer.c
        └── port_config.h
```

## 8. 构建系统

### CMakeLists.txt 依赖链

```
main/CMakeLists.txt
  └── REQUIRES gamelib_core gamelib_port_esp32s3

gamelib_core/CMakeLists.txt
  └── REQUIRES gamelib_hal

gamelib_port_esp32s3/CMakeLists.txt
  └── REQUIRES gamelib_hal driver esp_timer esp_lcd

gamelib_hal/CMakeLists.txt
  └── 纯头文件，无依赖
```

### 编译命令

```bash
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

## 9. 第一期不包含 (放入后续迭代)

- TTF 字体渲染 (SDL2_ttf 对应)
- PNG/JPG 图片加载 (SDL2_image 对应)
- 精灵缩放/旋转 (DrawSpriteScaled / DrawSpriteRotated)
- Tilemap (CreateTilemap / DrawTilemap)
- UI 控件 (Button / Checkbox / RadioBox / ToggleButton)
- 场景管理 (SetScene / GetScene)
- WAV 音乐播放 (PlayWAV / PlayMusic)
- 文件系统 HAL 接口
- C++ 封装层
- LVGL Canvas 后端 (先做 framebuffer 直刷)

## 10. 测试策略

- **单元测试**: draw.c 绘图算法可脱离硬件在 PC 上编译验证（纯 C 无依赖）
- **集成测试**: 在 ESP32-S3 开发板上跑 MVE 示例程序，验证渲染/输入/音频全部通路
- **MVE 示例内容**: 屏幕显示文字 + 移动的 BMP 精灵 + 按键控制方向 + 按键触发蜂鸣
