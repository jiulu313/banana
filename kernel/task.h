#ifndef TASK_H
#define TASK_H

//栈的大小，单位字节
#define TASK_STACK_SIZE 4096

#define TASK_READY   0
#define TASK_RUNNING 1
#define TASK_SLEEPING 2

struct task
{
    unsigned int id;

    unsigned int esp;
    // unsigned int ebp;

    unsigned int state;
    unsigned int sleep_ticks; // 距离恢复就绪还剩多少个时钟 tick。
};

/* 启动阶段关中断时调用；task、stack、entry 必须有效。
 * stack 指向独立的 TASK_STACK_SIZE 字节栈，生命周期覆盖任务运行期。
 * 本步骤的任务入口必须像 task_a/task_b 一样持续运行，不能 return。
 */
void task_create(
    struct task *task,
    unsigned int id,
    unsigned char *stack,
    void (*entry)(void));

#endif
