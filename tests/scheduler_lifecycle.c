/* 主机侧状态机测试：模拟 IF 和任务栈，不执行 int/iret/cli。
 * gcc -std=c11 -Wall -Wextra -Werror tests/scheduler_lifecycle.c -o scheduler-test
 */
#include <assert.h>
#include <stdio.h>
#include <setjmp.h>

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
static void (*on_schedule)(void);
static void irq_schedule(void)
{
    assert(on_schedule != 0);
    on_schedule();
}

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
    task->parent_id = 0;
    task->wait_id = 0;
    task->exit_status = 0;
}

static void worker(void) {}
static jmp_buf exited;
static void leave_exited_task(void)
{
    assert(!(mock_flags & 0x200));
    assert(tasks[current_task].state == TASK_EXITED);
    scheduler_on_yield(0);
    longjmp(exited, 1); // 模拟退出任务永不恢复
}

static void test_exit_api(void)
{
    mock_flags = 0;
    scheduler_init();
    scheduler_spawn(worker);
    scheduler_on_tick(0);
    mock_flags = 0x202;
    on_schedule = leave_exited_task;
    if (setjmp(exited) == 0) scheduler_exit_status(-42);
    assert(current_task == 0 && tasks[1].exit_status == -42);
    mock_flags = 0;
    scheduler_init();
    scheduler_spawn(worker);
    scheduler_on_tick(0);
    mock_flags = 0x202;
    if (setjmp(exited) == 0) scheduler_exit();
    assert(current_task == 0 && tasks[1].exit_status == 0);
}

// 模拟中断切换和子任务退出，返回时父任务已经重新获得 CPU。
static void finish_waited_child(void)
{
    assert(mock_flags == 2); // wait 的检查、阻塞和切换之间保持关中断
    assert(current_task == 1 && tasks[1].state == TASK_WAITING);
    assert(tasks[1].wait_id == tasks[2].id);
    scheduler_on_yield(0x1111);
    assert(current_task == 2);
    scheduler_on_tick(0x2222);
    assert(tasks[1].state == TASK_WAITING); // tick 不唤醒等待者
    scheduler_finish_current(-7);
    assert(tasks[1].state == TASK_READY);
    scheduler_on_yield(0x2222);
    assert(current_task == 1 && tasks[1].state == TASK_RUNNING);
}

static void test_wait(void)
{
    mock_flags = 0;
    scheduler_init();
    assert(scheduler_wait(1, 0) == -1);
    unsigned int parent = scheduler_spawn(worker);
    scheduler_on_tick(0);
    assert(scheduler_wait(parent, 0) == -1); // 关中断
    mock_flags = 0x202;
    assert(scheduler_wait(0, 0) == -1);
    assert(scheduler_wait(parent, 0) == -1); // 自己不是自己的子任务
    assert(scheduler_wait(999, 0) == -1);
    unsigned int child = scheduler_spawn(worker);
    assert(tasks[2].parent_id == parent);
    on_schedule = finish_waited_child;
    int status = 123;
    assert(scheduler_wait(child, &status) == 0 && status == -7);
    assert(mock_flags == 0x202 && tasks[1].wait_id == 0);
    assert(scheduler_wait(child, &status) == -1);
    assert(status == -7);

    // 子先退出：必须保留结果，其他创建不能覆盖它。
    child = scheduler_spawn(worker);
    scheduler_on_yield(0);
    assert(current_task == 2);
    assert(scheduler_wait(parent, 0) == -1); // 不能等待父任务
    scheduler_finish_current(42);
    scheduler_on_yield(0);
    unsigned int sibling = scheduler_spawn(worker);
    assert(tasks[2].id == child && tasks[3].id == sibling);
    on_schedule = 0; // 已退出的子任务不应触发切换
    assert(scheduler_wait(child, &status) == 0 && status == 42);
    assert(scheduler_wait(child, 0) == -1);

    // 不同子任务退出不能误唤醒父任务；父先退出，清除全部子任务所有权。
    tasks[1].state = TASK_WAITING;
    tasks[1].wait_id = 999;
    current_task = 3;
    scheduler_finish_current(5);
    assert(tasks[1].state == TASK_WAITING);
    current_task = 1;
    tasks[1].state = TASK_RUNNING;
    assert(scheduler_wait(sibling, 0) == 0); // 可忽略退出码
    unsigned int orphan = scheduler_spawn(worker);
    assert(tasks[2].id == orphan);
    // 再留一个尚未回收的退出子任务。
    unsigned int zombie = scheduler_spawn(worker);
    tasks[3].state = TASK_EXITED;
    assert(tasks[3].id == zombie && tasks[3].parent_id == parent);
    scheduler_finish_current(0);
    assert(tasks[2].parent_id == 0 && tasks[3].parent_id == 0);
    scheduler_on_yield(0);
    assert(current_task == 2);
    assert(scheduler_spawn(worker) != 0); // 可复用已退出的父任务槽位
    assert(scheduler_wait(zombie, 0) == -1); // 不能回收兄弟任务
    assert(scheduler_wait(child, 0) == -1); // 槽位复用后旧 ID 仍然无效
}

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
        tasks[1].parent_id = 0; // 模拟已由父任务回收
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
    tasks[1].parent_id = 0;
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
    test_wait();
    test_exit_api();
    puts("scheduler lifecycle tests passed");
    return 0;
}
