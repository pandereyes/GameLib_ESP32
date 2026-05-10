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
