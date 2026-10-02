org 0x1000
bits 16

CODE_SEG equ 0x08
DATA_SEG equ 0x10

start:
    cli

    ; 加载 GDT
    lgdt [gdt_descriptor]

    ; 打开 CR0 的 PE 位
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; 远跳转，刷新 CS，正式进入保护模式
    jmp CODE_SEG:protected_mode


; --------------------------
; GDT
; --------------------------

gdt_start:

gdt_null:
    dq 0x0000000000000000

gdt_code:
    dw 0xffff       ; limit 0:15
    dw 0x0000       ; base 0:15
    db 0x00         ; base 16:23
    db 10011010b    ; access
    db 11001111b    ; flags + limit 16:19
    db 0x00         ; base 24:31

gdt_data:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:


gdt_descriptor:
    dw gdt_end - gdt_start - 1          ;总共3个gdt entry,每个是8字节，所以一共24个字节，范围就是0～23，所以要减1
    dd gdt_start                        ;dd定义4个字节，保存GDT的位置


; --------------------------
; 32-bit protected mode

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

    ; 直接写 VGA 文本显存，直接在0xb8000位置写，就可以直接操作显存，就可以显示出来
    ; 屏幕上每个显示的字符，需要2个字节，第1个是ASCII码，第2个是颜色属性
    ; 0X0F，可以分为高4位，低4位。其中高4位表示背景色，低4位表示前景色
    ; 0x0F,代表背景是黑色，前景是白色，也就是黑底白字
    mov byte [0xb8000], 'B'
    mov byte [0xb8001], 0x0f

    mov byte [0xb8002], 'A'
    mov byte [0xb8003], 0x0f

    mov byte [0xb8004], 'N'
    mov byte [0xb8005], 0x0f

    mov byte [0xb8006], 'A'
    mov byte [0xb8007], 0x0ff

    mov byte [0xb8008], 'N'
    mov byte [0xb8009], 0x0f

    mov byte [0xb800a], 'A'
    mov byte [0xb800b], 0x0f


.halt:
    cli
    hlt
    jmp .halt


times 512 - ($ - $$) db 0