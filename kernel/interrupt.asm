bits 32

global idt_load

;timer中断处理
global irq0
extern timer_handler

;键盘中断处理
global irq1
extern keyboard_handler

extern exception_handler
extern syscall_handler


idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret


; --------------------------------
; 无 CPU error code 的异常
; --------------------------------
%macro ISR_NOERR 1
global isr%1

isr%1:
    push dword 0
    push dword %1
    jmp isr_common
%endmacro


; --------------------------------
; CPU 自动压 error code 的异常
; --------------------------------
%macro ISR_ERR 1
global isr%1

isr%1:
    push dword %1
    jmp isr_common
%endmacro


; CPU exceptions 0~31

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7

ISR_ERR   8

ISR_NOERR 9

ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14

ISR_NOERR 15
ISR_NOERR 16

ISR_ERR   17

ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21

ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29

ISR_ERR   30

ISR_NOERR 31


isr_common:
    pusha

    mov eax, esp
    push eax

    call exception_handler

    add esp, 4

    popa

    add esp, 8

    iret


; 软件中断测试
global isr80

isr80:
    pusha
    mov eax,esp 
    push eax 

    call syscall_handler
    add esp, 4

    popa
    iret


irq0:
    pusha
    call timer_handler
    popa
    iret     


irq1:
    pusha
    call keyboard_handler
    popa
    iret     