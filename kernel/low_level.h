#pragma once

#include "types.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

uint8_t get_cpu_id();

void spin();
void irq_vector_init();
void enable_interrupts();
void disable_interrupts();
int enable_timer_interrupt();


void user_timer_interrputs_enable();

