.global _main
_main:
    stp x29, x30, [sp, -16]!            // store pair fp & lr
    mov x29, sp            // store sp
    sub sp, sp, #16            // allocate 16-byte stack frame

    mov x0, #1            // store the integer in x0
// expression generated
    str x0, [x29, #-8]            // store x0 at fp + -8 into x0
    BeginLoop_1:

        ldr x0, [x29, #-8]            // peak from fp + -8 into x0

        str x0, [x29, #-16]            // store x0 at fp + -16 into x0

        mov x0, #21            // store the integer in x0
        neg x0, x0            // store in x0 negation of x0

        ldr x1, [x29, #-16]            // peak from fp + -16 into x1
        cmp x0, x1            // compare if x0 is 0 and set a flag
        cset x0, eq            // set x0 to the result

        cmp x0, #0            // compare if x0 is 0 and set a flag
        b.eq .Lnext_2            // branch to else if res is 0
        b EndLoop_1

        b .Lend_2            // branch to else after completing if
        .Lnext_2:
        .Lend_2:
// incremented branch count
    BeginLoop_3:

            ldr x0, [x29, #-8]            // peak from fp + -8 into x0

            str x0, [x29, #-16]            // store x0 at fp + -16 into x0

            mov x0, #1            // store the integer in x0

            ldr x1, [x29, #-16]            // peak from fp + -16 into x1

            sub x0, x1, x0            // x0 = x1 - x0
            str x0, [x29, #-8]            // store the new value
            b LoopCondition_3

            mov x0, #100            // store the integer in x0
            neg x0, x0            // store in x0 negation of x0
            str x0, [x29, #-8]            // store the new value
        LoopCondition_3:

        ldr x0, [x29, #-8]            // peak from fp + -8 into x0

        str x0, [x29, #-16]            // store x0 at fp + -16 into x0

        mov x0, #10            // store the integer in x0
        neg x0, x0            // store in x0 negation of x0

        ldr x1, [x29, #-16]            // peak from fp + -16 into x1
        cmp x1, x0            // compare if x0 is 0 and set a flag
        cset x0, ge            // set x0 to the result
        cmp x0, #0
        b.ne BeginLoop_3
    EndLoop_3:

        ldr x0, [x29, #-8]            // peak from fp + -8 into x0

        str x0, [x29, #-16]            // store x0 at fp + -16 into x0

        mov x0, #10            // store the integer in x0

        ldr x1, [x29, #-16]            // peak from fp + -16 into x1

        sub x0, x1, x0            // x0 = x1 - x0
        str x0, [x29, #-8]            // store the new value
    b BeginLoop_1
    EndLoop_1:

    mov x0, #256            // store the integer in x0

    str x0, [x29, #-16]            // store x0 at fp + -16 into x0

    ldr x0, [x29, #-8]            // peak from fp + -8 into x0

    ldr x1, [x29, #-16]            // peak from fp + -16 into x1

    sub x0, x1, x0            // x0 = x1 - x0

    mov sp, x29            // restore sp from fp
    ldp x29, x30, [sp], 16            // restore x29 and x30 from sp
    ret            // end of function
