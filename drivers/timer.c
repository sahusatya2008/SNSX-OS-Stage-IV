#include "common.h"
#include "isr.h"
#include "port_io.h"
#include "task.h"
#include "timer.h"
#include <stdint.h>

static volatile uint32_t g_ticks;

static void timer_callback(registers_t *regs) {
    UNUSED(regs);
    ++g_ticks;
    task_tick();
}

void timer_install(uint32_t frequency_hz) {
    uint32_t divisor = 1193180u / frequency_hz;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
    register_interrupt_handler(32, timer_callback);
}

uint32_t timer_ticks(void) {
    return g_ticks;
}
