.global user_start
user_start:

    // change execution level to EL0
    // and return to user program

    //those 2 controll exceptions at el0
    // we care about bits 9-0
    // 9-6 we want to disable only fiq so we set bit 6 to 1, 
    // 5-4 execution state will be aarch64 so we leave 00
    // 3-0 0000 for EL0t
    mov     x2, #0x340 // this is fucking important cause setting it to 3f0 would disable irq, which we try to enable
    msr     spsr_el1, x2
    // set user stack
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
   // svc 0
    cmp x0, 5
loop:
   add x0, x0, 1
   b loop