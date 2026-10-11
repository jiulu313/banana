#include "scheduler.h"
#include "task.h"
#include "vga.h"

// 容量上限不等于当前实际任务数。
#define TASK_MAX 8

static struct task tasks[TASK_MAX];

static unsigned char task_stack_a[TASK_STACK_SIZE]
    __attribute__((aligned(16)));

static unsigned char task_stack_b[TASK_STACK_SIZE]
    __attribute__((aligned(16)));

static int current_task = -1;
static int task_count = 0;



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


// 仅在启动阶段、关中断时添加任务；成功返回 1，失败返回 0。
static int scheduler_add_task(unsigned char *stack, void (*entry)(void))
{
    if (task_count >= TASK_MAX || stack == 0 || entry == 0) {
        return 0;
    }

    task_create(&tasks[task_count], task_count, stack, entry);
    task_count++; // 准备完成后，这个任务才计入调度范围。
    return 1;
}

void scheduler_init(void)
{
    task_count = 0;
    current_task = -1;

    scheduler_add_task(task_stack_a, task_a);
    scheduler_add_task(task_stack_b, task_b);
}


unsigned int scheduler_on_tick(unsigned int current_esp)
{
    // 没有任务时，返回原现场，继续执行被中断的代码。
    if (task_count == 0) {
        return current_esp;
    }

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

    if (current_task >= task_count) {
        current_task = 0;
    }

    tasks[current_task].state = TASK_RUNNING;

    return tasks[current_task].esp;
}