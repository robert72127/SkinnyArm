#pragma once

#include "low_level.h"

// uart.c
void uart_init();
void uart_interrupt();
void uart_send(uint8_t c);
char uart_getc();
void uart_puts(uint8_t *s);

// kalloc.c
struct PageFrame{
    struct  PageFrame *next; 
    // 4096 - 8 for pointer
    char data[PageSize - sizeof(struct  Pageframe*)];
};
void kalloc_init();
void kalloc_kern_reserve(uint64_t start_addr, uint64_t end_addr);
void kfree(struct  PageFrame **page);
int kalloc(struct PageFrame **page);
void clear_page(struct PageFrame *page);
// virtual memory
void vmem_enable();

// vmem.c
struct PageFrame *get_physical_page(pagetable_t pagetable, uint64_t va);
int map_page(pagetable_t pagetable, uint64_t va, uint8_t *data);
int map_region(pagetable_t pagetable, uint64_t va_start, uint64_t va_end, char* data);
void free_page(pagetable_t pagetable, int64_t va);
int unmap_region(pagetable_t pagetable, uint64_t va_start, uint64_t va_end);
void free_pagetable(pagetable_t pagetable);
pagetable_t make_kpagetable();
int clone_pagetable(pagetable_t from, pagetable_t to);
void copy_from_user(pagetable_t user_pgtb, uint8_t* from, uint8_t* to, uint64_t size);
void copy_to_user(pagetable_t user_pgtb, char* from, char *to, uint64_t size);


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

// string
uint8_t strequal(uint8_t *str1, uint8_t *str2);
void strcpy(uint8_t *src, uint8_t *dst, uint64_t size);