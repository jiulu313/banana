#include "vga.h"
#include "idt.h"


void kernel_main(void)
{
    
    vga_clear();

    vga_write("banana kernel started\n");

    idt_init();

    vga_write("IDT initialized\n");


    //C 里嵌入汇编
    // int 0x80
    __asm__ volatile (
        "int $0x80"
    );

    vga_write("returned from interrupt\n");

    while (1) {
    }
}