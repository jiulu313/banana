#include "vga.h"

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH  80           //col number
#define VGA_HEIGHT 25           //row number

static volatile char *vga = (volatile char *)VGA_MEMORY;

static int cursor_row = 0;
static int cursor_col = 0;


void vga_clear(void)
{
    for (int row = 0; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {

            int index = (row * VGA_WIDTH + col) * 2;

            vga[index] = ' ';
            vga[index + 1] = 0x0F;
        }
    }

    cursor_row = 0;
    cursor_col = 0;
}


void vga_putc(char c)
{
    if (c == '\n') {
        cursor_row++;
        cursor_col = 0;
        return;
    }

    int index = (cursor_row * VGA_WIDTH + cursor_col) * 2;

    vga[index] = c;
    vga[index + 1] = 0x0F;

    cursor_col++;

    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;
    }
}


void vga_write(const char *str)
{
    while (*str != '\0') {
        vga_putc(*str);
        str++;
    }
}