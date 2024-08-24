	
 #include "memory_map.h"

	// for invalid entries go to invalid_entry handler and print message
    .macro handle_invalid_entry state type
	kernel_entry
	mov x0,  #\state
	mov	x1, #\type // 0 - E1t, 1 - EL1h, 2 - EL0_64, 3 - EL0_32 
	mrs	x2, esr_el1
	mrs	x3, elr_el1
    mrs x4, spsr_el1
    mrs x5, far_el1
	bl	exception_handler
	kernel_exit
	eret
	.endm

    // save and restore context
    .macro	kernel_entry
	sub	sp, sp, 256 // 256 frame size for all saved registers
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	stp	x16, x17, [sp, #16 * 8]
	stp	x18, x19, [sp, #16 * 9]
	stp	x20, x21, [sp, #16 * 10]
	stp	x22, x23, [sp, #16 * 11]
	stp	x24, x25, [sp, #16 * 12]
	stp	x26, x27, [sp, #16 * 13]
	stp	x28, x29, [sp, #16 * 14]
	str	x30, [sp, #16 * 15] 
	.endm

	.macro	kernel_exit
	ldp	x0, x1, [sp, #16 * 0]
	ldp	x2, x3, [sp, #16 * 1]
	ldp	x4, x5, [sp, #16 * 2]
	ldp	x6, x7, [sp, #16 * 3]
	ldp	x8, x9, [sp, #16 * 4]
	ldp	x10, x11, [sp, #16 * 5]
	ldp	x12, x13, [sp, #16 * 6]
	ldp	x14, x15, [sp, #16 * 7]
	ldp	x16, x17, [sp, #16 * 8]
	ldp	x18, x19, [sp, #16 * 9]
	ldp	x20, x21, [sp, #16 * 10]
	ldp	x22, x23, [sp, #16 * 11]
	ldp	x24, x25, [sp, #16 * 12]
	ldp	x26, x27, [sp, #16 * 13]
	ldp	x28, x29, [sp, #16 * 14]
	ldr	x30, [sp, #16 * 15] 
	add	sp, sp, 256 // 256 frame size for all saved registers
	eret
	.endm
///////////////////////////

// entries must be 2^7 aligned
    .macro	ventry	label
	.align	7
	b	\label
	.endm

// Exception table
// must be aligned to 2^11


//// irq
.global irq_vector_init
irq_vector_init:
    // load VBAR_EL1 with virtual vector table address
    adr x0, vectors
    msr vbar_el1, x0 
    ret

.global enable_irq
enable_irq:
    msr daifclr, #2
    ret

.global disable_irq
disable_irq:
    msr daifset, #2
    ret

// per core timer interrupt enable in EL0 for now only for core 0
#define CORE0_TIMER_IRQ_CTRL 0x40000040
.global enable_timer_interrupt 
enable_timer_interrupt:
  mov x0, 1
  msr cntp_ctl_el0, x0 // enable
  mrs x0, cntfrq_el0
  msr cntp_tval_el0, x0 // set expired time
  mov x0, 0b10	// 0b10 this will enable timer interrupt for el0, and el1 
  ldr x1, =CORE0_TIMER_IRQ_CTRL
  str w0, [x1] // unmask timer interrupt
  ret
////////////////////////////////////////////////////////////////////////////
/// There are 4 types of exceptions:                                      //
////////////////////////////////////////////////////////////////////////////
// syynchronous exceptions - caused by currenlty executed instruction     //
// irq - normal interrupts                                                //
// fiq - fast interrupts                                                  //
// SError system error                                                    //
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// There are 4 different states that cause exception:                        //
////////////////////////////////////////////////////////////////////////////////
// EL1t - exception taken from EL1 while stack pointer was  shared with EL0 - //
// this happens when SPSel register holds value 0                             //
//****************************************************************************//
// EL1h - exception taken from EL1 at the time when dedicated stack pointer   //
// was allocated for EL1, this means SPSel holds value 1 and this is the mode // 
// that we are currently executing                                            //
//****************************************************************************//
// EL0_64 Exception is taken from EL0 executing in 64 bit mode                //
//****************************************************************************//
// EL0_32 Exception is taken from EL0 executing in 32 bit mode                //
//****************************************************************************//
////////////////////////////////////////////////////////////////////////////////

// in total we need 16  cases

.global vectors
.align 11 // must be aligned to 0x800
vectors:
    // EL1t
    ventry	sync_invalid_el1t			// Synchronous EL1t
	ventry	irq_exception_el1t			// IRQ EL1t
	ventry	fiq_invalid_el1t			// FIQ EL1t
	ventry	error_invalid_el1t			// Error EL1t
    // EL1h
    ventry	sync_invalid_el1h			// Synchronous EL1t
	ventry	irq_invalid_el1h			// IRQ EL1t
	ventry	fiq_invalid_el1h			// FIQ EL1t
	ventry	error_invalid_el1h			// Error EL1t
    // EL0_64
    ventry	sync_invalid_el0_64			// Synchronous EL1t
	ventry	irq_invalid_el0_64			// IRQ EL1t
	ventry	fiq_invalid_el0_64			// FIQ EL1t
	ventry	error_invalid_el0_64		// Error EL1t
    // EL0_32
    ventry	sync_invalid_el0_32			// Synchronous EL1t
	ventry	irq_invalid_el0_32			// IRQ EL1t
	ventry	fiq_invalid_el0_32			// FIQ EL1t
	ventry	error_invalid_el0_32		// Error EL1t

// now the labels we jump to:

sync_invalid_el1t:
	handle_invalid_entry 0 0

irq_exception_el1t:
	//kernel_entry 
	//bl	handle_irq
	//kernel_exit 
	handle_invalid_entry  0 1

fiq_invalid_el1t:
	handle_invalid_entry  0 2
    
error_invalid_el1t:
	handle_invalid_entry  0 3
////////////////////////////
sync_invalid_el1h:
	handle_invalid_entry  1 0

irq_invalid_el1h:
	handle_invalid_entry  1 1

fiq_invalid_el1h:
	handle_invalid_entry  1 2 
    
error_invalid_el1h:
	handle_invalid_entry  1 3
////////////////////////////
sync_invalid_el0_64:
	handle_invalid_entry  2 0

irq_invalid_el0_64:
//	kernel_entry
	// program to interrupt next time
	bl tick
	mrs x0, cntfrq_el0
 	msr cntp_tval_el0, x0
	kernel_exit
	handle_invalid_entry  2 1

fiq_invalid_el0_64:
	handle_invalid_entry  2 2
    
error_invalid_el0_64:
	handle_invalid_entry  2 3
////////////////////////////
sync_invalid_el0_32:
	handle_invalid_entry  3 0

irq_invalid_el0_32:
	handle_invalid_entry  3 1

fiq_invalid_el0_32:
	handle_invalid_entry  3 2
    
error_invalid_el0_32:
	handle_invalid_entry  3 3