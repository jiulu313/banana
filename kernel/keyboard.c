#include "keyboard.h"
#include "io.h"
#include "pic.h"
#include "vga.h"

#define KEYBOARD_DATA_PORT 0x60

//键盘中断处理函数
void keyboard_handler(void)
{
    unsigned char scancode;

    //x86 传统 PS/2 键盘控制器的数据端口是：0x60
    scancode = inb(KEYBOARD_DATA_PORT);

    vga_write("key: ");
    vga_write_hex(scancode);
    vga_write("\n");

    //IRQ1 已经处理完成，可以处理下一个irq1了
    pic_send_eoi(1);
}