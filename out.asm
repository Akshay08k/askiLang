global _start
_start:
    mov rax,17
    push rax
    mov rax,62
    push rax
    mov rax,3
    push rax
    pop rax
    pop rbx
    add rax, rbx
    push rax
    push QWORD [rsp + 0]

    mov rax, 60
    pop rdi
    syscall
    mov rax, 60
    mov rdi, 0
    syscall
