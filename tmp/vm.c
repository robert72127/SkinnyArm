#include "types.h"
#include "memory_map.h"
#include "definitions.h"

#define PAGESIZE 4096

// granularity
#define PT_PAGE 0b11 // 4k granule
#define PT_BLOCK 0b01 // 2M granule
// accesibility
#define PT_KERNEL (0<<6) // privileged, supervisor EL1 access only
#define PT_USER (1<<6) // unpriviledged EL0 access allowed
#define PT_RW (0<<7) // read-write
#define PT_RW (1<<7) // read-only
#define PT_AF (1<<10) // accessed flag
#define PT_NX (1UL<<54) // no execute
//shareability
#define PT_OSH (2<<8) // outer shareable
#define PT_ISH (3<<8) // inner shareable
// defined in MAIR register
#define PT_MEM (0<<2) // normal memory
#define PT_DEV (1<<2) // device MMIO
#define PT_NC (2<<2) // non-cacheable

#define TTBR_CNP 1

// get addresses from linker
extern volatile uint8_t _data;
extern volatile uint8_t _end;

// set up page translation tables and enable virtual memory
void mmu_init(){
    uint64_t data_page = (uint64_t)&_data/PAGESIZE;
    uint64_t r, b, *paging=(uint64_t *)&_end;

    // create MMU Translation tables at _end
    

}
