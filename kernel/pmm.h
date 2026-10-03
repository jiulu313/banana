//物理内存管理


#ifndef PMM_H
#define PMM_H

void pmm_init(void);

void *pmm_alloc_page(void);             //申请内存
void pmm_free_page(void *addr);         //释放内存

unsigned int pmm_get_total_pages(void); //获取内存总页数
unsigned int pmm_get_free_pages(void);  //获取释放的内存页数

#endif