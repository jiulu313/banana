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


    /**
     *  测试
     */
    void *physical = pmm_alloc_page();

    vga_write("physical page: ");
    vga_write_hex((unsigned int)physical);
    vga_write("\n");

  
    map_page(
        0x00400000,
        (unsigned int)physical,
        0x2
    );

    volatile unsigned int *p =
        (volatile unsigned int *)0x00400000;

    *p = 0x12345678;

    vga_write("value: ");
    vga_write_hex(*p);
    vga_write("\n");


    /**
     * 下面代码可以验证访问一个没有映射的地址，会报错
     * 
     *  vga_write("Trigger page fault...\n");

        volatile unsigned int *p =
            (volatile unsigned int *)0x00400000;

        unsigned int value = *p;

        (void)value;

        vga_write("You should never see this\n");
     */


    //开启中断
    __asm__ volatile ("sti");

    //CPU停下来，等中断
    while (1) {
        __asm__ volatile ("hlt");
    }
}