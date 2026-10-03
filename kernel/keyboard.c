#include "keyboard.h"
#include "io.h"
#include "pic.h"
#include "vga.h"
#include "shell.h"



//x86 传统 PS/2 键盘控制器的数据端口是：0x60
#define KEYBOARD_DATA_PORT 0x60

static int shift_pressed = 0;   //shift是否按下，1：按下，0：未按下
static int caps_lock = 0;

//scancode作为下标，对应的是小写字符
static const char scancode_normal[128] = {
    0, 27,
    '1','2','3','4','5','6','7','8','9','0',
    '-','=', '\b','\t',
    'q','w','e','r','t','y','u','i','o','p',
    '[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',
    ';','\'','`',
    0,
    '\\',
    'z','x','c','v','b','n','m',
    ',','.','/',
    0,
    '*',
    0,
    ' '
};

//shift表，对应的是大写的字符
static const char scancode_shift[128] = {
    0, 27,
    '!','@','#','$','%','^','&','*','(',')',
    '_','+', '\b','\t',
    'Q','W','E','R','T','Y','U','I','O','P',
    '{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',
    ':','"','~',
    0,
    '|',
    'Z','X','C','V','B','N','M',
    '<','>','?',
    0,
    '*',
    0,
    ' '
};


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
    //从端口读取一个字符
    unsigned char scancode = inb(KEYBOARD_DATA_PORT);

    // 0x2A:左边的shift键按下，0x36:右边的shift键按下
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        pic_send_eoi(1);
        return;
    }

    // Shift release，左右两边shift键松开
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
        pic_send_eoi(1);
        return;
    }

    // Caps Lock press ， 大写键按下
    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        pic_send_eoi(1);
        return;
    }

    // 普通 release 暂时忽略
    if (scancode & 0x80) {
        pic_send_eoi(1);
        return;
    }

    if (scancode < 128) {
        char c;

        if (shift_pressed) {
            c = scancode_shift[scancode];
        } else {
            c = scancode_normal[scancode];
        }

        // Caps Lock 只影响字母
        if (caps_lock) {
            if (c >= 'a' && c <= 'z') {
                c = c - 'a' + 'A';
            } else if (c >= 'A' && c <= 'Z') {
                c = c - 'A' + 'a';
            }
        }

        if (c != 0) {
            shell_input_char(c);
        }
    }

    pic_send_eoi(1);
}