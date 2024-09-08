#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"

// there are 3 indirect leveles we need to go before our final addr
#define IND_LVL 3

// each page is 4 kB, each entry is 64 bits so there will be 512 of them

//get bits from 47 to 12 corresponding to physical addr of pagetable entry
#define ENTRY_PADDR(x) (( x & 0x0FFF) >> 11)

//////////////////////////////////////////////////////////////////////////
//
//                  Starting virtual memory
//
//////////////////////////////////////////////////////////////////////////
//+------------------------------------------------------------------------------------------------+
//|         |47:39 PGD Index |38:30 PUD Index | 29:21PMD Index |20:12 PTE Index | 11:0 Page offset |
//+------------------------------------------------------------------------------------------------+
//|                          |   level 0      |  level1        | level 2        |                  |
//+------------------------------------------------------------------------------------------------+
#define LVL_ADDR(addr, level) (( (addr) >> (30 - (level*9) )) & 0x1FF)
#define OFFSET(addr) ((addr) & 0x1FF)
/**
 * search for physical address corresponding to virtual one
 * if there isn't one create it
 */

/**
 * @todo we also need to set priviledges etc for pages
 * after implementing just mapping do that
 */


struct PageFrame *get_physical_page(pagetable_t pagetable, uint64_t va){
    // we need to decide at each level which bucket it belongs to
    struct PageFrame* frame;
    pagetable_t parent_table;
    for(int i = 0; i <= 2; i++){
        parent_table = pagetable;
        pagetable = &pagetable[LVL_ADDR(va ,i)];
        // if this indirect page doesn't exists yet create it
        if(pagetable == NULL ){
            if(kalloc(&frame) == 0){
                parent_table[LVL_ADDR(va,i)] = (uint64_t)frame;
                pagetable = (pagetable_t)(frame);
            }
            else{
                return NULL;
            }
        }
        else{
            frame = (struct PageFrame *)(pagetable);
        }

    }
    return frame; 
}

// map single page of data
int map_page(pagetable_t pagetable, uint64_t va, uint8_t *data){
    struct PageFrame *frame = get_physical_page(pagetable,va);
    if (frame == NULL){
        return -1;
    }
    strcpy(data, (uint8_t *)frame, PageSize);
    return 0;
}


/**
 * Populate enough of page table to accomodate range from va_start to va_end
 */
int map_region(pagetable_t pagetable, uint64_t va_start, uint64_t va_end, char* data){
    // expect va_start and va_end to be multiple of pagesize
    if (va_start % PageSize != 0 || va_end % PageSize != 0){
        return -1;
    }
    uint64_t va_curr = va_start;
    while(va_curr < va_end){
        if(map_page(pagetable, va_start, data) == -1){
            unmap_region(pagetable, va_start, va_curr);
            return -1;
        }
        data += PageSize;
        va_curr += PageSize;
    }
}

/**
 * free physical frame
 * after that check if we can free indirect pages
 * if so do that too
 */
void free_page(pagetable_t pagetable, int64_t va){

    pagetable_t parent_table, gparent_table, current_table = pagetable;
    for(int i = 0; i <= 2; i++){
        gparent_table = parent_table;
        parent_table = current_table;
        current_table = &current_table[LVL_ADDR(va ,i)];
        // if this indirect page doesn't exists yet create it
        if(current_table == NULL ){
            return;
        }
    }
    struct PageFrame *frame = (struct PageFrame *)current_table;
    kfree(&frame);
    parent_table[LVL_ADDR(va,2)] = 0;

    // check if parent contains other entries, if not free it too
    for(int i = 0; i < PageSize / sizeof(uint64_t); i++ ){
        if( parent_table[i] != 0){
            return;
        }
    } 
    frame = (struct PageFrame *)parent_table;
    kfree(&frame);
    gparent_table[LVL_ADDR(va, 1)] = 0;

    for(int i = 0; i < PageSize / sizeof(uint64_t); i++ ){
        if(gparent_table[i] != 0){
            return;
        }
    }
    frame = (struct PageFrame *)gparent_table;
    kfree(&frame);
    pagetable[LVL_ADDR(va, 0)] = 0; 
}

/**
 * walk page table and free defined region
 */
int unmap_region(pagetable_t pagetable, uint64_t va_start, uint64_t va_end){
    while(va_start < va_end){
        free_page(pagetable, va_start);
        va_start += PageSize;
    }
}

/**
 * free's whole page table with all subtables
 */
void free_pagetable(pagetable_t pagetable){
    pagetable_t lpage,ppage,gpage;
    struct PageFrame *frame;
    for(int i = 0; i < PageSize/ sizeof(uint64_t); i++){
        gpage = pagetable[i];
        if(gpage){
            for(int j = 0; j < PageSize/ sizeof(uint64_t); j++){
                ppage = gpage[j];
                if(ppage){
                    for(int k = 0; k < PageSize/ sizeof(uint64_t); k++){
                        lpage = ppage[k];
                        if(lpage){
                            frame = (struct PageFrame *)lpage;
                            kfree(&frame);
                        }
                    }
                    frame = (struct PageFrame *)ppage;
                    kfree(&frame);
                }
            }
            frame = (struct PageFrame *)gpage;
            kfree(&frame);
        }
    }
    frame = (struct PageFrame *)pagetable;
    kfree(&frame);
}

extern char _end;
extern char _start;
// create page table for kernel
pagetable_t make_kpagetable(){
    struct Pagetable *pud;
    pagetable_t kpagetable = kalloc(&pud);
    uint64_t va_start = VKERN_START + &_start;
    uint64_t va_end = VKERN_START + &_end;

    uint8_t *kernel_memory = &_start;

    // in memory_map.h
    // MMIO_BASE and MMIO_END is defined with VKERN_START already added
    uint8_t *mmio_memory = MMIO_BASE - VKERN_START;

    // whole kernel address space is 1 GB
    // and also map peripherials
    if (map_region(kpagetable, va_start, va_end, kernel_memory) == -1 ){
        return NULL;
    } 

    //map peripherials
    if(map_region(kpagetable, MMIO_BASE, MMIO_END+1, mmio_memory) == -1){
        return NULL;
    }
    return kpagetable;
}


/**
 * clone pagetable for fork
 */
int clone_pagetable(pagetable_t from, pagetable_t to){
    pagetable_t from_lpage, from_ppage,from_gpage;
    pagetable_t to_lpage, to_ppage,to_gpage;

    struct PageFrame *from_frame, *to_frame;
    for(int i = 0; i < PageSize/ sizeof(uint64_t); i++){
        from_gpage = from[i];
        if(from_gpage){
            for(int j = 0; j < PageSize/ sizeof(uint64_t); j++){
                from_ppage = from_gpage[j];
                if (from_ppage){
                    for(int k = 0; k < PageSize/ sizeof(uint64_t); k++){ 
                        from_lpage = from_ppage[k];
                        if (from_lpage){
                            from_frame = (struct PageFrame *)from_lpage;
                            if(kalloc(&to_frame) == -1){
                            }
                            strcpy((uint8_t *)from_frame, (uint8_t *)to_frame, PageSize);
                            to_lpage[k] = to_frame; 
                        }
                    }
                    from_frame = (struct PageFrame *)from_ppage;
                    if(kalloc(&to_frame) == -1){
                    }
                    strcpy((uint8_t *)from_frame, (uint8_t *)to_frame, PageSize); 
                    to_ppage[j] = to_frame; 
                }
            }
            from_frame = (struct PageFrame *)from_gpage;
            if(kalloc(&to_frame) == -1){
            }
            strcpy((uint8_t *)from_frame, (uint8_t *)to_frame, PageSize); 
            to_ppage[i] = to_frame; 
        }
    }
    from_frame = (struct PageFrame *)from;
    to_frame = (struct PageFrame *)to;
    strcpy((uint8_t *)from_frame, (uint8_t *)to_frame, PageSize); 
}


void copy_from_user(pagetable_t user_pgtb, uint8_t* from, uint8_t* to, uint64_t size){
    pagetable_t va = (pagetable_t)(( (uint64_t)from / PageSize) * PageSize);
    from = (uint8_t *)((uint64_t)from % PageSize);
    struct pageframe *frame = get_physical_page(user_pgtb, va);
    from = (uint8_t *)frame + (uint64_t)from;
    strcpy(from, to, size);
    if( (uint64_t)from % PageSize + size < PageSize){
        return;
    }
    size = size - PageSize + (uint64_t)from;
    while(size > 0){
        va += PageSize;
        to += size;
        frame = get_physical_page(user_pgtb, va);
        strcpy((uint8_t *)frame, to, size);
        size -= PageSize;
    }

}

void copy_to_user(pagetable_t user_pgtb, char* from, char *to, uint64_t size){
    pagetable_t va = (pagetable_t)(( (uint64_t)to / PageSize) * PageSize);
    to = (uint8_t *)((uint64_t)to % PageSize);
    struct pageframe *frame = get_physical_page(user_pgtb, va);
    to = (uint8_t *)frame + (uint64_t)to;
    strcpy(from, to, size);
    if( (uint64_t)from % PageSize + size < PageSize){
        return;
    }
    size = size - PageSize + (uint64_t)to;
    while(size > 0){
        va += PageSize;
        from += size;
        frame = get_physical_page(user_pgtb, va);
        strcpy(from, (uint8_t *)frame, size);
        size -= PageSize;
    }
}


