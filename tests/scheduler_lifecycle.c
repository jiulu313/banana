/* 主机侧状态机测试：模拟 IF 和任务栈，不执行 int/iret/cli。
 * gcc -std=c11 -Wall -Wextra -Werror tests/scheduler_lifecycle.c -o scheduler-test
 */
#include <assert.h>
#include <stdio.h>

// 使用可观测的 IF 模型代替真实特权指令。
#define IRQ_H
static unsigned int mock_flags;
static unsigned int irq_flags(void) { return mock_flags; }
static unsigned int irq_save(void)
{
    unsigned int flags = mock_flags;
    mock_flags &= ~0x200u;
    return flags;
}
static void irq_restore(unsigned int flags) { mock_flags = flags; }

#include "../kernel/scheduler.c"

void task_create(struct task *task, unsigned int id,
                 unsigned char *stack, void (*entry)(void))
{
    assert(!(mock_flags & 0x200));
    assert(stack != 0 && entry != 0);
    task->id = id;
    task->esp = 0x1000 + id * 0x100;
    task->state = TASK_READY;
    task->sleep_ticks = 0;
}

static void worker(void) {}

int main(void)
{
    assert(scheduler_spawn(worker) == 0); // 未初始化
    scheduler_init();
    assert(task_count == 1 && scheduler_current_id() == 0);
    assert(scheduler_spawn(0) == 0);
    for (unsigned int id = 1; id <= 7; id++) {
        assert(scheduler_spawn(worker) == id);
        assert(mock_flags == 0); // 启动时保持关中断
    }
    assert(scheduler_spawn(worker) == 0); // 容量耗尽

    assert(scheduler_on_tick(0xdead) == tasks[1].esp);
    assert(scheduler_current_id() == 1);
    assert(scheduler_spawn(worker) == 0); // 运行时关中断拒绝创建
    mock_flags = 0x202;
    assert(scheduler_spawn(0) == 0 && mock_flags == 0x202);
    assert(scheduler_spawn(worker) == 0 && mock_flags == 0x202);

    // 正在退出的任务尚未切出，自己的栈绝不能被覆盖。
    tasks[1].state = TASK_EXITED;
    assert(scheduler_spawn(worker) == 0);
    scheduler_on_yield(0x1111);
    assert(current_task == 2 && tasks[1].state == TASK_EXITED);
    unsigned int replacement = scheduler_spawn(worker);
    assert(replacement == 8 && tasks[1].id == 8);
    assert(tasks[1].state == TASK_READY && tasks[1].sleep_ticks == 0);
    assert(task_count == 8 && mock_flags == 0x202);

    // 连续创建/退出远超槽位上限：只复用槽位，不复用 ID。
    for (int n = 0; n < 32; n++) {
        tasks[1].state = TASK_EXITED;
        unsigned int id = scheduler_spawn(worker);
        assert(id > replacement && tasks[1].id == id);
        replacement = id;
    }

    // 休眠任务不可复用，yield 不推进倒计时，真实 tick 才推进。
    tasks[1].state = TASK_SLEEPING;
    tasks[1].sleep_ticks = 2;
    assert(scheduler_spawn(worker) == 0);
    scheduler_on_yield(0x2222);
    assert(tasks[1].sleep_ticks == 2);
    scheduler_on_tick(0x3333);
    assert(tasks[1].sleep_ticks == 1 && tasks[1].state == TASK_SLEEPING);
    scheduler_on_tick(0x3333);
    assert(tasks[1].sleep_ticks == 0 && tasks[1].state == TASK_READY);

    // ID 溢出后停止分配，不让旧 ID 指向新任务。
    tasks[1].state = TASK_EXITED;
    next_task_id = ~0u;
    assert(scheduler_spawn(worker) == ~0u);
    tasks[1].state = TASK_EXITED;
    assert(scheduler_spawn(worker) == 0 && mock_flags == 0x202);

    // 所有普通任务退出后退回 idle，idle 不允许创建普通任务。
    for (int i = 1; i < task_count; i++) tasks[i].state = TASK_EXITED;
    scheduler_on_yield(0x4444);
    assert(current_task == 0 && scheduler_current_id() == 0);
    next_task_id = 100;
    assert(scheduler_spawn(worker) == 0);
    puts("scheduler lifecycle tests passed");
    return 0;
}
