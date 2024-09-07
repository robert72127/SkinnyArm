#include "memory_map.h"

.section ".text.boot"

.global _start

_start:

    // clear bss
    ldr     x1, =__bss_start
    ldr     x2, =__bss_size
clear_bss:
    cbz     x2, start_cores
    str     xzr, [x1], #16
    sub     x2, x2, #16
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
    adr     x2, enable_vmem
    msr     elr_el2, x2
    eret

enable_vmem:
    // write configurations for vmem
    ldr x0, = TCR_CONFIG_DEFAULT
    msr tcr_el1, x0
    // write memory access policies configurations to mair_el1
    ldr x0, =( \
        (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | \
        (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)) \
        )
    msr mair_el1, x0

    mov x0, 0 // PGD's page frame at 0x0
    mov x1, 0x1000 // PUD's page frame at 0x1000

    ldr x2, = BOOT_PGD_ATTR
    orr x2, x1, x2 // combine the physical address of next level page with attribute.
    str x2, [x0]

    ldr x2, = BOOT_PUD_ATTR
    mov x3, 0x00000000
    orr x3, x2, x3
    str x3, [x1] // 1st 1GB mapped by the 1st entry of PUD
    mov x3, 0x40000000
    orr x3, x2, x3
    str x3, [x1, 8] // 2nd 1GB mapped by the 2nd entry of PUD

    msr ttbr0_el1, x0 // load PGD to the bottom translation-based register.

    mrs x2, sctlr_el1
    orr x2 , x2, 1
    msr sctlr_el1, x2 // enable MMU, cache remains disabled



// our stack grown downward
// therefore for each core we should set initial address to:
// core_nr + 4095 or
// (core_nr+1)*4096 - 1
set_stack:
     mrs     x0, CurrentEL
    and     x0, x0, #12 // clear reserved bits
    # get cpu_id
    mrs     x0, mpidr_el1
    and     x0, x0, #3
    add     x0, x0, 1
    // multipy x0 by 4096
    lsl     x0, x0, #12
    ldr     x1, =kernel_stack
    add     x1, x1, x0
    sub     x1, x1, 1
    // set stack for each of the cores
    // at kernel_stack + 4096 * cpu_id
    mov     sp, x1

    // jump to C code, should not return
jump_main:
    bl      main
    // for failsafe, halt
loop_forever:
    b       loop_forever
