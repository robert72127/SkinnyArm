#pragma once

#include "types.h"

enum SYSCALL_TABLE{
    OPEN,
    READ,
    WRITE,
    CLOSE,
    FORK,
    EXECVE,
    WAIT,
    SLEEP,
    EXIT,
    MMAP,
    MUNMAP
};


int open(const char *pathname, int flags);
int read(int fd, void *buf, uint64_t size);
int write(int fd, void *buf, uint64_t size);
int close(int fd);
int fork();
int execve(const char *pathname, char *const argv[], int argc);
int wait();
/* sleep until signal arrives */
int sleep(uint32_t seconds);
int exit(int exit_code);
int mmap(void *addr, uint64_t size);
void munmmap(void *addr, uint64_t size);