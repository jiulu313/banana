#ifndef SYSCALL_H
#define SYSCALL_H


//不是创建结构体，而是把一块已有内存“解释成”某个结构体。
//查看interrupt.asm的isr80:
//    pusha
//    mov eax,esp 
//    push eax 
// 此时栈中的结构和syscall_frame中定义的是对应的
// 此时参数frame，就可以通过指针拿到栈中对应的字段的值了。
// 这是一个常用的内核技巧

struct syscall_frame
{
    unsigned int edi;
    unsigned int esi;
    unsigned int ebp;
    unsigned int esp;
    unsigned int ebx;
    unsigned int edx;
    unsigned int ecx;
    unsigned int eax;
};

void syscall_handler(struct syscall_frame *frame);

#endif