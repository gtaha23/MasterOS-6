bits 32
org 0x00300000
mov eax, 0x000B8000
mov byte [eax], 'H'
mov byte [eax+1], 0x0A

ret