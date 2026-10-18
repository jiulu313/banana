#include "fs.h"
#include "irq.h"

// 文件表保存名字和大小，每个文件有独立的数据区；删除后槽位可直接复用。
struct ram_file {
    int used;
    struct fs_info info;
    unsigned char data[FS_FILE_CAPACITY];
};
static struct ram_file files[FS_MAX_FILES];

static const char *valid_name(const char *name)
{
    if (name == 0) return 0;
    if (*name == '/') name++;
    unsigned int n = 0;
    while (name[n] != '\0') {
        char c = name[n];
        if (n == FS_NAME_MAX || !((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '.' || c == '_' || c == '-')) return 0;
        n++;
    }
    if (n == 0 || (n == 1 && name[0] == '.') ||
        (n == 2 && name[0] == '.' && name[1] == '.')) return 0;
    return name;
}

// 调用者持有关中断临界区，因此查找之后的文件不会被其他任务删除或替换。
static int find_file(const char *name)
{
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!files[i].used) continue;
        unsigned int n = 0;
        while (name[n] != '\0' && name[n] == files[i].info.name[n]) n++;
        if (name[n] == files[i].info.name[n]) return i;
    }
    return FS_ERR_NOT_FOUND;
}

void fs_init(void)
{
    unsigned int flags = irq_save();
    for (int i = 0; i < FS_MAX_FILES; i++) {
        files[i].used = 0;
        files[i].info.size = 0;
        files[i].info.name[0] = '\0';
    }
    irq_restore(flags);
}

int fs_create(const char *name)
{
    name = valid_name(name);
    if (name == 0) return FS_ERR_INVALID;
    unsigned int flags = irq_save();
    int result = FS_ERR_FULL;
    if (find_file(name) >= 0) {
        result = FS_ERR_EXISTS;
    } else {
        for (int i = 0; i < FS_MAX_FILES; i++) {
            if (files[i].used) continue;
            unsigned int n = 0;
            do {
                files[i].info.name[n] = name[n];
            } while (name[n++] != '\0');
            files[i].info.size = 0;
            files[i].used = 1;
            result = 0;
            break;
        }
    }
    irq_restore(flags);
    return result;
}

int fs_remove(const char *name)
{
    name = valid_name(name);
    if (name == 0) return FS_ERR_INVALID;
    unsigned int flags = irq_save();
    int slot = find_file(name);
    if (slot >= 0) {
        files[slot].used = 0;
        files[slot].info.size = 0;
    }
    irq_restore(flags);
    return slot < 0 ? slot : 0;
}

static int write_file(const char *name, const void *data, unsigned int length, int append)
{
    name = valid_name(name);
    if (name == 0 || (length != 0 && data == 0)) return FS_ERR_INVALID;
    unsigned int flags = irq_save();
    int slot = find_file(name);
    int result = slot;
    if (slot >= 0) {
        struct ram_file *file = &files[slot];
        unsigned int offset = append ? file->info.size : 0;
        // 先减再比较，避免 offset + length 溢出；失败时完全保留原文件。
        if (length > FS_FILE_CAPACITY - offset) {
            result = FS_ERR_TOO_LARGE;
        } else {
            const unsigned char *src = data;
            for (unsigned int i = 0; i < length; i++) file->data[offset + i] = src[i];
            file->info.size = offset + length;
            result = (int)length;
        }
    }
    irq_restore(flags);
    return result;
}

int fs_write(const char *name, const void *data, unsigned int length)
{
    return write_file(name, data, length, 0);
}

int fs_append(const char *name, const void *data, unsigned int length)
{
    return write_file(name, data, length, 1);
}

int fs_read(const char *name, unsigned int offset, void *buffer, unsigned int length)
{
    name = valid_name(name);
    if (name == 0 || (length != 0 && buffer == 0)) return FS_ERR_INVALID;
    unsigned int flags = irq_save();
    int slot = find_file(name);
    int result = slot;
    if (slot >= 0) {
        struct ram_file *file = &files[slot];
        unsigned int count = offset < file->info.size ? file->info.size - offset : 0;
        if (count > length) count = length;
        unsigned char *dest = buffer;
        for (unsigned int i = 0; i < count; i++) dest[i] = file->data[offset + i];
        result = (int)count;
    }
    irq_restore(flags);
    return result;
}

int fs_list(unsigned int slot, struct fs_info *info)
{
    if (slot >= FS_MAX_FILES || info == 0) return FS_ERR_INVALID;
    unsigned int flags = irq_save();
    int result = files[slot].used;
    if (result) *info = files[slot].info;
    irq_restore(flags);
    return result;
}
