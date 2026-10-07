#include "syscall.h"
#include "vga.h"
#include "paging.h"

#define SYS_WRITE 1



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
            const char *str = (const char *)frame->ebx;

            //检查字符串是否超过4096个字节
            if (user_string_valid(str)) {
                frame->eax = 0xFFFFFFFF;
                break;
            }
            

            //如果地址没有映射
            if (!is_user_address_mapped((unsigned int)str)) {
                frame->eax = 0xFFFFFFFF;
                break;   
            }
            

            vga_write(str);


            /** 
             * 返回值放进 saved EAX
             */
            frame->eax = str_length(str);

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