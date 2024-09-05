#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"

void exception_handler(unsigned long state,unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far)
{
    // print out state
    switch(state) {
        case 0: uart_puts("EL1T"); break;
        case 1: uart_puts("EL1H"); break;
        case 2: uart_puts("EL0_64"); break;
        case 3: uart_puts("EL0_32"); break;
    }
    uart_puts(":\t");
    // print out interruption type
    switch(type) {
        case 0: uart_puts("Synchronous"); break;
        case 1: uart_puts("IRQ"); break;
        case 2: uart_puts("FIQ"); break;
        case 3: uart_puts("SError"); break;
    }
    uart_puts(":\n");
    // decode exception type (some, not all. See ARM DDI0487B_b chapter D10.2.28)
    switch(esr>>26) {
        case 0b000000: uart_puts("Unknown"); break;
        case 0b000001: uart_puts("Trapped WFI/WFE"); break;
        case 0b001110: uart_puts("Illegal execution"); break;
        case 0b010101: uart_puts("System call"); break;
        case 0b100000: uart_puts("Instruction abort, lower EL"); break;
        case 0b100001: uart_puts("Instruction abort, same EL"); break;
        case 0b100010: uart_puts("Instruction alignment fault"); break;
        case 0b100100: uart_puts("Data abort, lower EL"); break;
        case 0b100101: uart_puts("Data abort, same EL"); break;
        case 0b100110: uart_puts("Stack alignment fault"); break;
        case 0b101100: uart_puts("Floating point"); break;
        default: uart_puts("Unknown"); break;
    }
    uart_puts("\n");
    // decode data abort cause
    if(esr>>26==0b100100 || esr>>26==0b100101) {
        uart_puts(", ");
        switch((esr>>2)&0x3) {
            case 0: uart_puts("Address size fault"); break;
            case 1: uart_puts("Translation fault"); break;
            case 2: uart_puts("Access flag fault"); break;
            case 3: uart_puts("Permission fault"); break;
        }
        switch(esr&0x3) {
            case 0: uart_puts(" at level 0"); break;
            case 1: uart_puts(" at level 1"); break;
            case 2: uart_puts(" at level 2"); break;
            case 3: uart_puts(" at level 3"); break;
        }
    }
    uart_puts("\n");
    // dump registers
    /*
    uart_puts(":\n  ESR_EL1 ");
    uart_hex(esr>>32);
    uart_hex(esr);
    uart_puts(" ELR_EL1 ");
    uart_hex(elr>>32);
    uart_hex(elr);
    uart_puts("\n SPSR_EL1 ");
    uart_hex(spsr>>32);
    uart_hex(spsr);
    uart_puts(" FAR_EL1 ");
    uart_hex(far>>32);
    uart_hex(far);
    uart_puts("\n");
    */
    // no return from exception for now
}

void handle_syscall(uint64_t syscall_nr){
    switch (syscall_nr)
    {
    case 0:
        sys_open();
        break;
    case 1:
        sys_read();
        break;
    case 2:
        sys_write();
        break;
    case 3:
        sys_close();
        break;
    case 4:
        sys_fork();
        break;
    case 5:
        sys_execve();
        break;
    case 6:
        sys_wait();
        break;
    case 7:
        sys_sleep();
        break;
    case 8:
        sys_exit();
        break;
    default:
        uart_puts("Unknown interrupt cause\n");
        break;
    }
    
    __asm__ volatile(
        "b userret\n"
    );
}


void handle_irq(){
    // check if pending interrupts is for mini uart
    // and it's read interrupt
    if (*AUX_IRQ & 0x1  && (*AUX_MU_IIR &0b100) ) {
        uart_interrupt();
    }
    
    uint8_t cpu_id = get_cpu_id();
    uint32_t *corex_irq_src;
    switch (cpu_id){
    case 0: corex_irq_src = CORE0_IRQ_SRC;break;
    case 1: corex_irq_src = CORE1_IRQ_SRC;break;
    case 2: corex_irq_src = CORE2_IRQ_SRC;break;
    case 3: corex_irq_src = CORE3_IRQ_SRC;break;}

    // bit 11 is set if source of irq is timer
    if (*corex_irq_src & 0b10) {
        // switch user thread 
        //scheduler();
        __asm__ volatile(
        // reprogram timer
	    "mrs x0, cntfrq_el0\n"
        "mov x1, 200\n" // 50\n"  // make timer interrupts less frequent
        "lsr x0, x0, x1\n"
 	    "msr cntp_tval_el0, x0\n"
        :
        :
        : "x0", "x1"
        );
    }
    
    // reset interrupt pending
    *AUX_MU_IIR &= 0b001;
}

