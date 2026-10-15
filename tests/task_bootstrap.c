/* 主机侧检查 32 位栈布局，并验证入口 return 后调用退出函数。
 * 64 位主机仅检查截断地址，不通过这些地址跳转。
 * gcc -std=c11 -Wall -Wextra -Werror -Wno-pointer-to-int-cast tests/task_bootstrap.c -o task-test
 */
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include "../kernel/task.c"

static jmp_buf exit_target;
static int entry_called;
static int exit_called;

void scheduler_exit(void)
{
    exit_called++;
    longjmp(exit_target, 1);
}

static void worker(void) { entry_called++; }

int main(void)
{
    static unsigned char stack[TASK_STACK_SIZE] __attribute__((aligned(16)));
    struct task task;
    task_create(&task, 42, stack, worker);
    unsigned int *frame = (unsigned int *)(stack + TASK_STACK_SIZE) - 16;
    assert(task.id == 42 && task.state == TASK_READY && task.sleep_ticks == 0);
    assert(task.esp == (unsigned int)(uintptr_t)frame);
    for (int i = 0; i < 8; i++) assert(frame[i] == 0);
    assert(frame[8] == (unsigned int)(uintptr_t)task_bootstrap);
    assert(frame[9] == 0x08 && frame[10] == 0x202);
    assert(frame[11] == 0); // cdecl 返回地址占位
    assert(frame[12] == (unsigned int)(uintptr_t)worker);
    assert(((uintptr_t)&frame[12] & 15) == 0); // 入口 ESP + 4 对齐

    if (setjmp(exit_target) == 0) task_bootstrap(worker);
    assert(entry_called == 1 && exit_called == 1);
    puts("task bootstrap tests passed");
    return 0;
}
