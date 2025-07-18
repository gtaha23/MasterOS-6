bits 32

section .multiboot
align 4
dd 0x1BADB002  ; Multiboot Magic Number
dd 0x00000000  ; Flags (0: minimum özellikler)
dd -(0x1BADB002 + 0x00000000) ; Checksum (bu üç değerin toplamı 0 olmalı)

section .text
global start
extern kmain

start:
    cli
    mov esp, stack_space
    push ebx
    push eax
    call kmain
    hlt

HaltKernel:
    cli
    hlt
    jmp HaltKernel

section .bss
align 16
resb 8192
stack_space:
