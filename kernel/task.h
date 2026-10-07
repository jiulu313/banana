#ifndef TASK_H
#define TASK_H

#define TASK_READY   0
#define TASK_RUNNING 1

struct task
{
    unsigned int id;

    unsigned int esp;
    // unsigned int ebp;

    unsigned int state;
};

void task_init(void);

#endif