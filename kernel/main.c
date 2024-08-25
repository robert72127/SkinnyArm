#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"

__attribute__((aligned(16))) uint8_t kernel_stack[NCPU * 4096];
// single user program with global stack for now
__attribute__((aligned(16))) uint8_t user_stack[4096];

extern char _end;

void tick()
{
    uart_puts("tick\n");
}

void main()
{
    uint8_t cpu_id = get_cpu_id();

    // print hello world from core 0
    if (cpu_id == 0)
    {
        vmem_init();
        irq_vector_init();
        // enable interrupts for el0
        enable_interrupts();
        // set up serial console
        uart_init();
        uart_puts("Hello World!\n");
        //enable_timer_interrupt();
        //user_start();
       
       /* 
        // say hello
        while (1)
        {
            char c= uart_getc();
            uart_puts(&c);
        }
        */
    }
    // loop forever on all cores
    while (1)
        ;
}