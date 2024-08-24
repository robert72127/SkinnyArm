#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"

void uart_init()
{
    register unsigned int r;

    /* initialize UART */
    *AUX_ENABLE |=1;       // enable UART1, AUX mini uart
    *AUX_MU_CNTL = 0;      // disable transmitter and receiver during config
    *AUX_MU_IER = 0;       //  disabler interrupts
    *AUX_MU_LCR = 3;       // set data size 8 bits
    *AUX_MU_MCR = 0;       // dont need auto flow control
    *AUX_MU_BAUD = 270;    // 115200 baud
    *AUX_MU_IIR = 0xc6;    // disable interrupts
    
    /* map UART1 to GPIO pins */
    r=*GPFSEL1;
    r&=~((7<<12)|(7<<15)); // gpio14, gpio15
    r|=(2<<12)|(2<<15);    // alt5
    *GPFSEL1 = r;
    *GPPUD = 0;            // enable pins 14 and 15
    r=150; 
    while(r--) { spin(); }
   
    *GPPUDCLK0 = (1<<14)|(1<<15);
    r=150; 
    while(r--) { spin(); }
    *GPPUDCLK0 = 0;        // flush GPIO setup
    

    *AUX_MU_CNTL = 3;    // enable Tx, Rx
    *AUX_MU_IER = 0x3; // enable TX, RX interrupts
}

void uart_send(unsigned int c) {
    /* wait until transmiter is empty and we can send */
    while(!(*AUX_MU_LSR & 0x20)){ spin();}
    /* write the character to the buffer */
    *AUX_MU_IO=c;
}

char uart_getc() {
    char r;
    /* wait until something arrives in the buffer */
    while(! (*AUX_MU_LSR&0x01)) { spin();}
    /* read it and return */
    r =  (char)(*AUX_MU_IO);
    /* convert carriage return to newline */
    return r=='\r'?'\n':r;
}

void uart_puts(uint8_t *s) {
    while(*s) {
        /* convert newline to carriage return + newline */
       // if(*s=='\n')
        //    uart_send('\r');
        uart_send(*s++);
    }
}

void uart_interrupt(void){
    
    // check if interrupt pending
    if(*AUX_MU_IIR) {
        return;
    }
    
    // check if pending interrupts is for mini uart
    if(*AUX_IRQ && 0x1) {
        return;
    }
    // check if it's read
    if(*AUX_MU_IIR & 0x3){
        uint8_t c = uart_getc();
        uart_send(c);
    }
   
    // reset interrupt pending
    *AUX_MU_IIR |= 0x1;
}