#include "wav.h"
#include <string.h>

int wav_parse(const uint8_t *wav_data, size_t len, wav_info_t *info)
{
    if (!wav_data || len < 44 || !info) return -1;
    memset(info, 0, sizeof(*info));

    /* RIFF header */
    if (memcmp(wav_data, "RIFF", 4) != 0) return -1;
    if (memcmp(wav_data + 8, "WAVE", 4) != 0) return -1;

    /* Find fmt and data chunks */
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
