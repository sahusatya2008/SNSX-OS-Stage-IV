#include "idt.h"
#include "isr.h"
#include "string.h"
#include "syscall.h"
#include "terminal.h"
#include "timer.h"
#include <stddef.h>
#include <stdint.h>

extern void isr128(void);

void syscall_handle(registers_t *regs) {
    switch (regs->eax) {
        case SYS_WRITE: {
            const char *text = (const char *)regs->ebx;
            uint32_t length = regs->ecx;
            if (text == NULL) {
                regs->eax = 0xFFFFFFFFu;
                break;
            }
            if (length == 0) {
                length = (uint32_t)strlen(text);
            }
            for (uint32_t i = 0; i < length; ++i) {
                terminal_putchar(text[i]);
            }
            regs->eax = length;
            break;
        }
        case SYS_CLEAR:
            terminal_clear();
            regs->eax = 0;
            break;
        case SYS_TICKS:
            regs->eax = timer_ticks();
            break;
        default:
            regs->eax = 0xFFFFFFFFu;
            break;
    }
}

void syscall_install(void) {
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE);
    register_interrupt_handler(128, syscall_handle);
}
