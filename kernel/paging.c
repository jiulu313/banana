#include "paging.h"
#include "memory.h"
#include "pmm.h"

#define PAGE_SIZE    4096   //字节
#define PAGE_ENTRIES 1024   //PDE,PTE的数量

#define PAGE_PRESENT 0x1    //P  PDE或者PTE是否存在
#define PAGE_WRITE   0x2    //RW 此页面允许写入
#define PAGE_USER    0x004  //US 用户态也可以访问，不设置的话，只能内核态能访问

// 表示保留高 20 位地址，把低 12 位 flags 清掉。
// 转成二进制： 11111111111111111111000000000000
#define PAGE_ADDR_MASK 0xFFFFF000

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

        unsigned int physical_address =  i * PAGE_SIZE;

        first_page_table[i] = physical_address | PAGE_PRESENT | PAGE_WRITE;
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

int map_page(
    unsigned int virtual_address,
    unsigned int physical_address,
    unsigned int flags)
{

    //高10位，目录表的索引
    unsigned int directory_index =
        virtual_address >> 22;

    //中间10位，页表的索引
    unsigned int table_index =
        (virtual_address >> 12) & 0x3FF;

    unsigned int *page_table;

    if (!(page_directory[directory_index] & PAGE_PRESENT)) { //如果页表不存在

        page_table = (unsigned int *)pmm_alloc_page();

        if (page_table == 0) {
            return 0;
        }

        memset(page_table, 0, PAGE_SIZE);

        page_directory[directory_index] =
            ((unsigned int)page_table & PAGE_ADDR_MASK)
            | PAGE_PRESENT
            | PAGE_WRITE;
    } else {
        page_table =(unsigned int *) (page_directory[directory_index] & PAGE_ADDR_MASK);
    }

    page_table[table_index] =
        (physical_address & PAGE_ADDR_MASK)
        | flags
        | PAGE_PRESENT;

    //这个虚拟地址对应的 TLB 项失效掉。
    __asm__ volatile (
        "invlpg (%0)"
        :
        : "r"(virtual_address)
        : "memory"
    );

    return 1;
}