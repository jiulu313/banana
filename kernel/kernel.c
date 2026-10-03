#include "vga.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"

void kernel_main(void)
{
    
    vga_clear();

    vga_write("banana kernel started\n");

    idt_init();

    vga_write("IDT initialized\n");

    pic_remap();

    vga_write("PIC remapped\n");

    timer_init(100);

    vga_write("PIT initialized\n");

    __asm__ volatile ("sti");

    vga_write("interrupts enabled\n");

    while (1) {
        __asm__ volatile ("hlt");
    }
}