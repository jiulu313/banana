#include "gdt.h"
#include "tss.h"
#include "memory.h"

struct gdt_entry
{
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_middle;
    unsigned char access;
    unsigned char granularity;
    unsigned char base_high;
}__attribute__((packed));



struct gdt_ptr
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));



static struct gdt_entry gdt[6];
static struct gdt_ptr gdtp;


extern void gdt_flush(unsigned int gdt_ptr_address);
extern void tss_flush(void);



static void gdt_set_gate(
    int index,
    unsigned int base,
    unsigned int limit,
    unsigned char access,
    unsigned char granularity)
{
    gdt[index].base_low =
        base & 0xFFFF;

    gdt[index].base_middle =
        (base >> 16) & 0xFF;

    gdt[index].base_high =
        (base >> 24) & 0xFF;

    gdt[index].limit_low =
        limit & 0xFFFF;

    gdt[index].granularity =
        (limit >> 16) & 0x0F;

    gdt[index].granularity |=
        granularity & 0xF0;

    gdt[index].access = access;
}



void gdt_init(void)
{
    gdtp.limit = sizeof(gdt) - 1;

    gdtp.base = (unsigned int)&gdt;

    // null
    gdt_set_gate(
        0,
        0,
        0,
        0,
        0
    );

    // kernel code
    gdt_set_gate(
        1,
        0,
        0xFFFFF,
        0x9A,
        0xCF
    );

    // kernel data
    gdt_set_gate(
        2,
        0,
        0xFFFFF,
        0x92,
        0xCF
    );

    // user code
    gdt_set_gate(
        3,
        0,
        0xFFFFF,
        0xFA,
        0xCF
    );

    // user data
    gdt_set_gate(
        4,
        0,
        0xFFFFF,
        0xF2,
        0xCF
    );

    tss_init();

    unsigned int tss_base =
        tss_get_address();

    unsigned int tss_limit =
        tss_get_size() - 1;

    // TSS
    gdt_set_gate(
        5,
        tss_base,
        tss_limit,
        0x89,
        0x00
    );

    gdt_flush((unsigned int)&gdtp);

    tss_flush();
}