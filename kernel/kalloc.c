#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"

extern char _kalloc_start;
const char *kalloc_start =(char *)(&_kalloc_start);
const char *kalloc_end = (char*)(MMIO_BASE);

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

    char *current_address = kalloc_start;
    struct PageFrame *current_frame, *previous_frame = NULL;

    if(current_address + PageSize < kalloc_end){
        page_frame_linked_list = (struct PageFrame*)(current_address);
        current_address += PageSize;
    }
    else{
        return;
    }
    previous_frame = page_frame_linked_list;

    while(current_address <= kalloc_end){
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
