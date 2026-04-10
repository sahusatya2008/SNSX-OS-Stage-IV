#include "common.h"
#include "gdt.h"
#include <stdint.h>

typedef struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} PACKED gdt_entry_t;

typedef struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} PACKED gdt_ptr_t;

static gdt_entry_t gdt_entries[5];
static gdt_ptr_t gdt_ptr;

extern void gdt_flush(uint32_t gdt_ptr_addr);

static void gdt_set_gate(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity) {
    gdt_entries[index].base_low = (uint16_t)(base & 0xFFFF);
    gdt_entries[index].base_middle = (uint8_t)((base >> 16) & 0xFF);
    gdt_entries[index].base_high = (uint8_t)((base >> 24) & 0xFF);
    gdt_entries[index].limit_low = (uint16_t)(limit & 0xFFFF);
    gdt_entries[index].granularity = (uint8_t)((limit >> 16) & 0x0F);
    gdt_entries[index].granularity |= granularity & 0xF0;
    gdt_entries[index].access = access;
}

void gdt_install(void) {
    gdt_ptr.limit = sizeof(gdt_entries) - 1;
    gdt_ptr.base = (uint32_t)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    gdt_flush((uint32_t)&gdt_ptr);
}
