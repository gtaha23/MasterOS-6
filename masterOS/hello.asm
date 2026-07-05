bits 32


mov eax, 0xC00B8000    
mov byte [eax], 'H'
mov byte [eax+1], 0x0A  

ret
