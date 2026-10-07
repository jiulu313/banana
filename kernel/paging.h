#ifndef PAGING_H
#define PAGING_H


#define PAGE_PRESENT 0x1    //P  PDE或者PTE是否存在
#define PAGE_WRITE   0x2    //RW 此页面允许写入
#define PAGE_USER    0x004  //US 用户态也可以访问，不设置的话，只能内核态能访问


//初始化分页机制
void paging_init(void);


//映射内存，虚拟地址到物理地址的映射
int map_page(
    unsigned int virtual_address,
    unsigned int physical_address,
    unsigned int flags
);


//主动撤销虚拟地址映射
//只是撤销地址映射，并没有释放物理页
int unmap_page(unsigned int virtual_address);


//查询一个虚拟地址当前映射到了哪个物理地址
//返回1：映射存在，则第二个参数把物理地址返回出来
//返回0：映射不存在
int get_mapping(
    unsigned int virtual_address,
    unsigned int *physical_address
);


//检查一个用户空间地址是否被映射
int is_user_address_mapped(unsigned int virtual_address);

#endif