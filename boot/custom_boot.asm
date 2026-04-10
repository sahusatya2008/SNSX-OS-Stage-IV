[org 0x7C00]
[bits 16]

%define STAGE2_SECTORS 17

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl
    mov bx, 0x8000
    mov ah, 0x02
    mov al, STAGE2_SECTORS
    mov ch, 0x00
    mov cl, 0x02
    mov dh, 0x00
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    jmp 0x0000:0x8000

disk_error:
    mov si, disk_error_message
.print:
    lodsb
    test al, al
    jz .hang
    mov ah, 0x0E
    int 0x10
    jmp .print
.hang:
    cli
    hlt
    jmp .hang

boot_drive: db 0
disk_error_message: db "Stage1 disk read failed", 0

times 510 - ($ - $$) db 0
dw 0xAA55
