#ifndef SYSCALL_H
#define SYSCALL_H

#include "isr.h"

enum {
    SYS_WRITE = 1,
    SYS_CLEAR = 2,
    SYS_TICKS = 3
};

void syscall_install(void);
void syscall_handle(registers_t *regs);

#endif
