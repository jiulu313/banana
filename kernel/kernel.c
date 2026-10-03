#include "vga.h"
#include "idt.h"


void kernel_main(void)
{
    
    vga_clear();

    vga_write("banana kernel started\n");

    idt_init();

    vga_write("IDT initialized\n");
    vga_write("Trigger invalid opcode...\n");

    __asm__ volatile (
        "ud2"
    );

    vga_write("You should never see this\n");

    while (1) {
    }
}