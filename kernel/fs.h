#ifndef FS_H
#define FS_H

// 第一阶段 RAMFS：数据存在内存中，重启丢失；只有根目录，没有子目录。
#define FS_MAX_FILES 16
#define FS_NAME_MAX 31
#define FS_FILE_CAPACITY 1024

#define FS_ERR_INVALID  (-1)
#define FS_ERR_NOT_FOUND (-2)
#define FS_ERR_EXISTS   (-3)
#define FS_ERR_FULL     (-4)
#define FS_ERR_TOO_LARGE (-5)

struct fs_info {
    char name[FS_NAME_MAX + 1];
    unsigned int size;
};

// 启动时调用一次，清空文件表。之后各接口在单核上用关中断保护，不阻塞。
void fs_init(void);
// 文件名支持字母、数字、点、下划线、减号，可带一个前导 /；不接受 . 和 ..。
// 创建空文件/删除文件：成功返回 0，失败返回负错误码。
int fs_create(const char *name);
int fs_remove(const char *name);
// 整体覆盖/追加已存在文件，成功返回写入字节数；失败不修改原内容。
// length 为 0 时允许 data 为 NULL；覆盖写 0 字节会清空文件。
int fs_write(const char *name, const void *data, unsigned int length);
int fs_append(const char *name, const void *data, unsigned int length);
// 按偏移读原始字节，不补字符串结束符；返回字节数，EOF 返回 0，错误返回负数。
// 非零 length 要求 buffer 有足够空间；所有指针必须指向有效内核内存。
int fs_read(const char *name, unsigned int offset, void *buffer, unsigned int length);
// 按槽位枚举 [0, FS_MAX_FILES)：有效文件返回 1，空槽返回 0，错误返回负数。
// 输出元数据副本，不暴露内部指针；多次枚举调用不保证同一时刻的快照。
int fs_list(unsigned int slot, struct fs_info *info);

#endif
