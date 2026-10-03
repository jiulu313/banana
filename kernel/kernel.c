#include "vga.h"


void kernel_main(void)
{
    
    vga_clear();

    vga_write("banana kernel started\n");
    vga_write("protected mode: OK\n");

    vga_write("kernel address: ");
    vga_write_hex(0x10000);
    vga_write("\n");

    vga_write("VGA memory: ");
    vga_write_hex(0xB8000);
    vga_write("\n");

    while (1) {
    }
}