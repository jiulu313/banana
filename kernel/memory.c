#include "memory.h"

/**
 * 内核堆从物理地址 2MB 开始。
 * 当前 banana 内核加载在：0x10000，也就是64KB
 * 所以暂时把help放到2MB处，离内核足够远，方便学习
 * 
 */
#define HEAP_START 0x200000

static unsigned int heap_current = HEAP_START;

void memory_init(void)
{
    heap_current = HEAP_START;
}

void *kmalloc(unsigned int size)
{
    unsigned int address = heap_current;

    heap_current += size;

    return (void *)address;
}