.global uservec
uservec:
    // allocate space on kernel stack for two user registers
    sub sp, sp, #16
    // save user x0, x1 on kernel stack for now
    stp x0, x1, [sp] 

    // use x0 and x1 to load user stackframe

    // get current process
    mrs x0, tpidr_el1
    // get this process trap frame address into x1
    ldr x1, [x0, 14*8]

    // store rest of registers there
    stp x2,   x3, [x1, 16 * 1]
    stp x4,   x5, [x1, 16 * 2]
    stp x6,   x7, [x1, 16 * 3]
    stp x8,   x9, [x1, 16 * 4]
    stp x10, x11, [x1, 16 * 5]
    stp x12, x13, [x1, 16 * 6]
    stp x14, x15, [x1, 16 * 7]
    stp x16, x17, [x1, 16 * 8]
    stp x18, x19, [x1, 16 * 9]
    stp x20, x21, [x1, 16 * 9]
    stp x22, x23, [x1, 16 * 10]
    stp x24, x25, [x1, 16 * 11]
    stp x26, x27, [x1, 16 * 12]
    stp x28, x29, [x1, 16 * 13]
    str x30,      [x1, 16 * 14]
    // we cap achvieve this with move and str
    // but likely it won't be needed
    //str fp   sp ,  [x1, 16*15]
    
    // load x0, x1 from stack
    mov x3, x1
    ldp x0, x1, [sp]
    add sp, sp, #16

    // store x0, x1 in trap frame
    stp x0, x1, [x3, 16 * 0]
	
    mov x0, x8
	b handle_syscall

.global userret
userret:
     // get current process
    mrs x0, tpidr_el1
    // get this process trap frame address into x1
    ldr x3, [x0, 14*8]

    //load x0(into x2),x1 from trap frame and store on stack 
    ldp x0, x1, [x3, 16 * 0]
    sub sp, sp, #16
    stp x0, x1, [sp]
    
    mov x1, x3

    // load rest of registers from trap frame
    ldp x2,   x3, [x1, 16 * 1]
    ldp x4,   x5, [x1, 16 * 2]
    ldp x6,   x7, [x1, 16 * 3]
    ldp x8,   x9, [x1, 16 * 4]
    ldp x10, x11, [x1, 16 * 5]
    ldp x12, x13, [x1, 16 * 6]
    ldp x14, x15, [x1, 16 * 7]
    ldp x16, x17, [x1, 16 * 8]
    ldp x18, x19, [x1, 16 * 8]
    ldp x20, x21, [x1, 16 * 9]
    ldp x22, x23, [x1, 16 * 10]
    ldp x24, x25, [x1, 16 * 11]
    ldp x26, x27, [x1, 16 * 12]
    ldp x28, x29, [x1, 16 * 13]
    ldr x30,      [x1, 16 * 14]
    // we can achvieve this with move and str
    // but likely it won't be needed
    //ldp fp  sp_el0, [x1, 16*15]

    // finally load x0, x1 from stack
    ldp x0, x1, [sp]
    add sp, sp, #16
    
    // return from exception
    eret 
    
// switch state between two processes 
.global switch_to // (prev, next) only needs to save callee saved as caler saved are already on the stack
switch_to:
    stp x19, x20, [x0, 16 * 0]
    stp x21, x22, [x0, 16 * 1]
    stp x23, x24, [x0, 16 * 2]
    stp x25, x26, [x0, 16 * 3]
    stp x27, x28, [x0, 16 * 4]
    stp fp, lr, [x0, 16 * 5]
    mov x9, sp
    str x9, [x0, 16 * 6]


    ldp x19, x20, [x1, 16 * 0]
    ldp x21, x22, [x1, 16 * 1]
    ldp x23, x24, [x1, 16 * 2]
    ldp x25, x26, [x1, 16 * 3]
    ldp x27, x28, [x1, 16 * 4]
    ldp fp, lr, [x1, 16 * 5]
    ldr x9, [x1, 16 * 6]
    mov sp,  x9
    // switch to new proc pagetable in el0
    ldr x0, [x1,16*8] // load pagetable address
    msr ttbr0_el1, x0 // switch translation based address
    tlbi vmalle1is // invalidate all TLB entries
    msr tpidr_el1, x1
    ret

.global get_current_process
get_current_process:
    mrs x0, tpidr_el1
    ret
