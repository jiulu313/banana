#include "scheduler.h"
#include "task.h"
#include "vga.h"

// 容量上限不等于当前实际任务数。
#define TASK_MAX 8

// 每个任务连续运行 5 个时钟 tick，再轮到下一个任务。
#define TASK_TIME_SLICE 5

static struct task tasks[TASK_MAX];

// 每个任务槽位对应一块独立的栈，启动时直接使用，不需要动态申请。
// TASK_STACK_SIZE 为 4096，因此每一行也保持 16 字节对齐。
static unsigned char task_stacks[TASK_MAX][TASK_STACK_SIZE]
    __attribute__((aligned(16)));

static int current_task = -1;
static int task_count = 0;
static unsigned int slice_ticks = 0; // 当前这一轮时间片已使用的 tick 数。



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
static int scheduler_add_task(void (*entry)(void))
{
    if (task_count >= TASK_MAX || entry == 0) {
        return 0;
    }

    task_create(&tasks[task_count], task_count,
                task_stacks[task_count], entry);
    task_count++; // 准备完成后，这个任务才计入调度范围。
    return 1;
}

void scheduler_init(void)
{
    task_count = 0;
    current_task = -1;
    slice_ticks = 0;

    scheduler_add_task(task_a);
    scheduler_add_task(task_b);
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

    slice_ticks++;
    if (slice_ticks < TASK_TIME_SLICE) {
        // 时间片还没用完：返回同一份现场，继续运行当前任务。
        return current_esp;
    }

    slice_ticks = 0; // 下一任务从一个完整的新时间片开始。
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
