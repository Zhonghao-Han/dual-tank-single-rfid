#include "utility.h"

#include "tim.h"

void run_periodically(periodic_task_t *task) {
    if (task->period == 0) {
        return;
    }
    uint32_t curr_time = HAL_GetTick();
    if (curr_time - task->prev_time > task->period) {
        task->prev_time = curr_time;
        task->func();
    }
}

void delay_us(uint32_t us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    __HAL_TIM_ENABLE(&htim2);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us * SystemCoreClock / 1000000)
        ;  // Make sure the core clock is same with timer clock
    __HAL_TIM_DISABLE(&htim2);
}