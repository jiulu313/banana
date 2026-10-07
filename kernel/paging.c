#include "paging.h"
#include "memory.h"
#include "pmm.h"

#define PAGE_SIZE    4096   //字节
#define PAGE_ENTRIES 1024   //PDE,PTE的数量



// 表示保留高 20 位地址，把低 12 位 flags 清掉。
// 转成二进制： 11111111111111111111000000000000
#define PAGE_ADDR_MASK 0xFFFFF000


#define IDENTITY_MB     16  //16MB
#define IDENTITY_TABLES (IDENTITY_MB / 4)

static unsigned int page_directory[PAGE_ENTRIES]
    __attribute__((aligned(4096)));

static unsigned int identity_page_tables[IDENTITY_TABLES][PAGE_ENTRIES]
    __attribute__((aligned(4096)));


void paging_init(void)
{
    //初始化
    memset(page_directory, 0, sizeof(page_directory));
    memset(identity_page_tables, 0, sizeof(identity_page_tables));

   //填充4张页表
    for (unsigned int table = 0; table < IDENTITY_TABLES; table++)
    {
        
        for (unsigned int entry = 0; entry < PAGE_ENTRIES; entry++)
        {
            unsigned int page_number = table * PAGE_ENTRIES + entry;

            unsigned int physical_address = page_number * PAGE_SIZE;

            identity_page_tables[table][entry] = 
                physical_address | 
                PAGE_PRESENT | 
                PAGE_WRITE;
        }
    }
    

    //把 4 张 Page Table 挂进 Page Directory
    for (unsigned int i = 0; i < IDENTITY_TABLES; i++)
    {
        //第 i 张页表首地址
        unsigned int addr = (unsigned int) &identity_page_tables[i][0];
        addr = addr & PAGE_ADDR_MASK;
        page_directory[i] = addr | PAGE_PRESENT | PAGE_WRITE;
    }
    



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

    //0xFFF对应二进制： 1111 1111 1111
    //检查是否4KB对齐，因为只映射页，不能映射任意字节
    //最低 12 位不为 0，就说明地址不是 4KB 对齐。
    //所以要检查是否4KB对齐
    if ((virtual_address & 0xFFF) != 0) {
        return 0;
    }

    if ((physical_address & 0xFFF) != 0) {
        return 0;
    }


    //高10位，目录表的索引
    unsigned int directory_index =
        virtual_address >> 22;

    //中间10位，页表的索引
    unsigned int table_index =
        (virtual_address >> 12) & 0x3FF;

    unsigned int *page_table;

    if (!(page_directory[directory_index] & PAGE_PRESENT)) { //如果页表不存在

    
         /*
         * 向 PMM 申请一页，用来当 Page Table。
         */
        page_table = (unsigned int *)pmm_alloc_page();

        if (page_table == 0) {
            return 0;
        }

        memset(page_table, 0, PAGE_SIZE);



        /*
         * PDE 的 flags。
         */
        unsigned int directory_flags =
            PAGE_PRESENT |
            PAGE_WRITE;
        
        /**
         * 如果要映射的是用户页， 
         * PDE 也必须设置 PAGE_USER。
         */
        if (flags & PAGE_USER) {
            directory_flags |= PAGE_USER;
        }    


        page_directory[directory_index] = 
            ((unsigned int)page_table & PAGE_ADDR_MASK)
            | directory_flags;


    } else {

        if (flags & PAGE_USER)
        {
            /**
             * 非常关键：
             * 
             * 如果PTE是 user
             * PDE也必须是 user
             * 
             */
            page_directory[directory_index] |= PAGE_USER;
        }

        

        page_table = 
        (unsigned int *) 
        (page_directory[directory_index] & PAGE_ADDR_MASK);
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


//主动撤销虚拟地址映射
//只是撤销地址映射，并没有释放物理页
int unmap_page(unsigned int virtual_address)
{
    unsigned int directory_index =
        virtual_address >> 22;

    unsigned int table_index =
        (virtual_address >> 12) & 0x3FF;

    // 对应的 Page Table 都不存在
    if (!(page_directory[directory_index] & PAGE_PRESENT)) {
        return 0;
    }

    unsigned int *page_table =
        (unsigned int *)
        (page_directory[directory_index] & PAGE_ADDR_MASK);

    // 这个虚拟页本身没有映射
    if (!(page_table[table_index] & PAGE_PRESENT)) {
        return 0;
    }

    // 清掉整个 PTE
    page_table[table_index] = 0;

    // 清掉这个虚拟地址对应的 TLB 缓存
    __asm__ volatile (
        "invlpg (%0)"
        :
        : "r"(virtual_address)
        : "memory"
    );

    return 1;
}


//查询一个虚拟地址当前映射到了哪个物理地址
//返回1：映射存在，则第二个参数把物理地址返回出来
//返回0：映射不存在
int get_mapping(
    unsigned int virtual_address,
    unsigned int *physical_address)
{
    unsigned int directory_index =
        virtual_address >> 22;

    unsigned int table_index =
        (virtual_address >> 12) & 0x3FF;

    unsigned int offset =
        virtual_address & 0xFFF;

    // PDE 不存在
    if (!(page_directory[directory_index] & PAGE_PRESENT)) {
        return 0;
    }

    unsigned int *page_table =
        (unsigned int *)
        (page_directory[directory_index] & PAGE_ADDR_MASK);

    // PTE 不存在
    if (!(page_table[table_index] & PAGE_PRESENT)) {
        return 0;
    }

    unsigned int physical_page =
        page_table[table_index] & PAGE_ADDR_MASK;

    *physical_address =
        physical_page + offset;

    return 1;
}

//检查一个用户空间地址是否被映射
int is_user_address_mapped(unsigned int virtual_address)
{
    unsigned int directory_index =
        virtual_address >> 22;

    unsigned int table_index =
        (virtual_address >> 12) & 0x3FF;

    unsigned int pde =
        page_directory[directory_index];

    //PDE不存在    
    if (!(pde & PAGE_PRESENT)) {
        return 0;
    }

    //PDE.USER=1 ?
    if (!(pde & PAGE_USER)) {
        return 0;
    }


    unsigned int *page_table =
        (unsigned int *)(pde & PAGE_ADDR_MASK);

    unsigned int pte =
        page_table[table_index];

    //PTE不存在
    if (!(pte & PAGE_PRESENT)) {
        return 0;
    }

    //PTE.USER=1?
    if (!(pte & PAGE_USER)) {
        return 0;
    }

    return 1;
}

//检查用户页是否可写
int is_user_address_writable(
    unsigned int virtual_address)
{
    unsigned int directory_index =
        virtual_address >> 22;

    unsigned int table_index =
        (virtual_address >> 12) & 0x3FF;

    unsigned int pde =
        page_directory[directory_index];

    if (!(pde & PAGE_PRESENT)) {
        return 0;
    }

    if (!(pde & PAGE_USER)) {
        return 0;
    }

    if (!(pde & PAGE_WRITE)) {
        return 0;
    }

    unsigned int *page_table =
        (unsigned int *)
        (pde & PAGE_ADDR_MASK);

    unsigned int pte =
        page_table[table_index];

    if (!(pte & PAGE_PRESENT)) {
        return 0;
    }

    if (!(pte & PAGE_USER)) {
        return 0;
    }

    if (!(pte & PAGE_WRITE)) {
        return 0;
    }

    return 1;
}