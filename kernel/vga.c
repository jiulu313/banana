#include "vga.h"

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_COLOR  0x0F

static volatile char *vga = (volatile char *)VGA_MEMORY;

static int cursor_row = 0;
static int cursor_col = 0;


static void vga_scroll(void)
{
    if (cursor_row < VGA_HEIGHT) {
        return;
    }

    for (int row = 1; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {

            int from = (row * VGA_WIDTH + col) * 2;
            int to   = ((row - 1) * VGA_WIDTH + col) * 2;

            vga[to]     = vga[from];
            vga[to + 1] = vga[from + 1];
        }
    }

    int last_row = VGA_HEIGHT - 1;

    for (int col = 0; col < VGA_WIDTH; col++) {

        int index = (last_row * VGA_WIDTH + col) * 2;

        vga[index]     = ' ';
        vga[index + 1] = VGA_COLOR;
    }

    cursor_row = VGA_HEIGHT - 1;
}


void vga_clear(void)
{
    for (int row = 0; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {

            int index = (row * VGA_WIDTH + col) * 2;

            vga[index]     = ' ';
            vga[index + 1] = VGA_COLOR;
        }
    }

    cursor_row = 0;
    cursor_col = 0;
}


void vga_putc(char c)
{
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;

        vga_scroll();
        return;
    }

    int index = (cursor_row * VGA_WIDTH + cursor_col) * 2;

    vga[index]     = c;
    vga[index + 1] = VGA_COLOR;

    cursor_col++;

    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;

        vga_scroll();
    }
}


void vga_write(const char *str)
{
    while (*str != '\0') {
        vga_putc(*str);
        str++;
    }
}


void vga_write_hex(unsigned int value)
{
    const char *hex = "0123456789ABCDEF";

    vga_write("0x");

    for (int shift = 28; shift >= 0; shift -= 4) {

        unsigned int digit = (value >> shift) & 0xF;

        vga_putc(hex[digit]);
    }
}