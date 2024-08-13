#include "types.h"
#include "memory_map.h"
#include "definitions.h"

void rand_init(){
    *RNG_STATUS=0x40000;
    // mask interrupt
    *RNG_INT_MASK|=1;
    // enable
    *RNG_CTRL|=1;
}

/**
 * Return a random number between [min..max]
 */
unsigned int rand(unsigned int min, unsigned int max)
{
    // may need to wait for entropy: bits 24-31 store how many words are
    // available for reading; require at least one
    while(!((*RNG_STATUS)>>24)) asm volatile("nop");
    return *RNG_DATA % (max-min) + min;
}