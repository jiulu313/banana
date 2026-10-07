#include "task.h"

static struct task current_task;

void task_init(void)
{
    current_task.id = 0;
    current_task.state = TASK_RUNNING;
}