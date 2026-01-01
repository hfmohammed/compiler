
.global g
g:
    stp x29, x30, [sp, -16]!            // save fp/lr
    mov x29, sp            // set fp

        mov x0, #0            // store the integer in x0

        cmp x0, #0            // compare if x0 is 0 and set a flag
        b.eq .Lnext_1            // branch to else if res is 0

            mov x0, #3            // store the integer in x0

            mov sp, x29            // restore sp from fp
            ldp x29, x30, [sp], 16            // restore x29 and x30 from sp
            ret            // end of function

        b .Lend_1            // branch to else after completing if
        .Lnext_1:


            mov x0, #8            // store the integer in x0

            mov sp, x29            // restore sp from fp
            ldp x29, x30, [sp], 16            // restore x29 and x30 from sp
            ret            // end of function

        .Lend_1:
// incremented branch count

.global main
main:
    stp x29, x30, [sp, -16]!            // save fp/lr
    mov x29, sp            // set fp
    sub sp, sp, #16            // alloc frame
        bl g            // call g
// expression generated
        str x0, [x29, #-8]            // store x0 at fp + -8 into x0

        ldr x0, [x29, #-8]            // peak from fp + -8 into x0

        mov sp, x29            // restore sp from fp
        ldp x29, x30, [sp], 16            // restore x29 and x30 from sp
        ret            // end of function
.global _main
_main:
    stp x29, x30, [sp, -16]!            // save fp/lr
    mov x29, sp            // set fp
    bl main            // call main

    mov sp, x29            // restore sp from fp
    ldp x29, x30, [sp], 16            // restore x29 and x30 from sp
    ret            // end of function
    mov sp, x29            // restore sp
    ldp x29, x30, [sp], 16            // restore fp/lr
    ret            // return
