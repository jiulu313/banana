bits 32

global enter_user_mode

enter_user_mode:
    mov eax, [esp + 4]
    mov ebx, [esp + 8]

    mov cx, 0x23

    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    push dword 0x23
    push ebx

    pushfd
    or dword [esp], 0x200

    push dword 0x1B
    push eax

    iret