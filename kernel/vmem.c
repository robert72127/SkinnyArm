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
struct Pagerame *get_physical_page(pagetable_t pagetable, uint64_t va){
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

/**
 * Populate enough of page table to accomodate range from va_start to va_end
 */
int map_region(pagetable_t pagetable, uint64_t va_start, uint64_t va_end){
    uint64_t va_curr = va_start;
    while(va_curr < va_end){
        if(get_physical_page(pagetable, va_start) == NULL){
            unmap_region(pagetable, va_start, va_curr);
            return -1;
        }
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
        free_range(pagetable, va_start);
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
        for(int j = 0; j < PageSize/ sizeof(uint64_t); j++){
            ppage = gpage[j];
            for(int k = 0; k < PageSize/ sizeof(uint64_t); k++){
                lpage = ppage[k];
                frame = (struct PageFrame *)lpage;
                kfree(&frame);
            }
            frame = (struct PageFrame *)ppage;
            kfree(&frame);
        }
        frame = (struct PageFrame *)gpage;
        kfree(&frame);
    }
    frame = (struct PageFrame *)pagetable;
    kfree(&frame);
}


// create page table for kernel
pagetable_t make_kpagetable(){
    struct Pagetable *pud;
    pagetable_t kpagetable = kalloc(&pud);

    // whole kernel address space is 1 GB
    // and also map peripherials
    map_region(kpagetable, _start, _end);
}


/**
 * clone pagetable for fork
 */
int clone_pagetable(pagetable_t from, pagetable_t to){

}

void copy_from_user(pagetable_t pgtb, char* addr, uint64_t size){

}

void copy_to_user(pagetable_t pgtb, char* addr, uint64_t size){

}


