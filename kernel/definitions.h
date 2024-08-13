#pragma once

// uart.c
void uart_init();
void uart_send(unsigned int c);
char uart_getc();
void uart_puts(char *s);

// rand.c