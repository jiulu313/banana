#include "memory.h"

#define HEAP_START 0x200000 //起始地址
#define HEAP_SIZE  0x100000 //大小

#define HEAP_END   (HEAP_START + HEAP_SIZE) //结束地址 

static unsigned int heap_current = HEAP_START;


//给一个数，返回对应的数，假如alignment=16
//如：7,返回最近的大于7的16的倍数，得到 16，
//如：18，返回32, 33返回64
//算法细节不深挖了
static unsigned int align_up(unsigned int value, unsigned int alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}


void memory_init(void)
{
    heap_current = HEAP_START;
}

//申请size个字节的内存
void *kmalloc(unsigned int size)
{
    if (size == 0) {
        return 0;
    }

    heap_current = align_up(heap_current, 16);

    //防止内存越界
    if (heap_current + size > HEAP_END) {
        return 0;
    }

    unsigned int address = heap_current;

    heap_current += size;

    //返回申请的内存的首地址 
    return (void *)address;
}

//把一片内存全部填充某个值
void *memset(void *dest, int value, unsigned int count)
{
    unsigned char *ptr = (unsigned char *)dest;

    for (unsigned int i = 0; i < count; i++) {
        ptr[i] = (unsigned char)value;
    }

    return dest;
}

//从一块内存复制到另一块内存
void *memcpy(void *dest, const void *src, unsigned int count)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    for (unsigned int i = 0; i < count; i++) {
        d[i] = s[i];
    }

    return dest;
}