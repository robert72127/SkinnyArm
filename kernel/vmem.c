#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"

#define PageSize 4096
extern char _vmem_start;
const char *vmem_start =(char *)(&_vmem_start);
const char *vmem_end = (char*)(MMIO_BASE);


struct PageFrame{
    struct  PageFrame *next; 
    // 4096 - 8 for pointer
    char data[PageSize - sizeof(struct  Pageframe*)];
};

struct PageFrame *page_frame_linked_list;

void vmem_init(
){

    char *current_address = vmem_start;
    struct PageFrame *current_frame, *previous_frame = NULL;
   
    while(current_address <= vmem_end){
        current_frame = (struct PageFrame *)(current_address);
        if(previous_frame != NULL) {
            previous_frame->next = current_frame;
        }
        current_address += PageSize;
        previous_frame = current_frame;
    }
    current_frame->next = NULL;
}

// clear page

// add page

// get page