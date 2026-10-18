/* 主机测试：gcc -std=c11 -Wall -Wextra -Werror tests/fs_test.c -o fs-test */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define IRQ_H
static unsigned int flags = 0x202;
static unsigned int irq_save(void) { unsigned int saved = flags; flags &= ~0x200u; return saved; }
static void irq_restore(unsigned int saved) { assert(!(flags & 0x200)); flags = saved; }
#include "../kernel/fs.c"

int main(void)
{
    fs_init();
    struct fs_info info;
    char buffer[FS_FILE_CAPACITY + 1];
    assert(fs_list(0, &info) == 0);
    assert(fs_list(FS_MAX_FILES, &info) == FS_ERR_INVALID);
    assert(fs_list(0, 0) == FS_ERR_INVALID);
    const char *bad[] = {0, "", "/", ".", "..", "/..", "a/b", "//a", "a b", "a\\b",
        "abcdefghijklmnopqrstuvwxyz123456"};
    for (unsigned int i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        assert(fs_create(bad[i]) == FS_ERR_INVALID);
    }
    assert(fs_create("abcdefghijklmnopqrstuvwxyz12345") == 0); // 31 字符
    assert(fs_create("note") == 0);
    assert(fs_create("/note") == FS_ERR_EXISTS);
    assert(fs_write("missing", "x", 1) == FS_ERR_NOT_FOUND);
    assert(fs_read("note", 0, buffer, sizeof(buffer)) == 0);
    const char binary[] = {'A', '\0', 'B'};
    assert(fs_write("/note", binary, sizeof(binary)) == 3);
    assert(fs_append("note", "xy", 2) == 2);
    assert(fs_read("note", 1, buffer, 9) == 4);
    assert(memcmp(buffer, "\0Bxy", 4) == 0);
    assert(fs_read("note", 5, buffer, 1) == 0);
    assert(fs_read("note", ~0u, buffer, 1) == 0);
    assert(fs_read("note", 0, 0, 1) == FS_ERR_INVALID);
    assert(fs_read("note", 0, 0, 0) == 0);
    assert(fs_write("note", 0, 1) == FS_ERR_INVALID);
    assert(fs_write("note", "q", 1) == 1); // 整体覆盖后截短
    assert(fs_read("note", 0, buffer, 8) == 1 && buffer[0] == 'q');
    memset(buffer, 'Z', sizeof(buffer));
    assert(fs_write("note", buffer, FS_FILE_CAPACITY) == FS_FILE_CAPACITY);
    assert(fs_append("note", "!", 1) == FS_ERR_TOO_LARGE);
    assert(fs_write("note", buffer, ~0u) == FS_ERR_TOO_LARGE);
    assert(fs_append("note", buffer, ~0u) == FS_ERR_TOO_LARGE);
    assert(fs_read("note", 0, buffer, sizeof(buffer)) == FS_FILE_CAPACITY);
    for (int i = 0; i < FS_FILE_CAPACITY; i++) assert(buffer[i] == 'Z');
    assert(fs_write("note", 0, 0) == 0);
    assert(fs_read("note", 0, buffer, 1) == 0);
    assert(fs_append("note", 0, 0) == 0);
    assert(fs_remove("note") == 0);
    assert(fs_remove("note") == FS_ERR_NOT_FOUND);
    assert(fs_create("fresh") == 0);
    assert(fs_read("fresh", 0, buffer, sizeof(buffer)) == 0); // 无旧数据泄露
    for (int i = 2; i < FS_MAX_FILES; i++) {
        char name[16];
        snprintf(name, sizeof(name), "file%d", i);
        assert(fs_create(name) == 0);
    }
    assert(fs_create("overflow") == FS_ERR_FULL);
    assert(fs_remove("fresh") == 0 && fs_create("reused") == 0);
    int count = 0;
    for (unsigned int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_list(i, &info) == 1) {
            count++;
            assert(info.size == 0 && info.name[0] != '\0');
        }
    }
    assert(count == FS_MAX_FILES && flags == 0x202);
    flags = 2; // 键盘 IRQ 中调用时必须保持关中断
    assert(fs_remove("reused") == 0 && flags == 2);
    fs_init();
    for (unsigned int i = 0; i < FS_MAX_FILES; i++) assert(fs_list(i, &info) == 0);
    assert(flags == 2);
    puts("RAMFS tests passed");
    return 0;
}
