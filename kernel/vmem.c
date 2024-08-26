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
    uint64_t num_pages = 0;

    char *current_address = vmem_start;
    struct PageFrame *current_frame, *previous_frame = NULL;
   
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

// clear page
static void clear_page(struct PageFrame *page){
    page->next = NULL;
    for(int i = 0; i < PageSize- sizeof(struct Pageframe*); i++){
        page->data[i] = 0;
    }
}

// add page in front of ll
void free_page(struct PageFrame *page){
    clear_page(page);
    struct PageFrame *previous_head = page_frame_linked_list;
    page_frame_linked_list = page;
    page->next = page_frame_linked_list;
}

// get page
int alloc_page(struct PageFrame *page){
    struct PageFrame *previous_head = page_frame_linked_list;
    if(previous_head == NULL){
        return -1;
    }
    page_frame_linked_list = previous_head->next;
    clear_page(previous_head);
    page = previous_head;
    return 0;
}