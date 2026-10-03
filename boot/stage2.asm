org 0x1000
bits 16


;检测内存用的
E820_COUNT       equ 0x4FF0
E820_BUFFER      equ 0x5000 ;写入的内存地址相关 
E820_ENTRY_SIZE  equ 24
E820_MAX_ENTRIES equ 128


;GDT相关
CODE_SEG equ 0x08
DATA_SEG equ 0x10

KERNEL_SEGMENT equ 0x1000
KERNEL_SECTORS equ 32

start:

    ; 确保数据段为 0
    xor ax, ax
    mov ds, ax
    mov es, ax


    ; 保存启动磁盘号
    mov [boot_drive], dl


    ; 获取 BIOS 内存地图
    call detect_memory

    ; ---------------------------------
    ; 把 kernel 加载到物理地址 0x10000
    ; ES:BX = 1000:0000, 1000 * 16 + 0 = 0x10000
    ; ---------------------------------

    mov ax, KERNEL_SEGMENT
    mov es, ax
    xor bx, bx

    ;read disk
    mov ah, 0x02
    mov al, KERNEL_SECTORS      ;读32个扇区

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


;检测内存
detect_memory:
    ; entry 数量先清零
    mov dword [E820_COUNT], 0

    ; BIOS 第一次调用要求 EBX = 0
    xor ebx, ebx

    ; BIOS 把 entry 写到 ES:DI
    xor ax, ax
    mov es, ax

    mov di, E820_BUFFER ;ES=0,DI=0x=0x5000,所以ES*16+DI = 0x5000,BIOS的第一条内存信息写入到这里

;循环向 BIOS 要内存地图”的过程:
;
;准备缓冲区
;   ↓
;向 BIOS 请求一条 E820 entry
;   ↓
;BIOS 写到 ES:DI
;   ↓
;保存成功
;   ↓
;DI += 24
;   ↓
;EBX 是否为 0？
; ├─ 否 → 继续取下一条
; └─ 是 → 结束
.next_entry:
    ; 最多保存 128 项，防止把后面的内存覆盖掉
    cmp dword [E820_COUNT], E820_MAX_ENTRIES
    jae .done ; jae 是 >= ，这里是 >=128，如果当前数量>=128,就结束

    ; E820 功能号
    mov eax, 0xE820

    ; "SMAP"
    mov edx, 0x534D4150

    ; 每个 entry 最大 24 字节
    mov ecx, E820_ENTRY_SIZE

    ; ACPI 3.x 扩展属性，先设 bit0
    mov dword [es:di + 20], 1

    int 0x15

    ; CF = 1 表示 BIOS 调用失败/结束
    jc .done

    ; BIOS 成功时应该返回 "SMAP"，SMAP是E820规定
    cmp eax, 0x534D4150
    jne .done

    ; 至少应该返回经典的 20 字节
    cmp ecx, 20
    jb .done

    ; 成功得到一个 entry，[E820_COUNT]自增1
    inc dword [E820_COUNT]

    ; DI 指向下一个 entry
    add di, E820_ENTRY_SIZE

    ; EBX != 0 表示还有下一项
    test ebx, ebx
    jnz .next_entry

.done:
    ret











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