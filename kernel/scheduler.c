#include "scheduler.h"
#include "task.h"
#include "vga.h"

#define TASK_COUNT 2
#define STACK_SIZE 4096

static struct task tasks[TASK_COUNT];

static unsigned char task_stack_a[STACK_SIZE]
    __attribute__((aligned(16)));

static unsigned char task_stack_b[STACK_SIZE]
    __attribute__((aligned(16)));

static int current_task = -1;



static void task_a(void)
{
    while (1) {
        vga_putc('A');

        for (volatile unsigned int i = 0;
             i < 1000000;
             i++) {
        }
    }
}

static void task_b(void)
{
    while (1) {
        vga_putc('B');

        for (volatile unsigned int i = 0;
             i < 1000000;
             i++) {
        }
    }
}


// 给“新任务”提前伪造一份 CPU 中断现场，
// 让调度器第一次切到这个任务时，
// popa + iret 能像“恢复旧任务”一样把它启动起来。
static unsigned int prepare_task_stack(
    unsigned char *stack,
    void (*entry)(void))
{
    unsigned int *sp =
        (unsigned int *)(stack + STACK_SIZE);

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

void scheduler_init(void)
{
    tasks[0].id = 0;
    tasks[0].state = TASK_READY;

    tasks[0].esp =
        prepare_task_stack(
            task_stack_a,
            task_a
        );

    tasks[1].id = 1;
    tasks[1].state = TASK_READY;

    tasks[1].esp =
        prepare_task_stack(
            task_stack_b,
            task_b
        );

    /*
     * 当前还没有正式 task 在运行。
     */
    current_task = -1;
}


unsigned int scheduler_on_tick(
    unsigned int current_esp)
{
    /*
     * 第一次时钟中断发生时，
     * 当前运行的是 kernel_main，
     * 还没有任何正式 task 在运行。
     */
    if (current_task == -1) {

        current_task = 0;

        tasks[0].state = TASK_RUNNING;

        /*
         * 不保存 current_esp！
         *
         * 因为这个 ESP 属于 kernel_main，
         * 不是 task_a。
         *
         * 直接返回 task_a 预先准备好的 ESP。
         */
        return tasks[0].esp;
    }

    /*
     * 从第二次 tick 开始，
     * 当前真的正在运行某个 task。
     */
    tasks[current_task].esp = current_esp;

    tasks[current_task].state = TASK_READY;

    /*
     * 选择下一个任务
     */
    current_task++;

    if (current_task >= TASK_COUNT) {
        current_task = 0;
    }

    tasks[current_task].state = TASK_RUNNING;

    return tasks[current_task].esp;
}