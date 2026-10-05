#include "tss.h"
#include "memory.h"

/**
 * 这个结构看起来很大
 * 是因为早期 x86 真的支持通过 TSS 保存完整任务状态。
 * 但我们现在真正关心的主要只是：esp0,ss0
 */
struct tss_entry
{
    unsigned int prev_tss;

    //最重要的就是这2个
    unsigned int esp0;
    unsigned int ss0;

    unsigned int esp1;
    unsigned int ss1;

    unsigned int esp2;
    unsigned int ss2;

    unsigned int cr3;
    unsigned int eip;
    unsigned int eflags;

    unsigned int eax;
    unsigned int ecx;
    unsigned int edx;
    unsigned int ebx;

    unsigned int esp;
    unsigned int ebp;
    unsigned int esi;
    unsigned int edi;

    unsigned int es;
    unsigned int cs;
    unsigned int ss;
    unsigned int ds;
    unsigned int fs;
    unsigned int gs;

    unsigned int ldt;

    unsigned short trap;
    unsigned short iomap_base;

} __attribute__((packed));

static struct tss_entry tss;




void tss_init(void)
{
    memset(&tss, 0, sizeof(tss));

    tss.ss0 = 0x10;
    tss.esp0 = 0x90000;

    tss.iomap_base = sizeof(tss);
}

void tss_set_kernel_stack(unsigned int esp0)
{
    tss.esp0 = esp0;
}

unsigned int tss_get_address(void)
{
    return (unsigned int)&tss;
}

unsigned int tss_get_size(void)
{
    return sizeof(tss);
}