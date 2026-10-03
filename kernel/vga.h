#ifndef VGA_H
#define VGA_H

void vga_clear(void);
void vga_putc(char c);
void vga_write(const char *str);
void vga_write_hex(unsigned int value);

#endif