#pragma once

// uart.c
void uart_init();
void uart_send(unsigned int c);
char uart_getc();
void uart_puts(char *s);

// rand.c
void rand_init();
uint64_t rand(uint64_t min, uint64_t max);

// sd_card.c
#define SD_OK 0
#define SD_TIMEOUT -1
#define SD_ERROR -2

int sd_init();
int sd_readblock(uint32_t lba, uint8_t *buffer, uint32_t num);

// delays.c
void wait_cycles(unsigned int n);
void wait_msec(unsigned int n);
unsigned long get_system_timer();
void wait_msec_st(unsigned int n);

// irq.c
void enable_intterrupt_controller();
void irq_vector_init();
void enable_irq();
void disable_irq();


// vm.c
//void mmu_init();