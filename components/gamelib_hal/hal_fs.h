#ifndef HAL_FS_H
#define HAL_FS_H

#include <stdint.h>

typedef struct {
    int   (*init)(void);
    void  (*deinit)(void);
    void* (*open)(const char *path, const char *mode);
    int   (*read)(void *file, uint8_t *buf, int len);
    int   (*write)(void *file, const uint8_t *buf, int len);
    int   (*seek)(void *file, int offset, int whence);
    int   (*tell)(void *file);
    int   (*size)(void *file);
    int   (*close)(void *file);
} hal_fs_t;

#endif
