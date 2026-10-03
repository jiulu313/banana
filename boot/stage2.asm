org 0x1000
bits 16

CODE_SEG equ 0x08
DATA_SEG equ 0x10

KERNEL_SEGMENT equ 0x1000
KERNEL_SECTORS equ 8

start:
    ; 保存启动磁盘号
    mov [boot_drive], dl

    ; ---------------------------------
    ; 把 kernel 加载到物理地址 0x10000
    ; ES:BX = 1000:0000, 1000 * 16 + 0 = 0x10000
    ; ---------------------------------

    mov ax, KERNEL_SEGMENT
    mov es, ax
    xor bx, bx

    ;read disk
    mov ah, 0x02
    mov al, KERNEL_SECTORS      ;读8个扇区

    mov ch, 0x00
    mov dh, 0x00
    mov cl, 0x03                ;

    mov dl, [boot_drive]

    int 0x13

    jc disk_error

    ; ---------------------------------
    ; 准备进入保护模式
    ; ---------------------------------

    cli

    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 0x01
    mov cr0, eax

    jmp CODE_SEG:protected_mode


disk_error:
    mov si, disk_error_msg

.print:
    lodsb
    test al, al
    jz .halt

    mov ah, 0x0e
    int 0x10

    jmp .print

.halt:
    cli
    hlt
    jmp .halt


boot_drive:
    db 0

disk_error_msg:
    db "Kernel disk read error", 0


; --------------------------
; GDT
; --------------------------

gdt_start:

gdt_null:
    dq 0

gdt_code:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

gdt_data:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:


gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start


; --------------------------
; Protected mode
; --------------------------

bits 32

protected_mode:
    mov ax, DATA_SEG

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000

    ; kernel 已经被加载到了 0x10000
    jmp 0x10000


times 512 - ($ - $$) db 0