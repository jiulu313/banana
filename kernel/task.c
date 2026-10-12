#include "task.h"

// 给“新任务”提前伪造一份 CPU 中断现场，
// 让调度器第一次切到这个任务时，
// popa + iret 能像“恢复旧任务”一样把它启动起来。
static unsigned int prepare_task_stack(
    unsigned char *stack,
    void (*entry)(void))
{
    unsigned int *sp =
        (unsigned int *)(stack + TASK_STACK_SIZE);

    /*
     * iret 将来需要：
     *
     * EIP
     * CS
     * EFLAGS
     *
     * 栈向低地址增长，
     * 所以反着 push。
     */


    *--sp = 0x202;              // EFLAGS
    *--sp = 0x08;               // CS
    *--sp = (unsigned int)entry; // EIP

    /*
     * popa 将来需要的布局：
     *
     * EDI
     * ESI
     * EBP
     * old ESP (popa 会跳过)
     * EBX
     * EDX
     * ECX
     * EAX
     *
     * 同样反方向构造。
     */

    *--sp = 0;   // EAX
    *--sp = 0;   // ECX
    *--sp = 0;   // EDX
    *--sp = 0;   // EBX
    *--sp = 0;   // ESP dummy
    *--sp = 0;   // EBP
    *--sp = 0;   // ESI
    *--sp = 0;   // EDI

    return (unsigned int)sp;
}

// 这里只准备任务，真正开始运行由时钟中断中的调度器决定。
void task_create(
    struct task *task,
    unsigned int id,
    unsigned char *stack,
    void (*entry)(void))
{
    task->id = id;
    task->esp = prepare_task_stack(stack, entry);
    task->state = TASK_READY;
    task->sleep_ticks = 0;
}
