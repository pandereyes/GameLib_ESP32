#include "hal_fs.h"
#include <stddef.h>

static int  fs_init(void)           { return 0; }
static void* fs_open(const char *p)  { (void)p; return NULL; }
static int  fs_read(void *f, uint8_t *b, int l) { (void)f; (void)b; (void)l; return 0; }
static int  fs_size(void *f)         { (void)f; return 0; }
static void fs_close(void *f)        { (void)f; }

hal_fs_t g_hal_fs = {
    .init  = fs_init,
    .open  = fs_open,
    .read  = fs_read,
    .size  = fs_size,
    .close = fs_close
};
