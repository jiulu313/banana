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

    mov esi, message
    call print_string

.halt:
    cli
    hlt
    jmp .halt


print_string:
    mov edi, 0xb8000

.next_char:
    lodsb

    test al, al
    jz .done

    mov byte [edi], al 
    mov byte [edi + 1], 0x0f

    add edi, 2

    jmp .next_char

.done:
    ret


message:
    db "Welcome to banana 32-bit protected mode!", 0
    
        


.halt:
    cli
    hlt
    jmp .halt





times 512 - ($ - $$) db 0