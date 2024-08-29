.global user_start
user_start:

    // change execution level to EL0
    // and return to user program
    adr     x2, user_program
    msr     elr_el1, x2
    ldr     x1, =user_stack
    msr     sp_el0, x1
    eret

// example program that will cause exception using svc
.global user_program
user_program:
    mov x0, 0
dummy:
    add x0, x0, 1
    svc 0
    cmp x0, 5
loop:
   add x0, x0, 1
   b loop