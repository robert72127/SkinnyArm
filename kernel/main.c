#include "types.h"
#include "kernel.h"
#include "low_level.h"
#include "uart.h"

__attribute__ ((aligned (16))) uint8_t kernel_stack[NCPU * 4096];

void main()
{
    uint8_t cpu_id = get_cpu_id();

    if(cpu_id == 0){ 
        // set up serial console
        uart_init();
        
        // say hello
        uart_puts("Hello World!\n");
        
        // echo everything back
        while(1) {
            uart_send(uart_getc());
        }
    }
    // loop forever
    while(1);
}