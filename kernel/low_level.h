#pragma once

#include "types.h"

#define NULL 0

uint8_t get_cpu_id();

void spin();
void irq_vector_init();
void enable_interrupts();
void disable_interrupts();
int enable_timer_interrupt();

