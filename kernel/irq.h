#ifndef IRQ_H
#define IRQ_H

// 单核临界区：保存原状态，退出时恢复，不能无条件 sti。
/* 读取当前 EFLAGS，不改变中断开关。
 * 返回值中第 9 位是 IF：flags & 0x200 非零表示允许可屏蔽硬件中断。
 */
static inline unsigned int irq_flags(void)
{
    unsigned int flags;
    // pushfl：EFLAGS 压栈；popl %0：弹入编译器为 flags 分配的寄存器。
    // "=r" 表示寄存器输出；没有输入操作数；压栈、出栈后 ESP 不变。
    __asm__ volatile ("pushfl; popl %0" : "=r"(flags) : : "memory");
    return flags;
}

/* 保存 EFLAGS，然后关闭可屏蔽硬件中断，供单核临界区使用。
 * 返回的是 cli 之前的标志，必须交给 irq_restore() 恢复。
 * 关闭硬件中断不会阻止代码执行 int 软件中断。
 */
static inline unsigned int irq_save(void)
{
    unsigned int flags;
    // 先把原 EFLAGS 保存到 flags，再用 cli 清除 IF 位。
    __asm__ volatile ("pushfl; popl %0; cli" : "=r"(flags) : : "memory");
    return flags;
}

/* 恢复之前保存的 EFLAGS，包括 IF 中断允许位。
 * 原来开中断就恢复为开启，原来关中断就保持关闭，不能用 sti 替代。
 * 典型用法：flags = irq_save(); 修改共享数据; irq_restore(flags);
 */
static inline void irq_restore(unsigned int flags)
{
    // pushl %0：将 flags 压栈；popfl：弹出并写回 EFLAGS。
    // "r"(flags) 是寄存器输入，前面的空冒号表示没有 C 输出变量。
    // volatile 保留汇编的副作用；"memory" 限制编译器跨越此处重排内存访问，
    // 不是 CPU 内存屏障；"cc" 告知编译器条件标志被修改。
    __asm__ volatile ("pushl %0; popfl" : : "r"(flags) : "memory", "cc");
}

// 保存/恢复任务现场由 isr81 完成。
static inline void irq_schedule(void)
{
    __asm__ volatile ("int $0x81" : : : "memory");
}

#endif
