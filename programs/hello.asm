[bits 32]
[org 0x00400000]

%define SYS_WRITE 1

start:
    mov eax, SYS_WRITE
    mov ebx, hello_message
    mov ecx, hello_message_len
    int 0x80

    mov eax, SYS_WRITE
    mov ebx, done_message
    mov ecx, done_message_len
    int 0x80
    ret

hello_message:
    db "Hello from SNSX. hello.app is running through the kernel syscall interface.", 10
hello_message_len equ $ - hello_message

done_message:
    db "Execution returned cleanly to the SNSX shell.", 10
done_message_len equ $ - done_message
