#ifndef TASK_H
#define TASK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TASK_NAME_MAX 32

typedef enum task_state {
    TASK_READY = 0,
    TASK_RUNNING = 1,
    TASK_ZOMBIE = 2
} task_state_t;

typedef struct task {
    int pid;
    char name[TASK_NAME_MAX];
    task_state_t state;
    uint32_t runtime_ticks;
    bool kernel_task;
} task_t;

void task_init(void);
void task_tick(void);
int task_run_program(const char *name, const uint8_t *image, size_t size);
const task_t *task_table(size_t *count);

#endif
