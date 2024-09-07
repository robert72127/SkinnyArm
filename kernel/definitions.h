#pragma once

#include "low_level.h"

// uart.c
void uart_init();
void uart_interrupt();
void uart_send(uint8_t c);
char uart_getc();
void uart_puts(uint8_t *s);

// vmem.c
// kalloc
struct PageFrame{
    struct  PageFrame *next; 
    // 4096 - 8 for pointer
    char data[PageSize - sizeof(struct  Pageframe*)];
};
void kalloc_init();
int kalloc_kern_reserve(uint64_t start_addr, uint64_t end_addr);
void kfree(struct  PageFrame **page);
int kalloc(struct PageFrame **page);
void clear_page(struct PageFrame *page);
// virtual memory
void vmem_enable();


// rootfs.c
struct file;
init_ramfs();
int read(uint8_t *filename, uint32_t offset);
int ls(uint8_t *filepath);
int search_file(uint8_t *name, struct file **f);

// process
void create_first_process();
int sys_fork();
int sys_execve();
int sys_wait();
int sys_sleep();
int sys_exit();


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
void strcpy(uint8_t *src, uint8_t *dst, uint64_t size);