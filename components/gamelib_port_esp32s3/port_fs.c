#include "hal_fs.h"
#include "port_config.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define SD_MOUNT_POINT "/sdcard"
#define SD_MAX_FILES    4

static sdmmc_card_t *g_card = NULL;
static bool           g_mounted = false;

typedef struct {
    int fd;
} port_fs_file_t;

/* ====== init ====== */
int port_fs_init(void)
{
    esp_vfs_fat_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = SD_MAX_FILES,
        .allocation_unit_size = 4096
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = PORT_SD_SPI_HOST;

    sdspi_device_config_t dev_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    dev_cfg.gpio_cs = (gpio_num_t)PORT_SD_PIN_CS;
    dev_cfg.host_id = host.slot;

    esp_err_t ret = esp_vfs_fat_sdspi_mount(
        SD_MOUNT_POINT, &host, &dev_cfg, &mount_cfg, &g_card);
    if (ret != ESP_OK) {
        printf("port_fs: esp_vfs_fat_sdspi_mount failed (0x%x)\n", ret);
        return -1;
    }

    g_mounted = true;
    printf("port_fs: SD mounted at %s\n", SD_MOUNT_POINT);
    return 0;
}

/* ====== deinit ====== */
void port_fs_deinit(void)
{
    if (!g_mounted) return;

    esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, g_card);
    g_card = NULL;
    g_mounted = false;
    printf("port_fs: SD unmounted\n");
}

/* ====== mode string to POSIX flags ====== */
static int mode_to_open_flags(const char *mode)
{
    if (mode == NULL) return -1;
    if (strcmp(mode, "rb") == 0)  return O_RDONLY;
    if (strcmp(mode, "wb") == 0)  return O_WRONLY | O_CREAT | O_TRUNC;
    if (strcmp(mode, "ab") == 0)  return O_WRONLY | O_CREAT | O_APPEND;
    if (strcmp(mode, "rb+") == 0) return O_RDWR;
    if (strcmp(mode, "wb+") == 0) return O_RDWR | O_CREAT | O_TRUNC;
    if (strcmp(mode, "ab+") == 0) return O_RDWR | O_CREAT | O_APPEND;
    return -1;
}

/* ====== open ====== */
void *port_fs_open(const char *path, const char *mode)
{
    int open_flags = mode_to_open_flags(mode);
    if (!g_mounted || !path || open_flags < 0) return NULL;

    char full_path[64];
    int path_len = snprintf(full_path, sizeof(full_path), "%s/%s", SD_MOUNT_POINT,
        (path[0] == '/') ? path + 1 : path);
    if (path_len < 0 || path_len >= (int)sizeof(full_path)) {
        printf("port_fs: path too long: %s\n", path);
        return NULL;
    }

    port_fs_file_t *file = (port_fs_file_t *)malloc(sizeof(port_fs_file_t));
    if (!file) return NULL;

    file->fd = open(full_path, open_flags, 0644);
    if (file->fd < 0) {
        printf("port_fs: open(%s) failed\n", full_path);
        free(file);
        return NULL;
    }

    if (mode[0] == 'a' && lseek(file->fd, 0, SEEK_END) < 0) {
        close(file->fd);
        free(file);
        return NULL;
    }

    return (void *)file;
}

/* ====== read ====== */
int port_fs_read(void *file, uint8_t *buf, int len)
{
    if (!file || !buf || len <= 0) return -1;
    port_fs_file_t *fp = (port_fs_file_t *)file;
    ssize_t bytes_read = read(fp->fd, buf, (size_t)len);
    return (bytes_read >= 0) ? (int)bytes_read : -1;
}

/* ====== write ====== */
int port_fs_write(void *file, const uint8_t *buf, int len)
{
    if (!file || !buf || len <= 0) return -1;
    port_fs_file_t *fp = (port_fs_file_t *)file;
    ssize_t bytes_written = write(fp->fd, buf, (size_t)len);
    return (bytes_written >= 0) ? (int)bytes_written : -1;
}

/* ====== seek ====== */
int port_fs_seek(void *file, int offset, int whence)
{
    if (!file) return -1;
    port_fs_file_t *fp = (port_fs_file_t *)file;
    int posix_whence;
    switch (whence) {
    case 0: posix_whence = SEEK_SET; break;
    case 1: posix_whence = SEEK_CUR; break;
    case 2: posix_whence = SEEK_END; break;
    default: return -1;
    }

    return (lseek(fp->fd, offset, posix_whence) >= 0) ? 0 : -1;
}

/* ====== tell ====== */
int port_fs_tell(void *file)
{
    if (!file) return -1;
    port_fs_file_t *fp = (port_fs_file_t *)file;
    off_t pos = lseek(fp->fd, 0, SEEK_CUR);
    return (pos >= 0) ? (int)pos : -1;
}

/* ====== size ====== */
int port_fs_size(void *file)
{
    if (!file) return -1;
    port_fs_file_t *fp = (port_fs_file_t *)file;
    struct stat st;
    if (fstat(fp->fd, &st) == 0) {
        return (int)st.st_size;
    }

    return -1;
}

/* ====== close ====== */
int port_fs_close(void *file)
{
    if (!file) return -1;
    port_fs_file_t *fp = (port_fs_file_t *)file;
    int ret = close(fp->fd);
    free(fp);
    return (ret == 0) ? 0 : -1;
}

/* ====== HAL instance ====== */
hal_fs_t g_hal_fs = {
    .init   = port_fs_init,
    .deinit = port_fs_deinit,
    .open   = port_fs_open,
    .read   = port_fs_read,
    .write  = port_fs_write,
    .seek   = port_fs_seek,
    .tell   = port_fs_tell,
    .size   = port_fs_size,
    .close  = port_fs_close
};
