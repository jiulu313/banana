#include "vga.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "shell.h"
#include "memory.h"
#include "pmm.h"

void kernel_main(void)
{
    
    vga_clear();

    idt_init();
    pic_remap();
    timer_init(100);

    memory_init();

    pmm_init();

    shell_init();

    __asm__ volatile ("sti");

    while (1) {
        __asm__ volatile ("hlt");
    }
}