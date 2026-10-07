#include "syscall.h"
#include "vga.h"
#include "paging.h"
#include "usercopy.h"

#define SYS_WRITE 1

#define SYSCALL_STRING_MAX 256

static unsigned int str_length(const char *str) {

    unsigned int length = 0;
    while (str[length] != '\0') {
        length++;
    }

    return length;
}

//如果字符串很长，跨到了下一页，而下一页没有映射
//所以，暂时先限制字符串最多4096个字节。
static int user_string_valid(const char *str)
{
    for (unsigned int i = 0; i < 4096; i++) {

        unsigned int addr =
            (unsigned int)&str[i];

        if (!is_user_address_mapped(addr)) {
            return 0;
        }

        if (str[i] == '\0') {
            return 1;
        }
    }

    return 0;
}

void syscall_handler(struct syscall_frame *frame)
{
    unsigned int syscall_number = frame->eax;

    switch (syscall_number) {
        case SYS_WRITE:{
            const char *user_str = (const char *)frame->ebx;

            //syscall_handler这个函数是在内核中运行
            //所以直接定义的kernel_buffer数组，就是内核中的数组
            char kernel_buffer[SYSCALL_STRING_MAX];

            if (!copy_string_from_user(
                    kernel_buffer,
                    user_str,
                    sizeof(kernel_buffer)))
            {
                frame->eax = 0xFFFFFFFF;
                break;
            }

            //vga_write函数，接收到的地址
            //不再是用户空间的地址了
            //而是内核空间的地址
            //更安全，更清晰
            vga_write(kernel_buffer);

            unsigned int length = 0;

            while (kernel_buffer[length] != '\0') {
                length++;
            }
            
            frame->eax = length;

            break;
        } 
        default: {
            /*
             * 这里暂时用 0xFFFFFFFF 表示 syscall 不存在
             */
            frame->eax = 0xFFFFFFFF;
            break;
        }

    }
}