#ifndef HAL_INPUT_H
#define HAL_INPUT_H

#include "hal_types.h"

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

#endif
