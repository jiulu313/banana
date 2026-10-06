#include "vga.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "shell.h"
#include "memory.h"
#include "pmm.h"
#include "paging.h"
#include "gdt.h"


#define USER_CODE_VA    0x40000000
#define USER_STACK_VA   0x40002000
#define USER_STACK_TOP  0x40003000


extern void enter_user_mode(
    unsigned int user_eip,
    unsigned int user_esp
);


void kernel_main(void)
{
    /* -----------------------------
     * 基础初始化
     * -----------------------------
     */

    vga_clear();

    vga_write("banana kernel started\n");

    /*
     * 重新建立 kernel 自己的 GDT，
     * 其中包括：
     *
     * kernel code
     * kernel data
     * user code
     * user data
     * TSS
     */
    gdt_init();

    vga_write("GDT initialized\n");

    /*
     * 初始化 IDT
     */
    idt_init();

    vga_write("IDT initialized\n");

    /*
     * 重新映射 8259 PIC
     *
     * IRQ0 -> 32
     * IRQ1 -> 33
     */
    pic_remap();

    vga_write("PIC remapped\n");

    /*
     * PIT 每秒产生大约 100 次 IRQ0
     */
    timer_init(100);

    vga_write("PIT initialized\n");

    /*
     * 当前简单 heap
     */
    memory_init();

    /*
     * 物理页管理器
     */
    pmm_init();

    vga_write("PMM initialized\n");

    /*
     * 开启分页
     */
    paging_init();

    vga_write("Paging initialized\n");

    /*
     * shell 暂时仍然初始化。
     *
     * 注意：
     * 一旦下面 enter_user_mode() 成功执行，
     * kernel_main 不会继续往后普通执行。
     */
    shell_init();


    /* -----------------------------
     * 创建 Ring 3 地址空间
     * -----------------------------
     */

    /*
     * 为用户代码申请一个物理页
     */
    void *user_code_page =
        pmm_alloc_page();

    /*
     * 为用户栈申请一个物理页
     */
    void *user_stack_page =
        pmm_alloc_page();


    if (user_code_page == 0 ||
        user_stack_page == 0)
    {
        vga_write(
            "user page allocation failed\n"
        );

        while (1) {
            __asm__ volatile ("hlt");
        }
    }


    vga_write("user code physical: ");
    vga_write_hex(
        (unsigned int)user_code_page
    );
    vga_write("\n");


    vga_write("user stack physical: ");
    vga_write_hex(
        (unsigned int)user_stack_page
    );
    vga_write("\n");


    /*
     * 建立用户代码映射：
     *
     * VA 0x40000000
     *       ↓
     * user_code_page
     *
     * PAGE_USER 很重要。
     */
    if (!map_page(
            USER_CODE_VA,
            (unsigned int)user_code_page,
            PAGE_USER | PAGE_WRITE))
    {
        vga_write(
            "map user code failed\n"
        );

        while (1) {
            __asm__ volatile ("hlt");
        }
    }


    /*
     * 建立用户栈映射：
     *
     * VA 0x40002000
     *       ↓
     * user_stack_page
     */
    if (!map_page(
            USER_STACK_VA,
            (unsigned int)user_stack_page,
            PAGE_USER | PAGE_WRITE))
    {
        vga_write(
            "map user stack failed\n"
        );

        while (1) {
            __asm__ volatile ("hlt");
        }
    }


    /* -----------------------------
     * 准备用户态字符串
     * -----------------------------
     */

    /*
     * 字符串放在用户代码页里面，
     * 偏移 0x100。
     *
     * 地址：
     *
     * 0x40000000 + 0x100
     * =
     * 0x40000100
     */
    char *user_message =
        (char *)(USER_CODE_VA + 0x100);


    user_message[0]  = 'H';
    user_message[1]  = 'e';
    user_message[2]  = 'l';
    user_message[3]  = 'l';
    user_message[4]  = 'o';
    user_message[5]  = ' ';
    user_message[6]  = 'f';
    user_message[7]  = 'r';
    user_message[8]  = 'o';
    user_message[9]  = 'm';
    user_message[10] = ' ';
    user_message[11] = 'R';
    user_message[12] = 'i';
    user_message[13] = 'n';
    user_message[14] = 'g';
    user_message[15] = '3';
    user_message[16] = '\n';
    user_message[17] = '\0';


    /* -----------------------------
     * 准备 Ring 3 用户代码
     * -----------------------------
     */

    /*
     * 用户程序实际要执行：
     *
     * mov eax, 1
     * mov ebx, 0x40000100
     * int 0x80
     * jmp $
     *
     *
     * syscall ABI：
     *
     * EAX = syscall number
     * EBX = arg1
     */
    unsigned char *code =
        (unsigned char *)USER_CODE_VA;


    /*
     * mov eax, 1
     *
     * opcode:
     *
     * B8 xx xx xx xx
     */
    code[0] = 0xB8;

    *(unsigned int *)&code[1] = 1;


    /*
     * mov ebx, USER_CODE_VA + 0x100
     *
     * opcode:
     *
     * BB xx xx xx xx
     */
    code[5] = 0xBB;

    *(unsigned int *)&code[6] =
        USER_CODE_VA + 0x100;


    /*
     * int 0x80
     *
     * CD 80
     */
    code[10] = 0xCD;
    code[11] = 0x80;


    /*
     * jmp $
     *
     * EB FE
     *
     * 用户程序执行 syscall 后，
     * 就停在这里无限循环。
     */
    code[12] = 0xEB;
    code[13] = 0xFE;


    /* -----------------------------
     * 进入 Ring 3
     * -----------------------------
     */

    vga_write(
        "entering user mode...\n"
    );


    /*
     * enter_user_mode 最后执行 iret。
     *
     * 成功后：
     *
     * CPL = 3
     *
     * CS = 0x1B
     * SS = 0x23
     *
     * EIP = 0x40000000
     * ESP = 0x40003000
     *
     * 所以正常情况下，
     * 这个函数不会像普通 C 函数一样 return。
     */
    enter_user_mode(
        USER_CODE_VA,
        USER_STACK_TOP
    );


    /*
     * 正常情况下永远执行不到这里。
     */
    vga_write(
        "ERROR: returned from user mode\n"
    );


    while (1) {
        __asm__ volatile ("hlt");
    }
}