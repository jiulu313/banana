bits 32 

global idt_load
global isr80


extern interrupt_handler



;加载idt的函数
;C语言这样调用: idt_load(&idt_ptr);
;参数会通过栈传进来
;32位x86常见C调用约定如下：
;[esp]      返回地址 
;[esp+4]    第一参数

idt_load:
    mov eax,[esp + 4]
    lidt [eax]              ;告诉CPU,IDT的地址和大小在这里
    ret 


;真正的中断入口函数
isr80:
    pusha
    call interrupt_handler
    popa
    iret    