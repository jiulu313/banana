#include "paging.h"
#include "memory.h"

#define PAGE_SIZE    4096   //字节
#define PAGE_ENTRIES 1024   //PDE,PTE的数量

#define PAGE_PRESENT 0x1
#define PAGE_WRITE   0x2

static unsigned int page_directory[PAGE_ENTRIES]
    __attribute__((aligned(4096)));

static unsigned int first_page_table[PAGE_ENTRIES]
    __attribute__((aligned(4096)));


void paging_init(void)
{
    //初始化
    memset(page_directory, 0, sizeof(page_directory));
    memset(first_page_table, 0, sizeof(first_page_table));

    for (unsigned int i = 0; i < PAGE_ENTRIES; i++) {

        unsigned int physical_address =
            i * PAGE_SIZE;

        first_page_table[i] =
            physical_address |
            PAGE_PRESENT |
            PAGE_WRITE;
    }

    //把页表挂到页目录下
    page_directory[0] =
        ((unsigned int)first_page_table) |
        PAGE_PRESENT |
        PAGE_WRITE;


    //把当前页目录的物理地址，放在CR3中
    unsigned int pd =
        (unsigned int)page_directory;

    __asm__ volatile (
        "mov %0, %%cr3"
        :
        : "r"(pd)
        : "memory"
    );

    //CR0.PG=1,开启分页功能
    unsigned int cr0;

    __asm__ volatile (
        "mov %%cr0, %0"
        : "=r"(cr0)
    );

    cr0 |= 0x80000000;

    __asm__ volatile (
        "mov %0, %%cr0"
        :
        : "r"(cr0)
        : "memory"
    );
}