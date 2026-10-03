#include "keyboard.h"
#include "io.h"
#include "pic.h"
#include "vga.h"
#include "shell.h"



//x86 传统 PS/2 键盘控制器的数据端口是：0x60
#define KEYBOARD_DATA_PORT 0x60


// scancode作为数组索引，值为对应的字符
static const char scancode_table[128] =
{
    0,
    27,     // Esc

    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=',
    '\b',   // Backspace

    '\t',   // Tab

    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']',

    '\n',   // Enter

    0,      // Left Ctrl

    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', '\'', '`',

    0,      // Left Shift

    '\\',

    'z', 'x', 'c', 'v', 'b', 'n', 'm',
    ',', '.', '/',

    0,      // Right Shift

    '*',

    0,      // Alt

    ' ',    // Space
};



//键盘中断处理函数
void keyboard_handler(void)
{
    unsigned char scancode;

    scancode = inb(KEYBOARD_DATA_PORT);

    // bit7 = 1 通常表示按键释放
    if (scancode & 0x80) {
        pic_send_eoi(1);
        return;
    }

    if (scancode < 128) {
        char c = scancode_table[scancode];

        if (c != 0) {
            shell_input_char(c); //发给shell,决定如何处理
        }
    }

    pic_send_eoi(1);
}