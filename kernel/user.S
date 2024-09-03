.global user_start
user_start: //user_program, stack_address
    // change execution level to EL0
    // and return to user program
    msr     elr_el1, x0
    msr     sp_el0, x1
    eret

// example program that will cause exception using svc
.global user_program
user_program:
    mov x0, 0
dummy:
    add x0, x0, 1
    //svc 0
    cmp x0, 5
    // can't do that since we are in el0 need to call syscall
    bl user_fork
loop:
   add x0, x0, 1
   b loop