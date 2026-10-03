bits 32

global _start       ;把 _start 这个符号导出给链接器
extern kernel_main  ;kernel_main 不在这个 asm 文件里，它在别的目标文件里,需要链接器链接



section .text 

_start:
    call kernel_main

.halt: 
    cli 
    hlt 
    jmp .halt    












