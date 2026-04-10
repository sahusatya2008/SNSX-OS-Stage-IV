#include "fs.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "keyboard.h"
#include "multiboot.h"
#include "paging.h"
#include "port_io.h"
#include "shell.h"
#include "syscall.h"
#include "task.h"
#include "terminal.h"
#include "timer.h"
#include <stdbool.h>
#include <stdint.h>

void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info_addr) {
    terminal_initialize();
    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_writeline("SNSX kernel booting...");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    gdt_install();
    idt_install();
    isr_install();
    irq_install();
    syscall_install();
    paging_install();
    task_init();
    timer_install(100);
    keyboard_install();

    bool initrd_loaded = fs_init(multiboot_magic, multiboot_info_addr);

    terminal_write("Storage layer: ");
    terminal_writeline(initrd_loaded ? "initrd image copied into writable RAM VFS" : "built-in fallback VFS");
    terminal_write("Mounted entries: ");
    terminal_write_dec((uint32_t)fs_count());
    terminal_writeline("");

    enable_interrupts();
    shell_run();

    while (1) {
        halt();
    }
}
