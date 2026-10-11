#ifndef SCHEDULER_H
#define SCHEDULER_H

// 启动时、关中断状态下初始化一次；初始化后再添加任务。
void scheduler_init(void);

/* 仅限启动阶段、开启中断前调用；成功返回 1，空入口或容量满返回 0。
 * 当前任务入口必须持续运行，不能 return。
 */
int scheduler_add_task(void (*entry)(void));

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