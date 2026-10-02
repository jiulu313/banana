org 0x7c00
bits 16

start:
    cli

    ; 初始化数据段, ax = 0
    xor ax, ax
    mov ds, ax
    mov es, ax 
    mov ss, ax

    mov sp,0x7c00

    sti


    ; 保存BIOS给我们的启动磁盘号
    ; 因为 BIOS 在把 boot sector 加载并跳到我们的代码时
    ; 会把“当前是从哪个磁盘启动的”这个磁盘编号放进 DL 寄存器
    ; 所以当BIOS执行start时，DL里面已经有一个很重要的信息
    ; 我是从哪块磁盘启动的
    mov [boot_drive], dl


    ; 把第二个扇区读到内存 0x1000 
    ; 是为了告诉BIOS，你从磁盘读出来的数据，放在内存的哪里？
    ; 因为BIOS的 int 0x13 读磁盘时，会把数据写到  ES:BX 中
    ; 又因为开头已经把 es 赋值为0了。
    ; 所以根据公式 ex * 16 +_bx得出的地址，就是0x1000
    ; 实际效果就是：把第2个扇区读出来数据，放在物理内存0x1000开始的位置
    mov bx, 0x1000


    ;读取磁盘
    mov ah, 0x02    ;BIOS读取磁盘的功能
    mov al, 0x01    ;读取1个扇区

    mov ch, 0x00    ;柱面 ＝ 0
    mov dh, 0x00    ;磁头 = 0
    mov cl, 0x02    ;扇区 ＝ 2，第2个扇区，从1开始

    mov dl, [boot_drive]    

    int 0x13        ;是BIOS的读取磁盘功能

    ;如果磁盘读取失败，跳到 disk_error
    jc disk_error

    ;跳到刚刚加载的stage2
    jmp 0x0000:0x1000



    disk_error:
        mov si, err_msg



    .print_error:
        lodsb                   ;从DS：SI指向的内存位置读取1个字节，放在AL里，然后SI自动加1
        test al, al             ;测试AL是否为0，类似 AL AND AL，如何结果是0，CPU标识位 ZF＝1  
        jz .halt                ;jz就是 ZF＝1时，跳转。在这里就是AL为0时，则ZF＝1，则跳转到 .halt
        
        mov ah, 0x0e            ;把AL中的字符显示到屏幕上
        int 0x10                ;然后回到循环继续取下一个字符
        jmp .print_error        ;这一句是调用 INT 0x10H的0x0E号功能


    .halt:
        cli
        hlt 
        jmp .halt 


    ;定义一个字节，用的时候 [boot_drive]表示，相当于定义了一个变量
    boot_drive:
        db 0

    err_msg:
        db "Disk read error",0


    times 510 - ($ - $$) db 0 
    dw 0xaa55                 

































