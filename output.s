.global _main
_main:
    stp x29, x30, [sp, -16]!            // store pair fp & lr
    mov x29, sp            // store sp
    sub sp, sp, #16            // allocate 16-byte stack frame

    mov x0, #2            // store the integer in x0

    str x0, [x29, #-8]            // store x0 at fp + -8 into x0

        ldr x0, [x29, #-8]            // peak from fp + -8 into x0

        str x0, [x29, #-16]            // store x0 at fp + -16 into x0

        mov x0, #2            // store the integer in x0

        ldr x1, [x29, #-16]            // peak from fp + -16 into x1

        add x0, x1, x0            // x0 = x1 + x0
        str x0, [x29, #0]            // store the new value

        ldr x0, [x29, #0]            // peak from fp + 0 into x0

        mov sp, x29            // restore sp from fp
        ldp x29, x30, [sp], 16            // restore x29 and x30 from sp
        ret            // end of function

    ldr x0, [x29, #-8]            // peak from fp + -8 into x0

    mov sp, x29            // restore sp from fp
    ldp x29, x30, [sp], 16            // restore x29 and x30 from sp
    ret            // end of function
