#pragma once

// uart.c
void uart_init();
void uart_interrupt();
void uart_send(uint8_t c);
char uart_getc();
void uart_puts(uint8_t *s);

// vm.c
void vmem_init();

// rand.c
void rand_init();
uint64_t rand(uint64_t min, uint64_t max);

// sd_card.c
#define SD_OK 0
#define SD_TIMEOUT -1
#define SD_ERROR -2

int sd_init();
int sd_readblock(uint32_t lba, uint8_t *buffer, uint32_t num);





// string
uint8_t strequal(uint8_t *str1, uint8_t *str2);