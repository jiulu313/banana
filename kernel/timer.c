#include "timer.h"
#include "io.h"
#include "pic.h"
#include "vga.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_FREQUENCY 1193182   //经典 PIT 输入频率。

static volatile unsigned int ticks = 0;

void timer_init(unsigned int frequency)
{
    unsigned int divisor = PIT_FREQUENCY / frequency;

    outb(PIT_COMMAND, 0x36);    //设置 PIT 工作模式 

    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}

//timer处理函数
void timer_handler(void)
{
    ticks++;

    //注释掉
    // if ((ticks % 100) == 0) {
    //     vga_write("tick\n");
    // }

    //“IRQ0 我处理完了，你可以继续接收下一次 IRQ0。”
    pic_send_eoi(0);
}

//返回系统已经运行了多少 tick
unsigned int timer_get_ticks(void) 
{
    return ticks;
}
