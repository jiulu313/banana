#ifndef SCHEDULER_H
#define SCHEDULER_H

// 启动时、关中断状态下初始化一次；初始化后再添加任务。
void scheduler_init(void);

/* 仅限启动阶段、开启中断前调用；成功返回 1，空入口或容量满返回 0。
 * 当前任务入口必须持续运行，不能 return。
 */
int scheduler_add_task(void (*entry)(void));

/* 普通内核任务中、开中断时调用；恢复运行后返回 0。
 * 启动线程或关中断/IRQ 上下文调用返回 -1。
 */
int scheduler_yield(void);

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
