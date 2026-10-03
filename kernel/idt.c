#include "idt.h"
#include "vga.h"

#define IDT_ENTRIES 256

//  8个字节 ，64位 [ 0 - 63 ]，结构如下：
//
//  63                         48 47              40 39      32 31              16 15               0
//  +----------------------------+------------------+----------+------------------+------------------+
//  |       offset_high          |    type_attr     |   zero   |     selector     |    offset_low    |
//  +----------------------------+------------------+----------+------------------+------------------+
//          16 bits                   8 bits          8 bits        16 bits            16 bits
//
struct idt_entry
{
    unsigned short offset_low;      //0-15  ，中断处理函数地址的低 16 位
    unsigned short selector;        //16-31 ，中断处理函数使用哪个代码段。
    unsigned char zero;             //32-39 ，硬性要求，必须为0,不是我们能自由使用的。
    unsigned char type_attr;        //40-47 ，重要的属性字段之一，描述中断描述符的属性
    unsigned short offset_high;     //48-63 ，处理函数地址的高 16 位。
} __attribute__((packed));


//专门给CPU的 lidt 指令准备的数据结构
//它对应的是CPU内部 IDTR 寄存器需要的格式 
// 共6个字节 
struct idt_ptr
{
    unsigned short limit;   //2个字节 ，16位，IDT 表的最大有效偏移。
    unsigned int base;      //4个字节 ，32位，IDT 表在内存里的起始地址。
} __attribute__((packed));


static struct idt_entry idt[IDT_ENTRIES];   //中断数组

static struct idt_ptr idtp; 

//interrupt.asm 汇编中定义的函数
extern void idt_load(struct idt_ptr *ptr);
extern void isr80(void);
extern void isr0(void);


static void idt_set_gate(
    int index,
    unsigned int handler,
    unsigned short selector,
    unsigned char type_attr)
{
    idt[index].offset_low = handler & 0xFFFF;

    idt[index].selector = selector;

    idt[index].zero = 0;

    idt[index].type_attr = type_attr;

    idt[index].offset_high = (handler >> 16) & 0xFFFF;
}


void idt_init(void)
{
    //初始化，把idt表中的每一项全部置为0
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].type_attr = 0;
        idt[i].offset_high = 0;
    }

    //给 IDT 的第 0x80 项，安装一个中断门，它的处理函数是 isr80
    //调用时，INT 0x80,就会走到isr80
    idt_set_gate(
        0x80,
        (unsigned int)isr80,
        0x08,
        0x8E
    );

    //除数是0的中断，安装处理函数
    idt_set_gate(
        0x00,
        (unsigned int)isr0,
        0x08,
        0x8E
    );

    //给CPU中的IDTR寄存器准备数据 
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (unsigned int)&idt;

    idt_load(&idtp);
}


//0x80中断处理函数
void interrupt_handler(void)
{
    vga_write("\ninterrupt 0x80 received\n");
}

//除数为0的中断处理函数
void divide_error_handler(void) 
{
    vga_write("\nDivide Error exception!\n");
    while(1) {

    }
}