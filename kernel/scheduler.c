#include "scheduler.h"
#include "task.h"
#include "irq.h"

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
static unsigned int next_task_id = 1; // ID 不等于槽位；复用栈时也分配新 ID。
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
        int next = (current_task + offset) % task_count;    //0-task_count-1,如果当前是0，task_count=3,则next=1,2,0
        if (next != 0 && tasks[next].state == TASK_READY) {
            return next;
        }
    }
    return 0; // 没有普通任务就绪时，才运行 idle。
}

unsigned int scheduler_spawn(void (*entry)(void))
{
    unsigned int flags = irq_save();
    // 初始化后才能创建；运行期间仅允许开中断的普通任务调用。
    if (task_count == 0 || entry == 0 || next_task_id == 0 ||
        (current_task != -1 && (current_task == 0 || !(flags & 0x200)))) {
        irq_restore(flags);
        return 0;
    }

    int slot;
    for (slot = 1; slot < task_count; slot++) {
        // 退出任务必须已经切出，才能覆盖它的栈。
        if (slot != current_task && tasks[slot].state == TASK_EXITED) {
            break;
        }
    }
    if (slot == TASK_MAX) {
        irq_restore(flags);
        return 0;
    }

    unsigned int id = next_task_id++;
    task_create(&tasks[slot], id, task_stacks[slot], entry);
    if (slot == task_count) {
        task_count++; // 记录已使用槽位的上界，不是活跃任务数。
    }
    irq_restore(flags);
    return id;
}

int scheduler_add_task(void (*entry)(void))
{
    return scheduler_spawn(entry) != 0;
}

unsigned int scheduler_current_id(void)
{
    return current_task > 0 ? tasks[current_task].id : 0;
}
void scheduler_init(void)
{
    task_count = 0;
    current_task = -1;
    slice_ticks = 0;
    next_task_id = 1;

    task_create(&tasks[0], 0, task_stacks[0], idle_task);
    task_count = 1; // idle 固定占用 0 号槽位和独立栈。
}

//让出 CPU 使用权，当前任务进入 READY 状态，调度器选择下一个任务运行。
int scheduler_yield(void)
{
    unsigned int flags = irq_flags();
    if (current_task == -1 || !(flags & 0x200)) {
        return -1;
    }

    // CPU 自动保存 EIP/CS/EFLAGS，入口再 pusha，形成与 IRQ0 相同的现场。
    __asm__ volatile ("int $0x81" : : : "memory");
    return 0; // 只有这个任务再次被选中并恢复后，才执行到这里。
}

// 让出 CPU 使用权，当前任务进入 SLEEPING 状态，调度器选择下一个任务运行。
int scheduler_sleep(unsigned int ticks)
{
    // 修改休眠状态和切换必须连续完成，不能在中间被时钟中断打断。
    unsigned int flags = irq_save();


    /**
     *  current_task == -1：还没开始运行正式任务。
        current_task == 0：当前是 idle，不能让它休眠。
        flags 的 IF 位为 0：调用之前就已经关闭了中断。
     */
    if (current_task <= 0 || !(flags & 0x200)) {
        irq_restore(flags);
        return -1;
    }

    if (ticks != 0) {
        tasks[current_task].sleep_ticks = ticks;
        tasks[current_task].state = TASK_SLEEPING;
    }

    // 不能调用 scheduler_yield()，因为它会拒绝当前关中断的状态。
    __asm__ volatile ("int $0x81" : : : "memory");
    // 此任务恢复时 IF 仍为 0；恢复调用前的标志，重新允许硬件中断。
    irq_restore(flags);
    return 0;
}

// 当前任务退出，调度器选择下一个任务运行。
void scheduler_exit(void)
{
    // 和休眠一样，标记状态与切出之间不能被时钟中断打断。
    unsigned int flags = irq_save();
    if (current_task > 0 && (flags & 0x200)) {
        tasks[current_task].sleep_ticks = 0;
        tasks[current_task].state = TASK_EXITED;
        __asm__ volatile ("int $0x81" : : : "memory");
        // 正常情况下不会回来：调度器只选择 READY 任务。
    }

    // 无法合法退出或意外恢复了已退出任务时，停机而不是继续执行。
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
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
    // 休眠或已退出的任务保持原状态，不能在切出时变回 READY。
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
