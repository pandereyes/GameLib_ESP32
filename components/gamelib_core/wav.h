#ifndef WAV_H
#define WAV_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    int sample_rate;
    int bits_per_sample;
    int num_channels;
    int data_len;
    const uint8_t *data;
} wav_info_t;

/* Parse WAV header. Returns 0 on success.
   info->data points to PCM samples, info->data_len is byte count. */
int wav_parse(const uint8_t *wav_data, size_t len, wav_info_t *info);

#endif
