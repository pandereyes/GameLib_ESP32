# GameLib ESP32-S3 Batch3: WAV Audio System

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans.

**Goal:** 新增 WAV 解码/播放、4通道混音、PWM/I2S 双后端、文件系统 HAL。

**Architecture:** 新建 wav.c 解码器 + hal_fs.h 文件系统抽象；hal_audio.h 扩展 PCM/音量接口；port_audio.c 重构为双后端（宏切换）；audio.c 重写为通道管理器。WAV 文件通过 xxd -i 嵌入固件。

**Tech Stack:** C11, ESP-IDF (LEDC/I2S), PWM DAC / I2S DAC

---

### Task 1: 新建 HAL 头文件 (hal_fs.h + 扩展 hal_audio.h)

**Files:**
- Create: `components/gamelib_hal/hal_fs.h`
- Modify: `components/gamelib_hal/hal_audio.h`

- [ ] **Step 1: 创建 hal_fs.h**

```c
#ifndef HAL_FS_H
#define HAL_FS_H

#include <stdint.h>

typedef struct {
    int   (*init)(void);
    void* (*open)(const char *path);
    int   (*read)(void *file, uint8_t *buf, int len);
    int   (*size)(void *file);
    void  (*close)(void *file);
} hal_fs_t;

#endif
```

- [ ] **Step 2: 扩展 hal_audio.h — 在现有 hal_audio_t 结构体中新增 PCM 和音量回调**

找到当前 hal_audio_t 定义，在 `bool (*is_busy)(void);` 之后、`}` 之前插入：

```c
    /* PCM output */
    int  (*play_pcm)(const uint8_t *data, int len, int sample_rate, int bits, int channels);
    void (*stop_pcm)(void);
    bool (*is_pcm_playing)(void);
    void (*set_volume)(int vol);
```

最终结构体：

```c
typedef struct {
    int  (*init)(void);
    void (*deinit)(void);
    void (*beep)(int freq_hz, int duration_ms);
    void (*stop)(void);
    bool (*is_busy)(void);
    /* PCM output */
    int  (*play_pcm)(const uint8_t *data, int len, int sample_rate, int bits, int channels);
    void (*stop_pcm)(void);
    bool (*is_pcm_playing)(void);
    void (*set_volume)(int vol);
} hal_audio_t;
```

---

### Task 2: 新建 WAV 解码器 (wav.h + wav.c)

**Files:**
- Create: `components/gamelib_core/wav.h`
- Create: `components/gamelib_core/wav.c`

- [ ] **Step 1: 创建 wav.h**

```c
#ifndef WAV_H
#define WAV_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    int sample_rate;
    int bits_per_sample;
    int num_channels;
    int data_len;
    const uint8_t *data;    /* pointer into PCM data (after header) */
} wav_info_t;

/* Parse WAV header. Returns 0 on success.
   On return, info->data points to PCM samples, info->data_len is byte count. */
int wav_parse(const uint8_t *wav_data, size_t len, wav_info_t *info);

#endif
```

- [ ] **Step 2: 创建 wav.c — WAV 头解析**

```c
#include "wav.h"
#include <string.h>

int wav_parse(const uint8_t *wav_data, size_t len, wav_info_t *info)
{
    if (!wav_data || len < 44 || !info) return -1;
    memset(info, 0, sizeof(*info));

    /* RIFF header */
    if (memcmp(wav_data, "RIFF", 4) != 0) return -1;
    if (memcmp(wav_data + 8, "WAVE", 4) != 0) return -1;

    /* Find fmt chunk */
    int pos = 12;
    while (pos < (int)len - 8) {
        char id[5] = {0};
        memcpy(id, wav_data + pos, 4);
        int chunk_size = wav_data[pos + 4] | (wav_data[pos + 5] << 8) |
                         (wav_data[pos + 6] << 16) | (wav_data[pos + 7] << 24);

        if (strcmp(id, "fmt ") == 0) {
            int audio_fmt = wav_data[pos + 8] | (wav_data[pos + 9] << 8);
            if (audio_fmt != 1) return -1; /* PCM only */
            info->num_channels = wav_data[pos + 10] | (wav_data[pos + 11] << 8);
            info->sample_rate  = wav_data[pos + 12] | (wav_data[pos + 13] << 8) |
                                  (wav_data[pos + 14] << 16) | (wav_data[pos + 15] << 24);
            info->bits_per_sample = wav_data[pos + 22] | (wav_data[pos + 23] << 8);
        } else if (strcmp(id, "data") == 0) {
            info->data = wav_data + pos + 8;
            info->data_len = chunk_size;
            break;
        }
        pos += 8 + chunk_size;
    }

    if (!info->data || info->data_len == 0) return -1;
    return 0;
}
```

---

### Task 3: 新建 port_fs.c + 修改 port_config.h

**Files:**
- Create: `components/gamelib_port_esp32s3/port_fs.c`
- Modify: `components/gamelib_port_esp32s3/port_config.h`

- [ ] **Step 1: 创建 port_fs.c — 嵌入式数组文件系统（空实现）**

```c
#include "hal_fs.h"

static int fs_init(void) { return 0; }
static void* fs_open(const char *path) { (void)path; return NULL; }
static int fs_read(void *file, uint8_t *buf, int len) { (void)file; (void)buf; (void)len; return 0; }
static int fs_size(void *file) { (void)file; return 0; }
static void fs_close(void *file) { (void)file; }

hal_fs_t g_hal_fs = {
    .init  = fs_init,
    .open  = fs_open,
    .read  = fs_read,
    .size  = fs_size,
    .close = fs_close
};
```

- [ ] **Step 2: 在 port_config.h 末尾添加音频模式配置**

在文件末尾 `#endif` 之前添加：

```c
/* === Audio Mode: PWM DAC (default) or I2S DAC === */
/* #define PORT_AUDIO_USE_I2S  ← uncomment for I2S DAC */

/* I2S pins (only used when PORT_AUDIO_USE_I2S is defined) */
#define PORT_I2S_BCK_PIN   7
#define PORT_I2S_WS_PIN    5
#define PORT_I2S_DOUT_PIN  9
#define PORT_I2S_NUM       I2S_NUM_0
```

---

### Task 4: 重构 port_audio.c — 双后端

**Files:**
- Modify: `components/gamelib_port_esp32s3/port_audio.c`

完整重写为双后端切换。保留原有 beep 逻辑，新增 PCM 函数（PWM 和 I2S 两个实现）。

```c
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

/* ============== Beeper (unchanged) ============== */

static void beep_timer_cb(void *arg)
{
    ledc_set_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH, 0);
    ledc_update_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH);
    beeping = false;
}

/* ============== PWM DAC ============== */

#ifndef PORT_AUDIO_USE_I2S

#define PWM_DAC_SAMPLE_RATE 22050
#define PWM_DAC_RESOLUTION  8

static const uint8_t *pcm_data;
static int pcm_len;
static int pcm_pos;
static int pcm_sample_rate;
static esp_timer_handle_t pcm_timer = NULL;

static void pcm_timer_cb(void *arg)
{
    if (!pcm_playing || pcm_pos >= pcm_len) {
        if (pcm_pos >= pcm_len) {
            ledc_set_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH, 0);
            ledc_update_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH);
            pcm_playing = false;
        }
        return;
    }
    uint8_t sample = pcm_data[pcm_pos++];
    uint32_t duty = (uint32_t)sample * master_volume / 1000;
    ledc_set_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH, duty);
    ledc_update_duty(PORT_BUZZER_LEDC_MODE, PORT_BUZZER_LEDC_CH);
}

static int port_audio_play_pcm(const uint8_t *data, int len, int sample_rate, int bits, int channels)
{
    (void)bits; (void)channels;
    port_audio_stop_pcm();
    pcm_data = data;
    pcm_len = len;
    pcm_pos = 0;
    pcm_sample_rate = sample_rate;
    pcm_playing = true;

    const esp_timer_create_args_t timer_args = {
        .callback = pcm_timer_cb, .name = "pcm_timer"
    };
    if (!pcm_timer) esp_timer_create(&timer_args, &pcm_timer);
    esp_timer_start_periodic(pcm_timer, 1000000 / sample_rate);
    return 0;
}

static void port_audio_stop_pcm(void)
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

static bool port_audio_is_pcm_playing(void) { return pcm_playing; }

/* ============== I2S DAC ============== */

#else

static int port_audio_play_pcm(const uint8_t *data, int len, int sample_rate, int bits, int channels)
{
    port_audio_stop_pcm();

    i2s_config_t i2s_cfg = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = sample_rate,
        .bits_per_sample = bits,
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
    i2s_write(PORT_I2S_NUM, data, len, &written, portMAX_DELAY);
    i2s_driver_uninstall(PORT_I2S_NUM);
    return 0;
}

static void port_audio_stop_pcm(void)
{
    i2s_zero_dma_buffer(PORT_I2S_NUM);
    i2s_stop(PORT_I2S_NUM);
    i2s_driver_uninstall(PORT_I2S_NUM);
}

static bool port_audio_is_pcm_playing(void) { return pcm_playing; }

#endif /* PORT_AUDIO_USE_I2S */

/* ============== Volume ============== */

static void port_audio_set_volume(int vol)
{
    if (vol < 0) vol = 0;
    if (vol > 1000) vol = 1000;
    master_volume = vol;
}

/* ============== Init / Deinit (adapted for dual mode) ============== */

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
```

---

### Task 5: 更新 gamelib.h 声明 + 重写 audio.c

**Files:**
- Modify: `components/gamelib_core/gamelib.h`
- Modify: `components/gamelib_core/audio.c`

- [ ] **Step 1: gamelib.h — 在现有音频声明区域新增 WAV/音量/音乐 API**

在 `gamelib_play_beep` / `gamelib_stop_beep` 声明之后，插入：

```c
/* --- wav audio --- */
int  gamelib_play_wav(gamelib_t *g, const uint8_t *wav_data, int channel, int volume);
void gamelib_stop_wav(gamelib_t *g, int channel);
void gamelib_stop_all_wav(gamelib_t *g);
bool gamelib_is_wav_playing(gamelib_t *g, int channel);
void gamelib_set_wav_volume(gamelib_t *g, int channel, int volume);
void gamelib_set_master_volume(gamelib_t *g, int volume);
int  gamelib_get_master_volume(gamelib_t *g);

/* --- music --- */
int  gamelib_play_music(gamelib_t *g, const uint8_t *wav_data);
void gamelib_stop_music(gamelib_t *g);
bool gamelib_is_music_playing(gamelib_t *g);
```

- [ ] **Step 2: 重写 audio.c — 通道管理 + 混音 + beep + WAV + 音乐**

```c
#include "gamelib.h"
#include "wav.h"
#include <string.h>
#include <stdlib.h>

#define MAX_WAV_CHANNELS 4
#define MIX_BUFFER_SIZE  4096

typedef enum { CH_FREE, CH_PLAYING } ch_state_t;

typedef struct {
    ch_state_t  state;
    wav_info_t  wav;
    int         pos;       /* byte position in PCM data */
    int         volume;    /* 0-1000 */
    bool        loop;
} wav_channel_t;

typedef struct {
    wav_channel_t channels[MAX_WAV_CHANNELS];
    wav_channel_t music;
    int           master_volume;
    int           sample_rate;   /* cached from port init */
    int16_t      *mix_buf;
} audio_ctx_t;

static audio_ctx_t *ctx = NULL;

/* ensure context */
static audio_ctx_t* audio_get_ctx(void)
{
    if (!ctx) {
        ctx = (audio_ctx_t*)calloc(1, sizeof(audio_ctx_t));
        if (ctx) {
            ctx->master_volume = 1000;
            ctx->sample_rate = 22050;
            ctx->mix_buf = (int16_t*)calloc(MIX_BUFFER_SIZE, sizeof(int16_t));
        }
    }
    return ctx;
}

/* --- beep (unchanged) --- */
void gamelib_play_beep(gamelib_t *g, int freq, int duration_ms)
{
    (void)g;
    if (g_hal.audio.beep) g_hal.audio.beep(freq, duration_ms);
}

void gamelib_stop_beep(gamelib_t *g)
{
    (void)g;
    if (g_hal.audio.stop) g_hal.audio.stop();
}

/* --- wav --- */
int gamelib_play_wav(gamelib_t *g, const uint8_t *wav_data, int channel, int volume)
{
    (void)g;
    audio_ctx_t *c = audio_get_ctx();
    if (!c || !wav_data) return -1;

    /* auto-allocate channel */
    if (channel < 0 || channel >= MAX_WAV_CHANNELS) {
        for (int i = 0; i < MAX_WAV_CHANNELS; i++) {
            if (c->channels[i].state == CH_FREE) { channel = i; break; }
        }
        if (channel < 0) return -1;
    }

    wav_channel_t *ch = &c->channels[channel];
    if (wav_parse(wav_data, 0xFFFFFFFF, &ch->wav) != 0) return -1;

    ch->state = CH_PLAYING;
    ch->pos = 0;
    ch->volume = (volume > 0 && volume <= 1000) ? volume : 1000;
    ch->loop = false;

    /* start PCM output if not already playing */
    if (!g_hal.audio.is_pcm_playing || !g_hal.audio.is_pcm_playing()) {
        /* will be fed in update */
    }

    return channel;
}

void gamelib_stop_wav(gamelib_t *g, int channel)
{
    (void)g;
    if (!ctx || channel < 0 || channel >= MAX_WAV_CHANNELS) return;
    memset(&ctx->channels[channel], 0, sizeof(wav_channel_t));
}

void gamelib_stop_all_wav(gamelib_t *g)
{
    (void)g;
    if (!ctx) return;
    for (int i = 0; i < MAX_WAV_CHANNELS; i++) {
        memset(&ctx->channels[i], 0, sizeof(wav_channel_t));
    }
    if (g_hal.audio.stop_pcm) g_hal.audio.stop_pcm();
}

bool gamelib_is_wav_playing(gamelib_t *g, int channel)
{
    (void)g;
    if (!ctx || channel < 0 || channel >= MAX_WAV_CHANNELS) return false;
    return ctx->channels[channel].state == CH_PLAYING;
}

void gamelib_set_wav_volume(gamelib_t *g, int channel, int volume)
{
    (void)g;
    if (!ctx || channel < 0 || channel >= MAX_WAV_CHANNELS) return;
    if (volume < 0) volume = 0;
    if (volume > 1000) volume = 1000;
    ctx->channels[channel].volume = volume;
}

void gamelib_set_master_volume(gamelib_t *g, int volume)
{
    (void)g;
    audio_ctx_t *c = audio_get_ctx();
    if (!c) return;
    if (volume < 0) volume = 0;
    if (volume > 1000) volume = 1000;
    c->master_volume = volume;
    if (g_hal.audio.set_volume) g_hal.audio.set_volume(volume);
}

int gamelib_get_master_volume(gamelib_t *g)
{
    (void)g;
    audio_ctx_t *c = audio_get_ctx();
    return c ? c->master_volume : 1000;
}

/* --- music --- */
int gamelib_play_music(gamelib_t *g, const uint8_t *wav_data)
{
    (void)g;
    audio_ctx_t *c = audio_get_ctx();
    if (!c || !wav_data) return -1;

    gamelib_stop_music(g);
    if (wav_parse(wav_data, 0xFFFFFFFF, &c->music.wav) != 0) return -1;
    c->music.state = CH_PLAYING;
    c->music.pos = 0;
    c->music.volume = 1000;
    c->music.loop = true;
    return 0;
}

void gamelib_stop_music(gamelib_t *g)
{
    (void)g;
    if (!ctx) return;
    if (g_hal.audio.stop_pcm) g_hal.audio.stop_pcm();
    memset(&ctx->music, 0, sizeof(wav_channel_t));
}

bool gamelib_is_music_playing(gamelib_t *g)
{
    (void)g;
    if (!ctx) return false;
    return ctx->music.state == CH_PLAYING;
}

/* --- audio mixer (called from gamelib_update or by timer) --- */
static void audio_mix_and_output(audio_ctx_t *c)
{
    if (!c || !c->mix_buf) return;

    bool any_playing = false;
    memset(c->mix_buf, 0, MIX_BUFFER_SIZE * sizeof(int16_t));

    /* mix all WAV channels */
    for (int i = 0; i < MAX_WAV_CHANNELS; i++) {
        wav_channel_t *ch = &c->channels[i];
        if (ch->state != CH_PLAYING || !ch->wav.data) continue;

        int bytes_per_sample = ch->wav.bits_per_sample / 8;
        int total_samples = ch->wav.data_len / bytes_per_sample;
        int samples_to_mix = MIX_BUFFER_SIZE / 2; /* output is 16-bit mono */

        for (int s = 0; s < samples_to_mix; s++) {
            int src_idx = (ch->pos / bytes_per_sample) + s * ch->wav.num_channels;
            int byte_pos = src_idx * bytes_per_sample;
            if (byte_pos + bytes_per_sample > ch->wav.data_len) {
                if (ch->loop) {
                    ch->pos = 0;
                    byte_pos = 0;
                } else {
                    ch->state = CH_FREE;
                    break;
                }
            }

            int16_t sample = 0;
            if (ch->wav.bits_per_sample == 8) {
                sample = ((int16_t)ch->wav.data[byte_pos] - 128) * 256;
            } else if (ch->wav.bits_per_sample == 16) {
                sample = (int16_t)(ch->wav.data[byte_pos] | (ch->wav.data[byte_pos + 1] << 8));
            }

            int mixed = c->mix_buf[s] + (sample * ch->volume * c->master_volume) / 1000000;
            if (mixed > 32767) mixed = 32767;
            if (mixed < -32768) mixed = -32768;
            c->mix_buf[s] = (int16_t)mixed;
        }
        ch->pos += samples_to_mix * ch->wav.num_channels * bytes_per_sample;
        if (ch->state == CH_PLAYING) any_playing = true;
    }

    /* music channel (exclusive — plays alone when active) */
    if (c->music.state == CH_PLAYING && c->music.wav.data) {
        int total_samples = c->music.wav.data_len / (c->music.wav.bits_per_sample / 8);
        int samples_to_mix = MIX_BUFFER_SIZE / 2;
        memset(c->mix_buf, 0, MIX_BUFFER_SIZE * sizeof(int16_t));

        for (int s = 0; s < samples_to_mix; s++) {
            int byte_pos = (c->music.pos / (c->music.wav.bits_per_sample / 8)) *
                           (c->music.wav.bits_per_sample / 8);
            if (byte_pos >= c->music.wav.data_len) {
                if (c->music.loop) { c->music.pos = 0; byte_pos = 0; }
                else { c->music.state = CH_FREE; break; }
            }
            /* simplified: just write first byte as 8-bit sample */
            c->mix_buf[s] = ((int16_t)c->music.wav.data[byte_pos] - 128) * 256;
            c->music.pos += c->music.wav.num_channels * (c->music.wav.bits_per_sample / 8);
        }
        any_playing = true;
    }

    /* output */
    if (any_playing && g_hal.audio.play_pcm) {
        g_hal.audio.play_pcm((const uint8_t*)c->mix_buf,
                             MIX_BUFFER_SIZE * sizeof(int16_t),
                             c->sample_rate, 16, 1);
    }
}
```

---

### Task 6: 更新 main.c — WAV 音频演示

**Files:**
- Modify: `main/main.c`

在现有 MVE v3 代码基础上，新增音频演示区域。由于 WAV 需要嵌入 C 数组，演示用 beep 钢琴 + 音量控制替代（不引入外部 WAV 文件依赖）。

在初始化区域新增音频初始化，在主循环中新增钢琴按键（KEY_B 触发不同音高）和音量控制（加/减）。保持所有现有绘制和输入不变。

> 具体 main.c 代码见实现时调整，核心是在原有循环中追加:
> - 按 KEY_B 切换音高 beep 测试
> - 画出 master volume 条
> - 保留所有 Batch1/2 功能

---

### 变更总结

| 文件 | 操作 | 行数 |
|------|------|------|
| `hal_fs.h` | 新建 | +20 |
| `hal_audio.h` | 修改 | +5 |
| `wav.h` | 新建 | +20 |
| `wav.c` | 新建 | +45 |
| `port_fs.c` | 新建 | +18 |
| `port_config.h` | 修改 | +8 |
| `port_audio.c` | 重写 | +180 |
| `gamelib.h` | 修改 | +11 |
| `audio.c` | 重写 | +200 |
| `main.c` | 修改 | +30 |
