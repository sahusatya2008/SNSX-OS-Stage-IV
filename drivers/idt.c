#include "common.h"
#include "idt.h"
#include "string.h"
#include <stdint.h>

typedef struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t always_zero;
    uint8_t flags;
    uint16_t base_high;
} PACKED idt_entry_t;

typedef struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} PACKED idt_ptr_t;

static idt_entry_t idt_entries[256];
static idt_ptr_t idt_ptr;

extern void idt_flush(uint32_t idt_ptr_addr);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt_entries[num].base_low = (uint16_t)(base & 0xFFFF);
    idt_entries[num].base_high = (uint16_t)((base >> 16) & 0xFFFF);
    idt_entries[num].selector = selector;
    idt_entries[num].always_zero = 0;
    idt_entries[num].flags = flags;
}

void idt_install(void) {
    idt_ptr.limit = sizeof(idt_entries) - 1;
    idt_ptr.base = (uint32_t)&idt_entries;
    memset(idt_entries, 0, sizeof(idt_entries));
    idt_flush((uint32_t)&idt_ptr);
}
