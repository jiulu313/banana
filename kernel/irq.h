#ifndef IRQ_H
#define IRQ_H

// 单核临界区：保存原状态，退出时恢复，不能无条件 sti。
static inline unsigned int irq_flags(void)
{
    unsigned int flags;
    __asm__ volatile ("pushfl; popl %0" : "=r"(flags) : : "memory");
    return flags;
}

static inline unsigned int irq_save(void)
{
    unsigned int flags;
    __asm__ volatile ("pushfl; popl %0; cli" : "=r"(flags) : : "memory");
    return flags;
}

static inline void irq_restore(unsigned int flags)
{
    __asm__ volatile ("pushl %0; popfl" : : "r"(flags) : "memory", "cc");
}

#endif
