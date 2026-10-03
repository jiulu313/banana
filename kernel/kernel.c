#include "vga.h"


void kernel_main(void)
{
    
    vga_clear();

    vga_write("banana kernel started\n");
    vga_write("hello,banana os");


    while (1) {
    }
}