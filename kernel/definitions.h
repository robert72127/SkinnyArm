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
struct PageFrame *get_physical_page(pagetable_t pagetable, uint64_t va, uint64_t pa, uint8_t kind);
int map_page(pagetable_t pagetable, uint64_t va, uint64_t pa, uint8_t kind);
int map_region(pagetable_t pagetable, uint64_t va_start, uint64_t pa_start, uint64_t size, uint8_t kind);
void free_page(pagetable_t pagetable, int64_t va);
int unmap_region(pagetable_t pagetable, uint64_t va_start, uint64_t va_end);
void free_pagetable(pagetable_t pagetable);
pagetable_t make_kpagetable();
int clone_pagetable(pagetable_t from, pagetable_t to);
void copy_from_user(pagetable_t user_pgtb, uint8_t* from, uint8_t* to, uint64_t size);
void copy_to_user(pagetable_t user_pgtb, char* from, char *to, uint64_t size);

// lock
struct SpinLock{
    uint8_t locked;
    int8_t cpu;
    char *name;
};
void spinlock_init(struct SpinLock *lk, char *name);
void spinlock_acquire(struct SpinLock *lk);
int spinlock_release(struct SpinLock *lk);
int spinlock_holds(struct SpinLock *lk);
// lock
struct SleepLock{
    // spin lock protecting this SleepLock
    struct SpinLock spin_lk;
    uint8_t locked;
    uint8_t pid;
    char *name;
};
void sleeplock_init(struct  SleepLock *lk, char *name);
void sleeplock_acquire(struct SleepLock *lk);
void sleeplock_release(struct SleepLock *lk);
int sleeplock_holds(struct SleepLock *lk);

// rootfs.c
struct file;
int init_ramfs();
int read(uint8_t *filename, uint32_t offset);
int ls(uint8_t *filepath);
int search_file(uint8_t *name, struct file **f);

// process.c
enum ProcesState {FREE, RUNNING, RUNNABLE, WAITING, KILLED};
struct process{
    // registers this space is used during context switch
    int64_t x19;
    int64_t x20;
    int64_t x21;
    int64_t x22;
    int64_t x23;
    int64_t x24;
    int64_t x25;
    int64_t x26;
    int64_t x27;
    int64_t x28;
    int64_t fp;
    int64_t lr;
    int64_t sp;
    // used for thread management
    struct PageFrame *stack_frame;
    // frame to store registers after context switch
    struct PageFrame *trap_frame;
    struct process *parent;
    pagetable_t pagetable;
    enum ProcesState state; // running runnable, waiting, killed
    uint8_t pid;
    struct SleepLock *wait_on;
    // struct SpinLock proc_lock;
};

void create_first_process();
int sys_fork();
int sys_execve();
int sys_wait();
int sys_sleep();
int sys_exit();
void sleep(struct SleepLock *lk, struct SpinLock *spin_lk);
void wakeup(struct SleepLock *lk);

// string
uint8_t strequal(uint8_t *str1, uint8_t *str2);
void strcpy(uint8_t *src, uint8_t *dst, uint64_t size);