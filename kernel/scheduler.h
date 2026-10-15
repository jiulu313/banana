#ifndef SCHEDULER_H
#define SCHEDULER_H

// 启动时、关中断状态下初始化一次，创建 0 号 idle；之后最多添加 7 个普通任务。
void scheduler_init(void);

/* 初始化后、启动阶段关中断或普通任务开中断时调用。
 * 成功返回 1，失败返回 0。入口可以直接 return，自动结束任务。
 * 已退出且切出的任务槽位和静态栈会在创建新任务时复用。
 */
int scheduler_add_task(void (*entry)(void));

/* 同 add_task，但成功返回非零任务 ID，失败返回 0。
 * ID 与槽位无关，复用槽位不会复用 ID；ID 耗尽后拒绝创建。
 * 最多同时容纳 7 个普通任务；不支持 IRQ 上下文调用。
 */
unsigned int scheduler_spawn(void (*entry)(void));

// 当前普通任务的 ID；启动阶段或 idle 返回 0。
unsigned int scheduler_current_id(void);

/* 普通内核任务中、开中断时调用；恢复运行后返回 0。
 * 启动线程或关中断/IRQ 上下文调用返回 -1。
 */
int scheduler_yield(void);

/* 普通内核任务、开中断时调用；idle/启动线程/关中断时返回 -1。
 * ticks 为 0 时等同主动让出；休眠到期并恢复运行后返回 0。
 */
int scheduler_sleep(unsigned int ticks);

/* 结束当前普通内核任务，永不返回；只允许在开中断的任务上下文调用。
 * idle/启动线程/关中断上下文误用会停机。切出后槽位和栈可供新任务复用。
 */
void scheduler_exit(void) __attribute__((noreturn));

// 软件中断入口使用，参数和返回值都是 pusha 现场的地址。
unsigned int scheduler_on_yield(unsigned int current_esp);

/*
 * 参数：
 * current_esp = 当前任务中断现场的栈顶
 *
 * 返回：
 * 下一任务的栈顶
 */
unsigned int scheduler_on_tick(
    unsigned int current_esp
);




#endif
