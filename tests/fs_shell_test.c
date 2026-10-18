/* gcc -std=c11 -Wall -Wextra -Werror -Wno-pointer-to-int-cast tests/fs_shell_test.c -o fs-shell-test */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define IRQ_H
static unsigned int irq_save(void) { return 0; }
static void irq_restore(unsigned int flags) { assert(flags == 0); }
#include "../kernel/fs.c"
// 主机使用 libc 内存函数；其 size_t 与内核 32 位声明不同。
#define MEMORY_H
static void *kmalloc(unsigned int size) { (void)size; return 0; }
#include "../kernel/shell.c"

static char output[8192];
static unsigned int output_length;
void vga_putc(char c)
{
    assert(output_length + 1 < sizeof(output));
    output[output_length++] = c;
    output[output_length] = '\0';
}
void vga_write(const char *s) { while (*s) vga_putc(*s++); }
void vga_clear(void) { output_length = 0; output[0] = '\0'; }
void vga_write_hex(unsigned int value)
{
    char text[16];
    snprintf(text, sizeof(text), "0x%x", value);
    vga_write(text);
}
unsigned int timer_get_ticks(void) { return 123; }
void e820_print_map(void) {}
void *pmm_alloc_page(void) { return 0; }
void pmm_free_page(void *ptr) { (void)ptr; }
unsigned int pmm_get_total_pages(void) { return 0; }
unsigned int pmm_get_free_pages(void) { return 0; }

static void command(const char *s)
{
    vga_clear();
    while (*s) shell_input_char(*s++);
    shell_input_char('\n');
}

int main(void)
{
    fs_init();
    shell_init();
    command("ls");
    assert(strstr(output, "(empty)") != 0);
    command("touch note");
    command("write note hello world");
    command("append /note !");
    command("touch note"); // 不截断已有内容
    command("cat note");
    assert(strstr(output, "hello world!\nbanana> ") != 0);
    command("ls");
    assert(strstr(output, "note  size=0xc bytes") != 0);
    command("rm note extra");
    assert(strstr(output, "too many arguments") != 0);
    command("touch /bad/path");
    assert(strstr(output, "invalid name") != 0);
    command("touch");
    assert(strstr(output, "missing") != 0);
    command("write note");
    char data[FS_FILE_CAPACITY];
    assert(fs_read("note", 0, data, sizeof(data)) == 0);
    command("write note keep");
    char long_line[200];
    memset(long_line, 'x', sizeof(long_line));
    memcpy(long_line, "write note ", 11);
    long_line[sizeof(long_line) - 1] = '\0';
    command(long_line);
    assert(strstr(output, "command discarded") != 0);
    assert(fs_read("note", 0, data, sizeof(data)) == 4 && memcmp(data, "keep", 4) == 0);
    command("touch abcdefghijklmnopqrstuvwxyz123456");
    assert(strstr(output, "invalid name") != 0);
    command("write note new"); // 超长输入后能正常接受下一行
    command("cat note");
    assert(strstr(output, "new\nbanana> ") != 0);
    command("rm note");
    command("cat note");
    assert(strstr(output, "file not found") != 0);
    command("help");
    assert(strstr(output, "append NAME TEXT") != 0);
    command("version");
    assert(strstr(output, "banana OS 0.1") != 0);
    puts("RAMFS shell tests passed");
    return 0;
}
