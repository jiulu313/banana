#ifndef USERCOPY_H
#define USERCOPY_H

//把用户数据复制到内核自己的缓冲区
// 用户 -> 内核 
int copy_from_user(
    void *kernel_dest,
    const void *user_src,
    unsigned int size
);


//字符串专用版本
int copy_string_from_user(
    char *kernel_dest,
    const char *user_src,
    unsigned int max_length
);

//把内核数据copy到用户空间
// 内核 -> 用户 
int copy_to_user(
    void *user_dest,
    const void *kernel_src,
    unsigned int size
);

#endif