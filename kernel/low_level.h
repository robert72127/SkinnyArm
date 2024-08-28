#pragma once

#include "types.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

#define ROOTFS_FILE_COUNT 256
#define ROOTFS_INODE_COUNT 256
#define ROOTFS_MAX_FILENAME_SIZE 16

#define PageSize 4096

uint8_t get_cpu_id();

void spin();
void irq_vector_init();
void enable_interrupts();
void disable_interrupts();
int enable_timer_interrupt();


void user_timer_interrputs_enable();

