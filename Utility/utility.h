#ifndef __UTILITY_H
#define __UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l0xx_hal.h"

typedef void (*periodic_func_t)(void);  // Function pointer type

typedef struct {
    uint32_t prev_time;
    uint32_t period;
    periodic_func_t func;
} periodic_task_t;

void run_periodically(periodic_task_t *task);

void delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif /* __UTILITY_H */
