global _start
_start:
    mov rax, 2
    push rax
    mov rax, 1
    push rax
    push QWORD [rsp + 8]
    pop rax
    pop rbx
    sub rax, rbx
    push rax
    pop rax
    test rax, rax
    jz label0
    mov rax, 255
    push rax
    push QWORD [rsp + 0]
    mov rax, 60
    pop rdi
    syscall
    sub rsp, 8
label0:
    push QWORD [rsp + 0]
    mov rax, 60
    pop rdi
    syscall
    mov rax, 60
    mov rdi, 0
    syscall
