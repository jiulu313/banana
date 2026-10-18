#include "shell.h"
#include "vga.h"
#include "timer.h"
#include "memory.h"
#include "e820.h"
#include "pmm.h"
#include "fs.h"

#define INPUT_BUFFER_SIZE 128

static char input_buffer[INPUT_BUFFER_SIZE];
static int input_length = 0;
static int input_overflow = 0;

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


static const char *skip_space(const char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

// 取一个以空格分隔的参数；过长时报错，不截断后执行另一个文件名。
static int shell_token(const char **cursor, char *out, unsigned int capacity)
{
    const char *s = skip_space(*cursor);
    unsigned int n = 0;
    while (*s != '\0' && *s != ' ' && *s != '\t') {
        if (n + 1 >= capacity) return -1;
        out[n++] = *s++;
    }
    out[n] = '\0';
    *cursor = s;
    return (int)n;
}

static void shell_fs_error(int result)
{
    if (result >= 0) return;
    switch (result) {
    case FS_ERR_INVALID: vga_write("fs: invalid name or argument\n"); break;
    case FS_ERR_NOT_FOUND: vga_write("fs: file not found\n"); break;
    case FS_ERR_EXISTS: vga_write("fs: file already exists\n"); break;
    case FS_ERR_FULL: vga_write("fs: file table full\n"); break;
    case FS_ERR_TOO_LARGE: vga_write("fs: file exceeds 1024 bytes\n"); break;
    default: vga_write("fs: error\n"); break;
    }
}

// 返回 1 表示已处理文件命令；其他命令交给原来的 shell_execute。
static int shell_fs_execute(const char *line)
{
    char command[16];
    const char *args = line;
    if (shell_token(&args, command, sizeof(command)) <= 0) return 0;
    if (str_equal(command, "ls")) {
        if (*skip_space(args) != '\0') {
            vga_write("usage: ls\n");
            return 1;
        }
        struct fs_info info;
        int count = 0;
        for (unsigned int slot = 0; slot < FS_MAX_FILES; slot++) {
            if (fs_list(slot, &info) != 1) continue;
            vga_write(info.name);
            vga_write("  size=");
            vga_write_hex(info.size);
            vga_write(" bytes\n");
            count++;
        }
        if (count == 0) vga_write("(empty)\n");
        return 1;
    }
    int write = str_equal(command, "write");
    int append = str_equal(command, "append");
    int touch = str_equal(command, "touch");
    int remove = str_equal(command, "rm");
    int cat = str_equal(command, "cat");
    if (!write && !append && !touch && !remove && !cat) return 0;

    char name[FS_NAME_MAX + 2]; // 允许可选的前导 /。
    if (shell_token(&args, name, sizeof(name)) <= 0) {
        vga_write("fs: missing or too long filename\n");
        return 1;
    }
    args = skip_space(args);
    if (!write && !append && *args != '\0') {
        vga_write("fs: too many arguments\n");
        return 1;
    }
    if (write || append) {
        unsigned int length = 0;
        while (args[length] != '\0') length++;
        shell_fs_error(write ? fs_write(name, args, length) : fs_append(name, args, length));
    } else if (touch) {
        int result = fs_create(name);
        if (result != FS_ERR_EXISTS) shell_fs_error(result); // touch 不清空已有文件。
    } else if (remove) {
        shell_fs_error(fs_remove(name));
    } else {
        // 小块读取，避免在任务的 4 KiB 栈上放整个文件。
        char buffer[64];
        unsigned int offset = 0;
        int count;
        while ((count = fs_read(name, offset, buffer, sizeof(buffer))) > 0) {
            for (int i = 0; i < count; i++) vga_putc(buffer[i]);
            offset += (unsigned int)count;
        }
        if (count < 0) shell_fs_error(count);
        else vga_putc('\n');
    }
    return 1;
}

//shell提示符
static void shell_prompt(void)
{
    vga_write("banana> ");
}

//判断是哪个命令，走相应的流程
static void shell_execute(const char *command)
{
    if (shell_fs_execute(command)) return;
    if (str_equal(command, "help")) {
        vga_write("commands:\n");
        vga_write("  help\n");
        vga_write("  clear\n");
        vga_write("  version\n");
        vga_write("  ls | touch NAME | cat NAME | rm NAME\n");
        vga_write("  write NAME TEXT | append NAME TEXT\n");
        vga_write("RAMFS: 16 files, 1024 bytes/file; lost on reboot.\n");
        vga_write("Use touch first. write replaces; empty TEXT clears.\n");
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
    } else if(str_equal(command,"page")){
        void *page = pmm_alloc_page();

        if (page == 0) {
            vga_write("page allocation failed\n");
        } else {
            vga_write("page: ");
            vga_write_hex((unsigned int)page);
            vga_write("\n");
        }
    } else if (str_equal(command,"meminfo")){
        vga_write("total pages: ");
        vga_write_hex(pmm_get_total_pages());
        vga_write("\n");

        vga_write("free pages: ");
        vga_write_hex(pmm_get_free_pages());
        vga_write("\n");
    } else if (str_equal(command,"pagetest")) {
        unsigned int before = pmm_get_free_pages();

        void *a = pmm_alloc_page();
        void *b = pmm_alloc_page();

        unsigned int after_alloc = pmm_get_free_pages();

        pmm_free_page(a);
        pmm_free_page(b);

        unsigned int after_free = pmm_get_free_pages();

        vga_write("a: ");
        vga_write_hex((unsigned int)a);
        vga_write("\n");

        vga_write("b: ");
        vga_write_hex((unsigned int)b);
        vga_write("\n");

        vga_write("before    : ");
        vga_write_hex(before);
        vga_write("\n");

        vga_write("after alloc: ");
        vga_write_hex(after_alloc);
        vga_write("\n");

        vga_write("after free : ");
        vga_write_hex(after_free);
        vga_write("\n");
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
    input_overflow = 0;

    vga_write("banana shell\n");
    shell_prompt();
}


void shell_input_char(char c)
{
    if (c == '\n') {
        vga_putc('\n');

        input_buffer[input_length] = '\0';

        if (input_overflow) vga_write("input too long; command discarded\n");
        else shell_execute(input_buffer);

        input_length = 0;
        input_overflow = 0;

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
    } else {
        // 不执行被截断的 write/rm 等命令；回车后重新输入整行。
        input_overflow = 1;
    }
}
