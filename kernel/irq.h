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

/* 触发 0x81 软件中断，进入 isr81，由 scheduler_on_yield() 选择任务。
 * CPU 和 isr81 保存当前现场；isr81 换栈后通过 popa、iret 恢复选中任务。
 * 当前任务再次被选中时，才从 int 后面继续；已退出的任务不会再返回。
 * 若调度器再次选中当前任务，也可能立即继续执行。
 *
 * 这是调度器内部的底层操作，不检查调用条件，也不设置休眠/退出状态。
 * 普通任务应调用 scheduler_yield()/scheduler_sleep()/scheduler_wait() 等接口。
 * IF 为 0 时仍可执行 int；恢复当前任务时 IF 恢复为执行 int 前的值，
 * 因此调用者若先执行 irq_save()，返回后仍需 irq_restore()。
 * 软件中断不推进时钟计数，也不需要向 PIC 发送 EOI。
 */
static inline void irq_schedule(void)
{
    // $0x81 是 AT&T 语法中的立即数中断号；无 C 输入、输出操作数。
    // "memory" 让编译器考虑其他任务在切换期间可能修改内存。
    __asm__ volatile ("int $0x81" : : : "memory");
}

#endif
