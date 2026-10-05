#include "vga.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "shell.h"
#include "memory.h"
#include "pmm.h"
#include "paging.h"
#include "gdt.h"

extern void enter_user_mode(
    unsigned int user_eip,
    unsigned int user_esp
);

void kernel_main(void)
{
    
    //清屏
    vga_clear();

    //重新加载GDT
    gdt_init();

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
     * 测试 ring3  -> ring0
     */
    #define USER_CODE_VA  0x40000000
    #define USER_STACK_VA 0x40002000

    #define USER_STACK_TOP 0x40003000


    void *user_code_page = pmm_alloc_page();
    void *user_stack_page = pmm_alloc_page();

    if (user_code_page == 0 || user_stack_page == 0) {
        vga_write("user page allocation failed\n");

        while (1) {
            __asm__ volatile ("hlt");
        }
    }


    map_page(
        USER_CODE_VA,
        (unsigned int)user_code_page,
        PAGE_USER | PAGE_WRITE
    );

    map_page(
        USER_STACK_VA,
        (unsigned int)user_stack_page,
        PAGE_USER | PAGE_WRITE
    );


    unsigned char *code = (unsigned char *)user_code_page;
    /**
     * 这四个字节对应：
     * 
     * int 0x80
     * jmp $
     * 
     */
    code[0] = 0xCD;
    code[1] = 0x80;

    code[2] = 0xEB;
    code[3] = 0xFE;

    vga_write("entering user mode...\n");

    enter_user_mode(
        USER_CODE_VA,
        USER_STACK_TOP
    );



    /**
     * 测试高地址低地址映射同一块内存
     */
    // void *physical = pmm_alloc_page();

    // map_page(
    //     0xC0000000,
    //     (unsigned int)physical,
    //     PAGE_WRITE
    // );

    // volatile unsigned int *high =
    //     (volatile unsigned int *)0xC0000000;

    // volatile unsigned int *low =
    //     (volatile unsigned int *)physical;

    // *high = 0xCAFEBABE;

    // vga_write("high: ");
    // vga_write_hex(*high);
    // vga_write("\n");

    // vga_write("low : ");
    // vga_write_hex(*low);
    // vga_write("\n");



    /**
     * 测试unmap_page后，再次访问此地址对应的内存
     * 看下是否报Page Fault
     * 结果：会报，符合预期
     */
    // void *physical = pmm_alloc_page();

    // map_page(
    //     0x02000000,
    //     (unsigned int)physical,
    //     0x02
    // );

    // volatile unsigned int *p =
    //     (volatile unsigned int *)0x02000000;

    // *p = 0x12345678;

    // vga_write("before unmap: ");
    // vga_write_hex(*p);
    // vga_write("\n");

    // unmap_page(0x02000000);

    // vga_write("unmapped\n");

    // // 故意再次访问
    // unsigned int value = *p;

    // vga_write_hex(value);

    /**
     *  测试虚拟地址
     */
    // void *physical = pmm_alloc_page();

    // vga_write("physical page: ");
    // vga_write_hex((unsigned int)physical);
    // vga_write("\n");

  
    // map_page(
    //     0x00400000,
    //     (unsigned int)physical,
    //     0x2
    // );

    // volatile unsigned int *p =
    //     (volatile unsigned int *)0x00400000;

    // *p = 0x12345678;

    // vga_write("value: ");
    // vga_write_hex(*p);
    // vga_write("\n");


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