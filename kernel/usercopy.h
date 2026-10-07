#ifndef USERCOPY_H
#define USERCOPY_H

//把用户数据复制到内核自己的缓冲区
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

#endif