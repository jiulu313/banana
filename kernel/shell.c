#include "shell.h"
#include "vga.h"
#include "timer.h"
#include "memory.h"
#include "e820.h"

#define INPUT_BUFFER_SIZE 128

static char input_buffer[INPUT_BUFFER_SIZE];
static int input_length = 0;

//判断两个字符串是否相等
//相等返回 1 ，不相等返回 0
static int str_equal(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return 0;
        }

        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}


//shell提示符
static void shell_prompt(void)
{
    vga_write("banana> ");
}

//判断是哪个命令，走相应的流程
static void shell_execute(const char *command)
{
    if (str_equal(command, "help")) {
        vga_write("commands:\n");
        vga_write("  help\n");
        vga_write("  clear\n");
        vga_write("  version\n");
    } else if (str_equal(command, "clear")) {
        vga_clear();
    } else if (str_equal(command, "version")) {
        vga_write("banana OS 0.1\n");
    } else if (str_equal(command,"ticks")){
        vga_write("ticks:");
        vga_write_hex(timer_get_ticks());
        vga_write("\n");
    } else if (str_equal(command,"alloc")){
        void *ptr = kmalloc(64);

        vga_write("allocated at: ");
        vga_write_hex((unsigned int)ptr);
        vga_write("\n");
    } else if (str_equal(command,"memtest")){
        char *a = (char *)kmalloc(32);
        char *b = (char *)kmalloc(32);

        if (a == 0 || b == 0) {
            vga_write("kmalloc failed\n");
        } else {

            a[0] = 'B';
            a[1] = 'A';
            a[2] = 'N';
            a[3] = 'A';
            a[4] = 'N';
            a[5] = 'A';
            a[6] = '\0';

            memset(b, 0, 32);
            memcpy(b, a, 7);

            vga_write("a: ");
            vga_write(a);
            vga_write("\n");

            vga_write("b: ");
            vga_write(b);
            vga_write("\n");

            vga_write("a addr: ");
            vga_write_hex((unsigned int)a);
            vga_write("\n");

            vga_write("b addr: ");
            vga_write_hex((unsigned int)b);
            vga_write("\n");
        }
    } else if (str_equal(command,"memmap")) {
        e820_print_map();//调用BIOS e820打印内存情况 
    }
    else if (command[0] == '\0') {
        // 空命令，不做任何事情
    } else {
        vga_write("unknown command: ");
        vga_write(command);
        vga_write("\n");
    }
}

//初始化
void shell_init(void)
{
    input_length = 0;

    vga_write("banana shell\n");
    shell_prompt();
}


void shell_input_char(char c)
{
    if (c == '\n') {
        vga_putc('\n');

        input_buffer[input_length] = '\0';

        shell_execute(input_buffer);

        input_length = 0;

        shell_prompt();

        return;
    }

    if (c == '\b') {
        if (input_length > 0) {
            input_length--;

            vga_putc('\b');
        }

        return;
    }

    if (input_length < INPUT_BUFFER_SIZE - 1) {
        input_buffer[input_length] = c;
        input_length++;

        vga_putc(c);
    }
}