#ifndef TASK_H
#define TASK_H

//栈的大小，单位字节
#define TASK_STACK_SIZE 4096

#define TASK_READY   0
#define TASK_RUNNING 1
#define TASK_SLEEPING 2
#define TASK_EXITED   3

struct task
{
    unsigned int id;

    unsigned int esp;
    // unsigned int ebp;

    unsigned int state;
    unsigned int sleep_ticks; // 距离恢复就绪还剩多少个时钟 tick。
};

/* 调度器在关中断时调用；task、stack、entry 必须有效。
 * stack 指向独立的 TASK_STACK_SIZE 字节栈，生命周期覆盖任务运行期。
 * 任务入口可以直接 return，bootstrap 会自动调用 scheduler_exit()。
 */
void task_create(
    struct task *task,
    unsigned int id,
    unsigned char *stack,
    void (*entry)(void));

#endif
