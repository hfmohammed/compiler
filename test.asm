global _start
_start:
    mov rax, 1
    push rax
    push QWORD [rsp + 0]
    pop rax
    test rax, rax
    jz label0
    mov rax, 254
    push rax
    push QWORD [rsp + 0]
    mov rax, 60
    pop rdi
    syscall
    sub rsp, 8
label0:
    push QWORD [rsp + 0]
    pop rax
    test rax, rax
    jz label2
    mov rax, 68
    push rax
    mov rax, 60
    pop rdi
    syscall
    sub rsp, 0
    jmp label1
label2:
    mov rax, 67
    push rax
    mov rax, 60
    pop rdi
    syscall
    sub rsp, 0
label1:
    push QWORD [rsp + 0]
    mov rax, 60
    pop rdi
    syscall
    mov rax, 60
    mov rdi, 0
    syscall
