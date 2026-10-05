#include "idt.h"
#include "vga.h"

#define IDT_ENTRIES 256

extern void irq0(void); //timer函数
extern void irq1(void); //键盘函数

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


// 专门给CPU的 lidt 指令准备的数据结构
// 它对应的是CPU内部 IDTR 寄存器需要的格式 
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

// 声明中断异常入口 
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

extern void isr80(void);




// 中断异常信息
static const char *exception_messages[] =
{
    "Divide Error",
    "Debug",
    "NMI",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating Point",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating Point",
    "Virtualization",
    "Control Protection",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection",
    "VMM Communication",
    "Security",
    "Reserved"
};


//描述栈上的内容的
struct interrupt_frame
{
    unsigned int edi;
    unsigned int esi;
    unsigned int ebp;
    unsigned int esp;
    unsigned int ebx;
    unsigned int edx;
    unsigned int ecx;
    unsigned int eax;

    unsigned int int_no;
    unsigned int err_code;

    unsigned int eip;
    unsigned int cs;
    unsigned int eflags;
};



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

    unsigned int attr = 0xEE;

    //安装中断入口
    idt_set_gate(0,  (unsigned int)isr0,  0x08, attr);
    idt_set_gate(1,  (unsigned int)isr1,  0x08, attr);
    idt_set_gate(2,  (unsigned int)isr2,  0x08, attr);
    idt_set_gate(3,  (unsigned int)isr3,  0x08, attr);
    idt_set_gate(4,  (unsigned int)isr4,  0x08, attr);
    idt_set_gate(5,  (unsigned int)isr5,  0x08, attr);
    idt_set_gate(6,  (unsigned int)isr6,  0x08, attr);
    idt_set_gate(7,  (unsigned int)isr7,  0x08, attr);
    idt_set_gate(8,  (unsigned int)isr8,  0x08, attr);
    idt_set_gate(9,  (unsigned int)isr9,  0x08, attr);
    idt_set_gate(10, (unsigned int)isr10, 0x08, attr);
    idt_set_gate(11, (unsigned int)isr11, 0x08, attr);
    idt_set_gate(12, (unsigned int)isr12, 0x08, attr);
    idt_set_gate(13, (unsigned int)isr13, 0x08, attr);
    idt_set_gate(14, (unsigned int)isr14, 0x08, attr);
    idt_set_gate(15, (unsigned int)isr15, 0x08, attr);
    idt_set_gate(16, (unsigned int)isr16, 0x08, attr);
    idt_set_gate(17, (unsigned int)isr17, 0x08, attr);
    idt_set_gate(18, (unsigned int)isr18, 0x08, attr);
    idt_set_gate(19, (unsigned int)isr19, 0x08, attr);
    idt_set_gate(20, (unsigned int)isr20, 0x08, attr);
    idt_set_gate(21, (unsigned int)isr21, 0x08, attr);
    idt_set_gate(22, (unsigned int)isr22, 0x08, attr);
    idt_set_gate(23, (unsigned int)isr23, 0x08, attr);
    idt_set_gate(24, (unsigned int)isr24, 0x08, attr);
    idt_set_gate(25, (unsigned int)isr25, 0x08, attr);
    idt_set_gate(26, (unsigned int)isr26, 0x08, attr);
    idt_set_gate(27, (unsigned int)isr27, 0x08, attr);
    idt_set_gate(28, (unsigned int)isr28, 0x08, attr);
    idt_set_gate(29, (unsigned int)isr29, 0x08, attr);
    idt_set_gate(30, (unsigned int)isr30, 0x08, attr);
    idt_set_gate(31, (unsigned int)isr31, 0x08, attr);

    idt_set_gate(0x80, (unsigned int)isr80, 0x08, attr);

    idt_set_gate(32,(unsigned int)irq0,0x08,attr);
    idt_set_gate(33,(unsigned int)irq1,0x08,attr);

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


void exception_handler(struct interrupt_frame *frame)
{

    vga_write("\nCPU Exception: ");

    vga_write_hex(frame->int_no);

    vga_write("\n");

    if (frame->int_no < 32) {
        vga_write(exception_messages[frame->int_no]);
        vga_write("\n");
    }

    vga_write("Error code: ");
    vga_write_hex(frame->err_code);
    vga_write("\n");

    vga_write("EIP: ");
    vga_write_hex(frame->eip);
    vga_write("\n");

    while (1) {
    }
}

