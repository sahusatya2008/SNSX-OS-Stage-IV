#include "task.h"
#include "string.h"
#include <stddef.h>
#include <stdint.h>

#define MAX_TASKS 16
#define PROGRAM_LOAD_ADDR ((uint8_t *)0x00400000)
#define MAX_PROGRAM_SIZE (64 * 1024)

static task_t g_tasks[MAX_TASKS];
static size_t g_task_count;
static size_t g_current_task;

static void task_invoke_program(void *entry_point) {
    __asm__ volatile(
        "call *%0"
        :
        : "m"(entry_point)
        : "eax", "ebx", "ecx", "edx", "esi", "edi", "memory", "cc");
}

static void add_task(const char *name, bool kernel_task, task_state_t state) {
    if (g_task_count >= MAX_TASKS) {
        return;
    }
    task_t *task = &g_tasks[g_task_count];
    task->pid = (int)g_task_count + 1;
    strncpy(task->name, name, TASK_NAME_MAX - 1);
    task->name[TASK_NAME_MAX - 1] = '\0';
    task->state = state;
    task->runtime_ticks = 0;
    task->kernel_task = kernel_task;
    ++g_task_count;
}

void task_init(void) {
    g_task_count = 0;
    g_current_task = 0;
    add_task("shell", true, TASK_RUNNING);
}

void task_tick(void) {
    if (g_current_task < g_task_count) {
        ++g_tasks[g_current_task].runtime_ticks;
    }
}

int task_run_program(const char *name, const uint8_t *image, size_t size) {
    if (g_task_count >= MAX_TASKS || size == 0 || size > MAX_PROGRAM_SIZE) {
        return -1;
    }

    size_t parent = g_current_task;
    size_t slot = g_task_count;
    add_task(name, false, TASK_READY);

    memset(PROGRAM_LOAD_ADDR, 0, MAX_PROGRAM_SIZE);
    memcpy(PROGRAM_LOAD_ADDR, image, size);

    g_tasks[parent].state = TASK_READY;
    g_tasks[slot].state = TASK_RUNNING;
    g_current_task = slot;

    task_invoke_program(PROGRAM_LOAD_ADDR);

    g_tasks[slot].state = TASK_ZOMBIE;
    g_current_task = parent;
    g_tasks[parent].state = TASK_RUNNING;
    return g_tasks[slot].pid;
}

const task_t *task_table(size_t *count) {
    if (count != NULL) {
        *count = g_task_count;
    }
    return g_tasks;
}
