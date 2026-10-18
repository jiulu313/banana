#include "task.h"
#include "scheduler.h"

// 所有新任务先进入这里：入口正常 return 后也会自动退出。
static void task_bootstrap(void (*entry)(void))
{
    entry();
    scheduler_exit();
}

// 给“新任务”提前伪造一份 CPU 中断现场，
// 让调度器第一次切到这个任务时，
// popa + iret 能像“恢复旧任务”一样把它启动起来。
static unsigned int prepare_task_stack(
    unsigned char *stack,
    void (*entry)(void))
{
    unsigned int *sp =
        (unsigned int *)(stack + TASK_STACK_SIZE);

    // 模拟 32 位 cdecl 调用：入口 ESP + 4 按 16 字节对齐，
    // [ESP] 是占位返回地址，[ESP + 4] 是 entry 参数。
    sp -= 3;
    *--sp = (unsigned int)entry;
    *--sp = 0; // bootstrap 永不返回，不会使用这个地址。

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
    *--sp = (unsigned int)task_bootstrap; // EIP

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
    task->parent_id = 0;
    task->wait_id = 0;
    task->exit_status = 0;
}
