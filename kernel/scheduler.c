#include "scheduler.h"
#include "vga.h"

static int current_task = 0;

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

void scheduler_init(void)
{
    current_task = 0;
}

void scheduler_tick(void)
{
    if (current_task == 0) {
        current_task = 1;
    } else {
        current_task = 0;
    }
}