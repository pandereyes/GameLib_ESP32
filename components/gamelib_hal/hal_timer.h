#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include <stdint.h>

typedef struct {
    void     (*init)(void);
    uint32_t (*millis)(void);
    uint32_t (*micros)(void);
    void     (*delay_ms)(uint32_t ms);
    void     (*sleep_ms)(uint32_t ms);
} hal_timer_t;

#endif
