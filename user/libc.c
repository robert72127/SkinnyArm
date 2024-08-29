#pragma once

#include "types.h"
#include "libc.h"

int open(const char *pathname, int flags){
    int fd;
    __asm__ volatile(
        "mov x8, 0"
        "svc 0 \n"
        : "=r"(fd)
        : "r" (pathname), "r"(flags)
        : "x0", "x1", "x8"
    );
}
int read(int fd, void *buf, uint64_t size){
    int ret;
    __asm__ volatile(
        "mov x8, 1"
        "svc 0 \n"
        : "=r"(ret)
        : "r" (buf), "r"(size)
        : "x0", "x1", "x8"
    );
}

int write(int fd, void *buf, uint64_t size){
    int ret;
    __asm__ volatile(
        "mov x8, 2"
        "svc 0 \n"
        : "=r"(ret)
        : "r" (buf), "r"(size)
        : "x0", "x1", "x8"
    );
}

int close(int fd){
    int ret;
    __asm__ volatile(
        "mov x8, 3"
        "svc 0 \n"
        : "=r"(ret)
        : 
        : "x0", "x8"
    );

}

int fork(){
    int ret;
    __asm__ volatile(
        "mov x8, 4"
        "svc 0 \n"
        : "=r"(ret)
        : 
        : "x0","x8"
    );
}

int execve(const char *pathname, char *const argv[], int argc){
    int ret;
    __asm__ volatile(
        "mov x8, 5"
        "svc 0 \n"
        : "=r"(ret)
        : "r" (pathname), "r"(argv), "r"(argc)
        : "x0", "x1", "x2", "x8"
    );
}

int wait(){
    int ret;
    __asm__ volatile(
        "mov x8, 6"
        "svc 0 \n"
        : "=r"(ret)
        : 
        :"x0", "x8"
    );
}

int sleep(){
    int ret;
    __asm__ volatile(
        "mov x8, 7"
        "svc 0 \n"
        : "=r"(ret)
        : 
        : "x0", "x8"
    );
}

int exit(int exit_code){
    int ret;
    __asm__ volatile(
        "mov x8, 8"
        "svc 0 \n"
        : "=r"(ret)
        : "r"(exit_code)
        : "x0", "x1", "x8"
    );
}

int mmap(void *addr, uint64_t size){
    int ret;
    __asm__ volatile(
        "mov x8, 9"
        "svc 0 \n"
        : "=r"(ret)
        : "r"(addr), "r"(size)
        : "x0", "x1", "x2" "x8"
    );
}

void munmmap(void *addr, uint64_t size){
    int ret;
    __asm__ volatile(
        "mov x8, 10"
        "svc 0 \n"
        : "=r"(ret)
        : "r"(addr), "r"(size)
        : "x0", "x1", "x2" "x8"
    );
}