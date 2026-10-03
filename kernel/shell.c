#include "shell.h"
#include "vga.h"

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
    } else if (command[0] == '\0') {
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