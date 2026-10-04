#include "vga.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "shell.h"
#include "memory.h"
#include "pmm.h"
#include "paging.h"

void kernel_main(void)
{
    
    //清屏
    vga_clear();

    //初始化中断
    idt_init();

    //重新映射
    pic_remap();

    //初始化时钟
    timer_init(100);

    //内存分配
    memory_init();

    //物理内存管理
    pmm_init();

    //开启分页机制
    paging_init();

    //shell
    shell_init();

    //开启中断
    __asm__ volatile ("sti");

    //CPU停下来，等中断
    while (1) {
        __asm__ volatile ("hlt");
    }
}