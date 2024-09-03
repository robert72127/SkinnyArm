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
void kalloc_init(uint8_t *hole_start, uint8_t *hole_end){
    uint64_t num_pages = 0;

    char *current_address = vmem_start;
    struct PageFrame *current_frame, *previous_frame = NULL;
   
    while(current_address <= vmem_end){

        if(current_address == hole_start){
            while(current_address < hole_end){
                current_address += PageSize;
            }
        }
        
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

// clear page
static void clear_page(struct PageFrame *page){
    page->next = NULL;
    for(int i = 0; i < PageSize- sizeof(struct Pageframe*); i++){
        page->data[i] = 0;
    }
}

// add page in front of ll
void kfree(struct PageFrame *page){
    clear_page(page);
    struct PageFrame *previous_head = page_frame_linked_list;
    page_frame_linked_list = page;
    page->next = page_frame_linked_list;
}

// get page
int kalloc(struct PageFrame *page){
    struct PageFrame *previous_head = page_frame_linked_list;
    if(previous_head == NULL){
        return -1;
    }
    page_frame_linked_list = previous_head->next;
    clear_page(previous_head);
    page = previous_head;
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