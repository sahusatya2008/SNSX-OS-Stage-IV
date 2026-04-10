#include "port_io.h"
#include "string.h"
#include "terminal.h"
#include <stddef.h>
#include <stdint.h>

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t *const VGA_MEMORY = (uint16_t *)0xB8000;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;

static uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | (uint8_t)(bg << 4);
}

static uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void terminal_update_cursor(void) {
    uint16_t position = (uint16_t)(terminal_row * VGA_WIDTH + terminal_column);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((position >> 8) & 0xFF));
}

static void terminal_scroll(void) {
    if (terminal_row < VGA_HEIGHT) {
        return;
    }
    for (size_t y = 1; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            VGA_MEMORY[(y - 1) * VGA_WIDTH + x] = VGA_MEMORY[y * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; ++x) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    }
    terminal_row = VGA_HEIGHT - 1;
}

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_clear();
}

void terminal_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
        }
    }
    terminal_row = 0;
    terminal_column = 0;
    terminal_update_cursor();
}

void terminal_set_color(uint8_t fg, uint8_t bg) {
    terminal_color = vga_entry_color(fg, bg);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        ++terminal_row;
        terminal_scroll();
        terminal_update_cursor();
        return;
    }
    if (c == '\r') {
        terminal_column = 0;
        terminal_update_cursor();
        return;
    }
    if (c == '\t') {
        for (int i = 0; i < 4; ++i) {
            terminal_putchar(' ');
        }
        return;
    }

    VGA_MEMORY[terminal_row * VGA_WIDTH + terminal_column] = vga_entry((unsigned char)c, terminal_color);
    ++terminal_column;
    if (terminal_column >= VGA_WIDTH) {
        terminal_column = 0;
        ++terminal_row;
    }
    terminal_scroll();
    terminal_update_cursor();
}

void terminal_backspace(void) {
    if (terminal_column == 0 && terminal_row == 0) {
        return;
    }
    if (terminal_column == 0) {
        --terminal_row;
        terminal_column = VGA_WIDTH - 1;
    } else {
        --terminal_column;
    }
    VGA_MEMORY[terminal_row * VGA_WIDTH + terminal_column] = vga_entry(' ', terminal_color);
    terminal_update_cursor();
}

void terminal_write(const char *data) {
    for (size_t i = 0; data[i] != '\0'; ++i) {
        terminal_putchar(data[i]);
    }
}

void terminal_writeline(const char *data) {
    terminal_write(data);
    terminal_putchar('\n');
}

void terminal_write_hex(uint32_t value) {
    char buffer[11];
    u32_to_hex(value, buffer);
    terminal_write(buffer);
}

void terminal_write_dec(uint32_t value) {
    char buffer[16];
    u32_to_dec(value, buffer);
    terminal_write(buffer);
}
