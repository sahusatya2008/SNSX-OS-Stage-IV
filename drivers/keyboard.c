#include "common.h"
#include "isr.h"
#include "keyboard.h"
#include "port_io.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KEYBOARD_BUFFER_SIZE 128

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static size_t buffer_head;
static size_t buffer_tail;
static bool shift_active;

static const char keymap[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,   '\\','z',
    'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   '-', 0,   0,   0,   '+', 0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0
};

static const char keymap_shift[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t','Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|', 'Z',
    'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   '-', 0,   0,   0,   '+', 0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0
};

static void buffer_push(char c) {
    size_t next = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next == buffer_tail) {
        return;
    }
    keyboard_buffer[buffer_head] = c;
    buffer_head = next;
}

static void keyboard_callback(registers_t *regs) {
    UNUSED(regs);

    uint8_t scancode = inb(0x60);
    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        if (released == 42 || released == 54) {
            shift_active = false;
        }
        return;
    }

    if (scancode == 42 || scancode == 54) {
        shift_active = true;
        return;
    }

    char translated = shift_active ? keymap_shift[scancode] : keymap[scancode];
    if (translated != 0) {
        buffer_push(translated);
    }
}

void keyboard_install(void) {
    register_interrupt_handler(33, keyboard_callback);
}

bool keyboard_has_char(void) {
    return buffer_head != buffer_tail;
}

char keyboard_getchar(void) {
    if (buffer_head == buffer_tail) {
        return 0;
    }
    char c = keyboard_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}
