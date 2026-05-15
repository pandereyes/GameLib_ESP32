#include "gamelib.h"
#include "wav.h"
#include <string.h>
#include <stdlib.h>

#define MAX_WAV_CHANNELS 4

typedef enum { CH_FREE, CH_PLAYING } ch_state_t;

typedef struct {
    ch_state_t  state;
    wav_info_t  wav;
    int         pos;
    int         volume;
    bool        loop;
} wav_channel_t;

typedef struct {
    wav_channel_t channels[MAX_WAV_CHANNELS];
    wav_channel_t music;
    int           master_volume;
    int           sample_rate;
} audio_ctx_t;

static audio_ctx_t *ctx = NULL;

static audio_ctx_t* audio_get_ctx(void)
{
    if (!ctx) {
        ctx = (audio_ctx_t*)calloc(1, sizeof(audio_ctx_t));
        if (ctx) {
            ctx->master_volume = 1000;
            ctx->sample_rate = 22050;
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

    if (g_hal.audio.play_pcm) {
        g_hal.audio.play_pcm(ch->wav.data, ch->wav.data_len, ch->wav.sample_rate, ch->wav.bits_per_sample, ch->wav.num_channels);
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
    for (int i = 0; i < MAX_WAV_CHANNELS; i++)
        memset(&ctx->channels[i], 0, sizeof(wav_channel_t));
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
    
    if (g_hal.audio.play_pcm) {
        g_hal.audio.play_pcm(c->music.wav.data, c->music.wav.data_len, c->music.wav.sample_rate, c->music.wav.bits_per_sample, c->music.wav.num_channels);
    }
    
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
