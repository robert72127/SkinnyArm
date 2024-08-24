.global user_start
user_start:

    // change execution level to EL0
    // and return to user program
    mov     x2, #0x3c0
    msr     spsr_el1, x2
    // set user stack
    adr     x2, user_program
    
    msr     elr_el1, x2
    eret

// example program that will cause exception using svc
.global user_program
user_program:
    ldr     x1, =user_stack
    mov     sp, x1
    mov x0, 0
1:
    add x0, x0, 1
    svc 0
    cmp x0, 5
    blt 1b
1:
