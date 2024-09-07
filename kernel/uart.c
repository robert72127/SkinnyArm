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
    *AUX_MU_IIR = 0x6;    // enable interrupts
    *AUX_MU_CNTL = 1;    // enable Tx, Rx


    // uart irq
    *AUX_MU_IER = 0b10; // enable TX, RX interrupts

}

void uart_send(uint8_t c) {
    /* wait until transmiter is empty and we can send */
    while(!(*AUX_MU_LSR & 0x20)){ spin();}
    /* write the character to the buffer */
    *AUX_MU_IO=c;
}

char uart_getc_sync() {
    char r;
    // wait until something arrives in the buffer 
    while(! (*AUX_MU_LSR&0x01)) { spin();}
    // read it and return 
    r =  (char)(*AUX_MU_IO);
    // convert carriage return to newline 
    return r=='\r'?'\n':r;
}

char uart_getc() {
    char r;
    /* wait until something arrives in the buffer */
    if((*AUX_MU_LSR&0x01)){

    /* read it and return */
    r =  (char)(*AUX_MU_IO);
    /* convert carriage return to newline */
    return r=='\r'?'\n':r;
    }
    else return -1;
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
    uint32_t iir = *AUX_MU_IIR;
    char c;
    // check if it's read
    if( iir & 0b100){
          c = uart_getc();
          while( c != 255 ){
            uart_send(c);
            c = uart_getc();
          }
    }
}