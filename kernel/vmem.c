#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"

extern char _vmem_start;
const char *vmem_start =(char *)(&_vmem_start);
const char *vmem_end = (char*)(MMIO_BASE);

struct PageFrame *page_frame_linked_list;

//////////////////////////////////////////////////////////////////////////
//
//                  kmalloc
//
//////////////////////////////////////////////////////////////////////////


// init vm skip rootfs space assume hole start and end 
// are aligned to page size, if not overwrite them
void kalloc_init(){
    uint64_t num_pages = 0;

    char *current_address = vmem_start;
    struct PageFrame *current_frame, *previous_frame = NULL;

    if(current_address + PageSize < vmem_end){
        page_frame_linked_list = (struct PageFrame*)(current_address);
        current_address += PageSize;
    }
    else{
        return;
    }
    previous_frame = page_frame_linked_list;

    while(current_address <= vmem_end){
        current_frame = (struct PageFrame *)(current_address);
        if(previous_frame != NULL) {
            previous_frame->next = current_frame;
        }
        current_address += PageSize;
        previous_frame = current_frame;
        num_pages++;
    }
    current_frame->next = NULL;
}

/** 
*   @brief reserves special regions for kernel,
*   used only during initialization, right after kalloc_init
*   so no need to check whether page is used
*/
int kalloc_kern_reselve(uint64_t start_addr, uint64_t end_addr){
    // rount start down and end up to multiple of page size's
    start_addr = (start_addr / PageSize ) * PageSize;
    end_addr = ((start_addr + PageSize - 1) / PageSize) * PageSize;

    struct PageFrame *ll_page, *prev_page, *current_page = page_frame_linked_list;
    uint64_t addr = 0;
    while(addr < start_addr){
        current_page = current_page->next;
        addr += PageSize;
    }
    ll_page = current_page;
    prev_page = current_page;
    while(addr < end_addr){
        prev_page->next = current_page->next;
        clear_page(current_page);
        current_page = prev_page->next;
        addr += PageSize;
    }
}



// clear page
void clear_page(struct PageFrame *page){
    page->next = NULL;
    for(int i = 0; i < PageSize- sizeof(struct Pageframe*); i++){
        page->data[i] = 0;
    }
}

// add page in front of ll
void kfree(struct PageFrame **page){
    clear_page(*page);
    struct PageFrame *previous_head = page_frame_linked_list;
    (*page)->next = page_frame_linked_list;
    page_frame_linked_list = *page;
}

// get page
int kalloc(struct PageFrame **page){
    struct PageFrame *previous_head = page_frame_linked_list;
    if(previous_head == NULL){
        return -1;
    }
    page_frame_linked_list = previous_head->next;
    clear_page(previous_head);
    *page = previous_head;
    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
//                  Starting virtual memory
//
//////////////////////////////////////////////////////////////////////////


// accessibility
#define PT_KERNEL   (0<<6)      // privileged, supervisor EL1 access only
#define PT_USER     (1<<6)      // unprivileged, EL0 access allowed
#define PT_RW       (0<<7)      // read-write
#define PT_RO       (1<<7)      // read-only
#define PT_AF       (1<<10)     // accessed flag
#define PT_NX       (1UL<<54)   // no execute

///There are 4 levels in the table hierarchy: PGD (Page Global Directory),
// PUD (Page Upper Directory), 
// PMD (Page Middle Directory), 
// PTE (Page Table Entry). 
// PTE is the last table in the hierarchy,
//  and it points to the actual page in the physical memory.

// Memory translation process starts by locating the address of PGD (Page Global Directory)
// table. The address of this table is stored in the ttbr0_el1 register. 
// Each process has its own copy of all page tables, including PGD,
// and therefore each process must keep its PGD address. 
// During a context switch, PGD address of the next process is loaded into the ttbr0_el1 register.

/*
Entry of PGD, PUD, PMD which point to a page table
+-----+------------------------------+---------+--+
|     | next level table's phys addr | ignored |11|
+-----+------------------------------+---------+--+
     47                             12         2  0

Entry of PUD, PMD which point to a block
+-----+------------------------------+---------+--+
|     |  block's physical address    |attribute|01|
+-----+------------------------------+---------+--+
     47                              n         2  0

Entry of PTE which points to a page
+-----+------------------------------+---------+--+
|     |  page's physical address     |attribute|11|
+-----+------------------------------+---------+--+
     47                             12         2  0

Invalid entry
+-----+------------------------------+---------+--+
|     |  page's physical address     |attribute|*0|
+-----+------------------------------+---------+--+
     47                             12         2  0


Bits[54]
The unprivileged execute-never bit, non-executable page frame for EL0 if set.

Bits[53]
The privileged execute-never bit, non-executable page frame for EL1 if set.

Bits[47:n]:
The physical address the entry point to. Note that the address should be aligned to 
 Byte.

Bits[10]
The access flag, a page fault is generated if not set.

Bits[7]
0 for read-write, 1 for read-only.

Bits[6]
0 for only kernel access, 1 for user/kernel access.

Bits[4:2]
The index to MAIR.

Bits[1:0]
Specify the next level is a block/page, page table, or invalid.
*/


#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1
/*
void vmem_enable(){
    __asm__ volatile(
        "ldr x0, %0\n"
        "msr tcr_el1, x0\n"
        :
        :  "r"(TCR_CONFIG_DEFAULT)
        : "x0"

    );

}
*/