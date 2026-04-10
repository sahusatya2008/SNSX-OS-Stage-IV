#include "paging.h"
#include "string.h"
#include <stdint.h>

static uint32_t page_directory[1024] __attribute__((aligned(4096)));

void paging_install(void) {
    memset(page_directory, 0, sizeof(page_directory));

    for (uint32_t i = 0; i < 16; ++i) {
        page_directory[i] = (i * 0x400000) | 0x83;
    }

    __asm__ volatile("mov %0, %%cr3" : : "r"(page_directory));

    uint32_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= 0x10;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));

    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}
