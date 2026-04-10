[bits 32]
[org 0x00400000]

%define SYS_WRITE 1
%define SYS_CLEAR 2

start:
    mov eax, SYS_CLEAR
    int 0x80

    mov eax, SYS_WRITE
    mov ebx, clear_message
    mov ecx, clear_message_len
    int 0x80
    ret

clear_message:
    db "clear.app asked SNSX to clear the console before printing this line.", 10
clear_message_len equ $ - clear_message
