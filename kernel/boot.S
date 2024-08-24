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
    ldr x0, =set_el1
    
    // start core 1 0xE0
    ldr x1, =0xE0
    str x0, [x1]

    // start core 2 0xE8
    ldr x1, =0xE8
    str x0, [x1]

    // start core 3
    ldr x1, =0xF0
    str x0, [x1] 

// we start in EL2
set_el1:
    // enable AArch64 in EL1
    mov     x0, #(1 << 31)      // AArch64
    msr     hcr_el2, x0
    // Setup SCTLR access
    // disable instrucion cache
    // disable MMu
    //mov     x2, #0x0800
    //movk    x2, #0x30d0, lsl #16
    // msr     sctlr_el1, x2
    // change execution level to EL1, return to set_stack
    mov     x2, #0x3c5
    msr     spsr_el2, x2
    adr     x2, set_stack
    msr     elr_el2, x2
    eret

set_stack:
     mrs     x0, CurrentEL
    and     x0, x0, #12 // clear reserved bits
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
