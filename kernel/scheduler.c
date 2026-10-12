#include "scheduler.h"
#include "task.h"

// 包含 0 号 idle，最多再添加 7 个普通任务。
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

static void idle_task(void)
{
    // 初始现场的 EFLAGS 已设置 IF=1，时钟/键盘中断可以唤醒 CPU。
    while (1) {
        __asm__ volatile ("hlt");
    }
}

static int scheduler_pick_next(void)
{
    // 从当前任务之后开始找一圈，优先选择就绪的普通任务。
    for (int offset = 1; offset <= task_count; offset++) {
        int next = (current_task + offset) % task_count;
        if (next != 0 && tasks[next].state == TASK_READY) {
            return next;
        }
    }
    return 0; // 没有普通任务就绪时，才运行 idle。
}

// 仅在启动阶段、关中断时添加任务；成功返回 1，失败返回 0。
int scheduler_add_task(void (*entry)(void))
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

    scheduler_add_task(idle_task); // 固定占用 0 号槽位和独立栈。
}

int scheduler_yield(void)
{
    unsigned int flags;
    __asm__ volatile ("pushfl; popl %0" : "=r"(flags) : : "memory");
    if (current_task == -1 || !(flags & 0x200)) {
        return -1;
    }

    // CPU 自动保存 EIP/CS/EFLAGS，入口再 pusha，形成与 IRQ0 相同的现场。
    __asm__ volatile ("int $0x81" : : : "memory");
    return 0; // 只有这个任务再次被选中并恢复后，才执行到这里。
}


int scheduler_sleep(unsigned int ticks)
{
    unsigned int flags;
    // 修改休眠状态和切换必须连续完成，不能在中间被时钟中断打断。
    __asm__ volatile ("pushfl; popl %0; cli" : "=r"(flags) : : "memory");
    if (current_task <= 0 || !(flags & 0x200)) {
        __asm__ volatile ("pushl %0; popfl" : : "r"(flags) : "memory", "cc");
        return -1;
    }

    if (ticks != 0) {
        tasks[current_task].sleep_ticks = ticks;
        tasks[current_task].state = TASK_SLEEPING;
    }

    // 不能调用 scheduler_yield()，因为它会拒绝当前关中断的状态。
    __asm__ volatile ("int $0x81" : : : "memory");
    // 此任务恢复时 IF 仍为 0；恢复调用前的标志，重新允许硬件中断。
    __asm__ volatile ("pushl %0; popfl" : : "r"(flags) : "memory", "cc");
    return 0;
}

unsigned int scheduler_on_tick(unsigned int current_esp)
{
    // 没有任务时，返回原现场，继续执行被中断的代码。
    if (task_count == 0) {
        return current_esp;
    }

    // 每次真实时钟中断推进休眠倒计时；主动让出不消耗休眠时间。
    for (int i = 1; i < task_count; i++) {
        if (tasks[i].state == TASK_SLEEPING && tasks[i].sleep_ticks > 0) {
            tasks[i].sleep_ticks--;
            if (tasks[i].sleep_ticks == 0) {
                tasks[i].state = TASK_READY;
            }
        }
    }

    /*
     * 第一次时钟中断发生时，
     * 当前运行的是 kernel_main，
     * 还没有任何正式 task 在运行。
     */
    if (current_task == -1) {

        current_task = scheduler_pick_next();

        tasks[current_task].state = TASK_RUNNING;

        /*
         * 不保存 current_esp！
         *
         * 因为这个 ESP 属于 kernel_main，
         * 不属于选中的任务。
         *
         * 直接返回选中任务预先准备好的 ESP。
         */
        return tasks[current_task].esp;
    }

    /*
     * 从第二次 tick 开始，
     * 当前真的正在运行某个 task。
     */
    tasks[current_task].esp = current_esp;

    slice_ticks++;
    if (current_task != 0 && slice_ticks < TASK_TIME_SLICE) {
        // 时间片还没用完：返回同一份现场，继续运行当前任务。
        return current_esp;
    }

    return scheduler_on_yield(current_esp);
}

// 时间片耗尽和主动让出共用这段轮转逻辑；这里不增加系统 tick。
unsigned int scheduler_on_yield(unsigned int current_esp)
{
    if (task_count == 0 || current_task == -1) {
        return current_esp;
    }

    tasks[current_task].esp = current_esp;
    slice_ticks = 0; // 下一任务从一个完整的新时间片开始。
    // 休眠任务要继续保持 SLEEPING，不能在切出时立即变回 READY。
    if (tasks[current_task].state == TASK_RUNNING) {
        tasks[current_task].state = TASK_READY;
    }

    /*
     * 选择下一个任务
     */
    current_task = scheduler_pick_next();

    tasks[current_task].state = TASK_RUNNING;

    return tasks[current_task].esp;
}
