#include "types.h"
#include "memory_map.h"
uint8_t get_cpu_id(){
    uint8_t cpu_id;
    __asm__ volatile (
        "mrs %0, mpidr_el1"
    : "=r" (cpu_id)
    );
    return cpu_id;
}

void spin(){
    asm volatile("nop");
}

void irq_vector_init(){
    __asm__ volatile(
        "adr x0, vector\n"
        "msr vbar_el1,x0"
        :
        :
        : "x0"
    );
}

void enable_interrupts(){
    __asm__ volatile(
        "msr daifclr, 0xf"
    );
}
void disable_interrupts(){
    __asm__ volatile(
        "msr daifset, 0xf"
    );
}

void user_timer_interrputs_enable(){
    __asm__ volatile(
        // 3 means we will return to 
    "mov     x2, #0x340\n" // this is fucking important cause setting it to 3f0 would disable irq, which we try to enable
    "msr     spsr_el1, x2\n"
    :
    :
    : "x2"
    );
}


// per core timer interrupt
int enable_timer_interrupt(){
    uint8_t cpu_id = get_cpu_id();
    volatile uint32_t *cpu_control_reg_addr;
    switch (cpu_id)
    {
    case 0: cpu_control_reg_addr = CORE0_TIMER_IRQ_CTRL; break;
    case 1: cpu_control_reg_addr = CORE1_TIMER_IRQ_CTRL; break;
    case 2: cpu_control_reg_addr = CORE2_TIMER_IRQ_CTRL; break;
    case 3:cpu_control_reg_addr = CORE3_TIMER_IRQ_CTRL; break;
    default: return -1;
    }

    __asm__ volatile(
        "mov x0, 1\n"
        "msr cntp_ctl_el0, x0\n" // enable
        "mrs x0, cntfrq_el0\n"
        "mov x1, 200\n"// 50\n" // make timer interrupts less frequent
        "lsr x0, x0, x1\n"
        "msr cntp_tval_el0, x0\n" // set expired time
        : 
        :  
        : "x0", "x1" 
    );
   *cpu_control_reg_addr = 2;  // 0b10 el1 ncntpnsirq exception level 1 non secure
    return 0;
}
//void disable_timer();