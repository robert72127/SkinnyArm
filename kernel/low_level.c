#include "types.h"

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

//void enable_interrupts(){}

//void disable_interrupts(){}

//void enable_timer();
//void disable_timer();