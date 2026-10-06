#include "syscall.h"
#include "vga.h"

#define SYS_WRITE 1

void syscall_handler(struct syscall_frame *frame)
{
    unsigned int syscall_number = frame->eax;
    switch (syscall_number)
    {
        case SYS_WRITE:
        {
            const char *str = (const char *)frame->ebx;

            vga_write(str);

            break;
        }

        default:
            vga_write("unknown syscall\n");
            break;
    }
}