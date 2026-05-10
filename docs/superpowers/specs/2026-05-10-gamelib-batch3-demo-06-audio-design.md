# GameLib ESP32-S3 第三批移植：WAV 音频系统

**日期**: 2026-05-10
**状态**: 设计完成，待实施
**前置**: Batch1/2 已验证通过
**范围**: WAV 解码/播放、多通道管理、PWM/I2S 双后端、文件系统 HAL

---

## 1. 目标

新增 WAV 音频播放系统，支持 demo 06 全部音频功能。PWM 和 I2S 双后端通过宏切换。

---

## 2. 新增 API

### 2.1 WAV 播放

```c
int  gamelib_play_wav(gamelib_t *g, const uint8_t *wav_data, int channel, int volume);
void gamelib_stop_wav(gamelib_t *g, int channel);
void gamelib_stop_all_wav(gamelib_t *g);
bool gamelib_is_wav_playing(gamelib_t *g, int channel);
```

- `wav_data`: xxd -i 嵌入的 WAV 文件 C 数组
- `channel`: 0-3，-1 表示自动分配
- `volume`: 0-1000
- 返回 channel ID，失败返回 -1

### 2.2 音量

```c
void gamelib_set_wav_volume(gamelib_t *g, int channel, int volume);
void gamelib_set_master_volume(gamelib_t *g, int volume);
int  gamelib_get_master_volume(gamelib_t *g);
```

### 2.3 背景音乐

```c
int  gamelib_play_music(gamelib_t *g, const uint8_t *wav_data);
void gamelib_stop_music(gamelib_t *g);
bool gamelib_is_music_playing(gamelib_t *g);
```

- 独立音乐通道，支持循环播放
- 同一时间仅一首，新播放自动停止旧曲

### 2.4 Demo 06 对应关系

| PC API | MCU API |
|--------|---------|
| PlayBeep(freq, dur) | gamelib_play_beep (已有) |
| PlayWAV(path, ch, vol) | gamelib_play_wav(data, ch, vol) |
| StopWAV(ch) | gamelib_stop_wav(ch) |
| StopAll() | gamelib_stop_all_wav() |
| IsPlaying(ch) | gamelib_is_wav_playing(ch) |
| SetVolume(ch, vol) | gamelib_set_wav_volume(ch, vol) |
| SetMasterVolume / GetMasterVolume | set/get_master_volume |
| PlayMusic / StopMusic / IsMusicPlaying | play/stop/is_music |

---

## 3. HAL 接口扩展

### 3.1 hal_audio.h 扩展

```c
typedef struct {
    /* 原有 beep */
    int  (*init)(void);
    void (*deinit)(void);
    void (*beep)(int freq_hz, int duration_ms);
    void (*stop)(void);
    bool (*is_busy)(void);
    /* 新增 PCM */
    int  (*play_pcm)(const uint8_t *data, int len, int sample_rate, int bits, int channels);
    void (*stop_pcm)(void);
    bool (*is_pcm_playing)(void);
    void (*set_volume)(int vol);   /* 0-1000 */
} hal_audio_t;
```

### 3.2 hal_fs.h — 文件系统 HAL

```c
typedef struct {
    int   (*init)(void);
    void* (*open)(const char *path);
    int   (*read)(void *file, uint8_t *buf, int len);
    int   (*size)(void *file);
    void  (*close)(void *file);
} hal_fs_t;
```

---

## 4. 端口实现

### 4.1 切换机制 (`port_config.h`)

```c
// #define PORT_AUDIO_USE_I2S  ← 取消注释启用 I2S DAC
```

### 4.2 PWM DAC 模式（默认）

- LEDC 定时器运行于采样率 × 分辨率（22050 × 256 ≈ 5.6MHz）
- Timer ISR 在每个采样周期更新占空比
- 8-bit 有效分辨率
- 单声道
- 使用 GPIO 21（与蜂鸣器共用）

### 4.3 I2S DAC 模式

- 标准 I2S 外设，DMA 双缓冲
- 16-bit 立体声
- I2S 引脚在 port_config.h 中定义
- `i2s_write()` 零 CPU 干预

### 4.4 文件系统 — 嵌入式数组

默认实现：`hal_fs` 操作为 no-op，WAV 通过 C 数组指针传入，文件路径参数被忽略。

---

## 5. WAV 解码器 (wav.c)

- 解析标准 PCM WAV 头（RIFF chunk）
- 支持格式: 8-bit unsigned / 16-bit signed, mono/stereo
- 输出归一化为 16-bit signed samples
- 零外部依赖，可在 PC 上单元测试

---

## 6. 通道管理 (audio.c)

```
通道池: [ch0][ch1][ch2][ch3] + [music]
         └─── 软件混音 ───┘    └─ 独立，循环 ─┘
                  ↓
            port PCM 输出
```

- 每通道状态: idle / playing / stopping
- 混音: 16-bit 加法 + 饱和限幅
- 音乐通道独立，不参与混音（音乐独占输出）
- beep 与 PCM 互斥：PCM 播放时 beep 停止，beep 时 PCM 继续

---

## 7. 文件变更

| 文件 | 操作 | 内容 |
|------|------|------|
| `gamelib.h` | 修改 | +10 API 声明 |
| `hal_audio.h` | 修改 | +PCM + 音量回调 |
| `hal_fs.h` | **新建** | 文件系统 HAL 接口 |
| `wav.c` + `wav.h` | **新建** | WAV 解码器 |
| `audio.c` | 重写 | 通道管理 + 混音 |
| `port_audio.c` | 重写 | 双后端 + 宏切换 |
| `port_fs.c` | **新建** | 嵌入式数组文件系统 |
| `port_config.h` | 修改 | +I2S 引脚 + 音频模式宏 |
| `main.c` | 修改 | 演示 WAV/音量/音乐 |

---

## 8. 不做

- MP3/OGG 解码（Flash 和 CPU 开销过大）
- SPIFFS/SD 卡文件系统（仅做嵌入式 C 数组方式）
- MIDI 播放（无合成器）
- 立体声定位/3D 音效
