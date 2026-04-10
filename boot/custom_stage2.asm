[org 0x8000]
[bits 16]

%ifndef KERNEL_SECTORS
%define KERNEL_SECTORS 128
%endif

%define KERNEL_LOAD_SEGMENT 0x1000
%define KERNEL_START_LBA 18
%define SECTORS_PER_TRACK 18
%define HEADS 2

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9000
    sti

    mov [boot_drive], dl
    mov bx, 0x0000
    mov ax, KERNEL_LOAD_SEGMENT
    mov es, ax
    mov word [current_lba], KERNEL_START_LBA
    mov cx, KERNEL_SECTORS

.read_loop:
    cmp cx, 0
    je .loaded
    push cx
    push bx
    call read_sector_lba
    pop bx
    pop cx
    jc disk_error
    add bx, 512
    jnc .segment_ok
    mov ax, es
    add ax, 0x20
    mov es, ax
.segment_ok:
    inc word [current_lba]
    dec cx
    jmp .read_loop

.loaded:
    call enable_a20
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp 0x08:protected_mode

read_sector_lba:
    pusha
    mov ax, [current_lba]
    xor dx, dx
    mov cx, SECTORS_PER_TRACK
    div cx
    mov bl, dl
    inc bl
    xor dx, dx
    mov cx, HEADS
    div cx
    mov dh, dl
    mov ch, al
    mov cl, bl
    mov ah, 0x02
    mov al, 0x01
    mov dl, [boot_drive]
    int 0x13
    jc .fail
    clc
    popa
    ret
.fail:
    stc
    popa
    ret

enable_a20:
    in al, 0x92
    or al, 0x02
    out 0x92, al
    ret

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

[bits 32]
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    xor eax, eax
    xor ebx, ebx
    mov ecx, 0x00010000
    jmp ecx

[bits 16]
boot_drive: db 0
current_lba: dw 0
disk_error_message: db "Stage2 kernel load failed", 0

gdt_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
