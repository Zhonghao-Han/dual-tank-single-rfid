#include "utility.h"

void run_periodically(periodic_task_t *task){
    if(task->period == 0){
        return;
    }
    uint32_t curr_time = HAL_GetTick();
    if (curr_time - task->prev_time > task->period) {
        task->prev_time = curr_time;
        task->func();  
    }
}