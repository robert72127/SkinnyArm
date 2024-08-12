.section ".text.boot"

.global _start

_start:
    // clear bss
    ldr     x1, =__bss_start
    ldr     x2, =__bss_size
clear_bss:
    cbz     x2, start_cores
    str     xzr, [x1], #8
    sub     x2, x2, #8
    cbnz    x2, clear_bss

    // other cores are stopped by firmware,
    // start them from core0 by writing address of _start 
    // to address of each core
start_cores: 
    ldr x0, =set_stack
    
    // start core 1 0xE0
    ldr x1, =0xE0
    str x0, [x1]

    // start core 2 0xE8
    ldr x1, =0xE8
    str x0, [x1]

    // start core 3
    ldr x1, =0xF0
    str x0, [x1] 

    b set_stack

set_stack:
    # get cpu_id
    mrs     x0, mpidr_el1
    and     x0, x0, #3
    // multipy x0 by 4096
    lsl     x0, x0, #12
    ldr     x1, =kernel_stack
    add     x1, x1, x0
    // set stack for each of the cores
    // at kernel_stack + 4096 * cpu_id
    mov     sp, x1

    // jump to C code, should not return
jump_main:
    bl      main
    // for failsafe, halt
loop_forever:
    b       loop_forever