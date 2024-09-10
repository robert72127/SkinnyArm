#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"

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
/**
 * Handle extraction from virtual address
 */

// there are 3 indirect leveles we need to go before our final addr
#define IND_LVL 3
// bits63..0
// get offset from va into given level pte
#define LVL_ADDR(addr, level) (( (addr) >> (30 - (level*9) )) & 0x1FF)
// get page offset from va
#define OFFSET(addr) ((addr) & 0x1FF)
/**
 * Construct page_table_Entry
 */
// set bit in a range 63..0 to val 1/0
#define SET_BIT(addr, bit, val)  ( ( (addr) & (~ ( 1 << (bit) ) ) )  | (1<< (bit) ) )
#define GET_BIT(addr, bit)  (( (addr) & (1 << (bit)) ) >> (bit) )
// bit 54, non executable by kernel
#define SET_K_NON_EXEC(addr)  SET_BIT(addr, 54, 1)
#define SET_K_EXEC(addr)  SET_BIT(addr, 54, 0)
#define GET_K_EXEC(addr)  SET_BIT(addr, 54)
//bit 53, non executable by user
#define SET_U_NON_EXEC(addr) SET_BIT(addr, 53,1)
#define SET_U_EXEC(addr)  SET_BIT(addr, 53, 0)
#define GET_U_EXEC(addr) GET_BIT(addr, 53)
// set physical page address in page table entry
#define SET_PHYSICAL_ADDR(addr, phys_addr) ( ( (addr) & (~ 0x0000FFFFFFFFF000) ) |   ( ( (phys_addr) << 12  )  & 0x0000FFFFFFFFF000))
#define GET_PHYSICAL_ADDR(addr) ((addr &  0x0000FFFFFFFFF000 ) >> 12)  
//bit 10, if not set page fault is generated when accessing
#define SET_PAGE_SET(addr)  SET_BIT(addr, 10, 1) 
#define SET_PAGE_NOT_SET(addr)  SET_BIT(addr, 10, 0) 
#define GET_PAGE_SET(addr)  GET_BIT(addr, 10) 
// bit 7, 0 is read-write, 1 is for read only
#define SET_U_RONLY(addr) SET_BIT(addr, 7, 1)
// warning u_rw means kernl is non executable in this region 
#define SET_U_RW(addr) SET_BIT(addr, 7, 0)
#define GET_RW(addr) GET_BIT(addr, 7)
// bit 6, 0 for kernel only 1 for user/kernel access 
#define SET_KACCES(addr) SET_BIT(addr, 6, 0)
#define SET_KUACCESS(addr) SET_BIT(addr, 6, 1)
#define GET_KUACCESS(addr) GET_BIT(addr, 6, 0)
//bit 4-2, index to MAIR
#define SET_MAIR(addr, index)  (SET_BIT(SET_BIT(SET_BIT(addr, 4, ( (index) >> 4)),  3, ( (index) >> 3)) , 2, ( (index) >> 2)) ) 
#define GET_MAIR(addr, index) ((GET_BIT(addr, 4) << 2) | ( GET_BIT(addr, 3),  1) |  GET_BIT(addr, 2)) 
// bits 1-0, specify next level
// 11 - page table
// 01 - page
// *0 - invalid
#define SET_NEXT_LEVEL_PTABLE(addr) SET_BIT(SET_BIT(addr, 1, 1 ), 0, 1) 
#define SET_NEXT_LEVEL_PAGE(addr)  SET_BIT(SET_BIT(addr, 1, 0 ) , 0, 1) 
#define SET_NEXT_LEVEL_INVALID(addr)  SET_BIT(SET_BIT(addr, 1, 0 ), 0, 0) 
#define GET_NEXT_LEVEL(addr) (GET_BIT(addr, 1) << 1) | GET_BIT(addr, 0)

// points to next level table
#define PGDPUD_ENTRY(addr, phys_addr) (SET_K_NON_EXEC(addr) | SET_PAGE_SET(addr) |  SET_U_NON_EXEC(addr) | SET_NEXT_LEVEL_PAGE(addr) | SET_PHYSICAL_ADDR(addr, phys_addr) )
// points to page_table_entry (ie actuall page that holds user/kernel data)
#define PMD_ENTRY(addr, phys_addr) (SET_K_NON_EXEC(addr) | SET_PAGE_SET(addr) |  SET_U_NON_EXEC(addr) | SET_NEXT_LEVEL_PAGE(addr) | SET_PHYSICAL_ADDR(addr, phys_addr) )
// points to kernel page
#define KERNEL_ENTRY(addr, phys_addr) (SET_K_EXEC(addr) | SET_U_NON_EXEC(addr) | SET_PAGE_SET(addr) | SET_KACCES(addr) | SET_MAIR(addr, 0) | SET_NEXT_LEVEL_INVALID(addr) | SET_PHYSICAL_ADDR(addr, phys_addr) )
// kernel device pages
#define DEVICE_ENTRY(addr, phys_addr) (SET_K_EXEC(addr) | SET_U_NON_EXEC(addr) | SET_PAGE_SET(addr) | SET_KACCES(addr) | SET_MAIR(addr, 1) | SET_NEXT_LEVEL_INVALID(addr) | SET_PHYSICAL_ADDR(addr, phys_addr) )
// points to user page
#define USER_ENTRY(addr, phys_addr) (SET_K_EXEC(addr) | SET_U_EXEC(addr) | SET_PAGE_SET(addr) | SET_U_RW(addr) | SET_KACCES(addr) | SET_MAIR(addr, 0) | SET_NEXT_LEVEL_INVALID(addr) | SET_PHYSICAL_ADDR(addr, phys_addr) )
// invalid entry
#define INVALID_ENTRY(addr) (SET_PAGE_NOT_SET(addr))

// each page is 4 kB, each entry is 64 bits so there will be 512 of them
#define PENTRY_CNT 512

/**
 * @todo we also need to set priviledges etc for pages
 * after implementing just mapping do that
 */


/**
 * @brief search for physical page corresponding to virtual address
 * if there isn't one:
 * if you are at level 2 and pa != 0
 *      set entry to pa
 * else if pa = 0 
 *      alloc new page
 * else 
 * kind can be one of 0: KERNEL, 1: Device, 2 User
 */
struct PageFrame *get_physical_page(pagetable_t pagetable, uint64_t va, uint64_t pa, uint8_t kind){
    // we need to decide at each level which bucket it belongs to
    struct PageFrame* frame;
    pagetable_t parent_table;
    uint64_t pentry;
    for(int i = 0; i <= 2; i++){
        pentry = 0;
        parent_table = pagetable;
        // if this indirect page doesn't exists yet create it
        if( GET_PAGE_SET(pagetable[(va)]) == 0 ){
            if(i == 2 && pa != 0) {
                switch (kind)
                    {
                        case 0:
                            pentry = KERNEL_ENTRY(pentry, pa);
                            break;
                        case 1:
                            pentry = DEVICE_ENTRY(pentry, pa);
                            break;
                        case 2:
                            pentry = USER_ENTRY(pentry, pa);
                            break;
                        default:
                            return NULL;
                    }
                    parent_table[LVL_ADDR(va,i)] = pentry;
                    return (struct PageFrame *)pa;
            }
            else if(kalloc(&frame) == 0){
                switch (i)
                {
                case 0:
                    pentry = PGDPUD_ENTRY(pentry, (uint64_t)frame);
                    break;
                case 1:
                    pentry = PMD_ENTRY(pentry, (uint64_t)frame);
                case 2:
                    switch (kind)
                    {
                        case 0:
                            pentry = KERNEL_ENTRY(pentry, (uint64_t)frame);
                            break;
                        case 1:
                            pentry = DEVICE_ENTRY(pentry, (uint64_t)frame);
                            break;
                        case 2:
                            pentry = USER_ENTRY(pentry, (uint64_t)frame);
                            break;
                        default:
                            kfree(&frame);
                            return NULL;
                    }
                    break;
                default:
                    kfree(&frame);
                    return NULL;
                }
                
                parent_table[LVL_ADDR(va,i)] = pentry;
                pagetable = (pagetable_t)(frame);
            }
            else{
                return NULL;
            }
        }
        pagetable = &pagetable[LVL_ADDR(va ,i)];
        frame = (struct PageFrame *)(pagetable);
    }
    return frame; 
}

/**
 * @brief
 * map single page of data
 * kind can be one of 0: KERNEL, 1: Device, 2 User
 */
int map_page(pagetable_t pagetable, uint64_t va, uint64_t pa, uint8_t kind){
    struct PageFrame *frame = get_physical_page(pagetable,va,pa,kind);
    if (frame == NULL){
        return -1;
    }
    return 0;
}


/**
 * @brief
 * Populate enough of page table to accomodate range from va_start to va_end
 * kind can be one of 0: KERNEL, 1: Device, 2 User
 */
int map_region(pagetable_t pagetable, uint64_t va_start, uint64_t pa_start, uint64_t size, uint8_t kind){
    // expect va_start and va_end to be multiple of pagesize
    if (va_start % PageSize != 0 || (va_start + size) % PageSize != 0 || pa_start % PageSize != 0) {
        return -1;
    }
    uint64_t va_curr = va_start;
    while(va_curr < va_start + size){
        if(map_page(pagetable, va_start, pa_start, kind) == -1){
            unmap_region(pagetable, va_start, va_curr);
            return -1;
        }
        pa_start += PageSize;
        va_curr += PageSize;
    }
}

/**
 * @brief
 * free physical frame corresponding to vmaddr pages
 * after that check if we can free indirect pages
 * if so do that too
 */
void free_page(pagetable_t pagetable, int64_t va){
    pagetable_t parent_table, gparent_table, current_table = pagetable;
    uint64_t pentry;
    for(int i = 0; i <= 2; i++){
        gparent_table = parent_table;
        parent_table = current_table;
        pentry = current_table[LVL_ADDR(va ,i)];
        if (GET_PAGE_SET(pentry) == 0){
            return;
        }
        current_table = GET_PHYSICAL_ADDR(pentry);

    }
    struct PageFrame *frame = (struct PageFrame *)current_table;
    kfree(&frame);
    pentry = SET_PAGE_NOT_SET(0);
    parent_table[LVL_ADDR(va,2)] = pentry;

    // check if parent contains other entries, if not free it too
    for(int i = 0; i < PENTRY_CNT; i++ ){
        if( GET_PAGE_SET(parent_table[i]) != 0){
            return;
        }
    } 
    frame = (struct PageFrame *)parent_table;
    kfree(&frame);
    gparent_table[LVL_ADDR(va, 1)] = INVALID_ENTRY(0) ;

    for(int i = 0; i < PENTRY_CNT; i++ ){
        if( GET_PAGE_SET(parent_table[i]) != 0){
            return;
        }
    }
    frame = (struct PageFrame *)gparent_table;
    kfree(&frame);
    pagetable[LVL_ADDR(va, 0)] = INVALID_ENTRY(0); 
}

/**
 * @brief
 * walk page table and free defined region
 */
int unmap_region(pagetable_t pagetable, uint64_t va_start, uint64_t va_end){
    while(va_start < va_end){
        free_page(pagetable, va_start);
        va_start += PageSize;
    }
}

/**
 * @brief
 * free's whole page table with all subtables
 */
void free_pagetable(pagetable_t pagetable){
    pagetable_t lpage,ppage,gpage;
    uint64_t pentry;
    struct PageFrame *frame;
    for(int i = 0; i < PENTRY_CNT; i++){
        if (GET_PAGE_SET(pagetable[i]) != 0){
            pentry = GET_PHYSICAL_ADDR(pagetable[i]);
            gpage = pentry;
            for(int j = 0; j < PENTRY_CNT; j++){
                if (GET_PAGE_SET(gpage[j]) != 0){
                    pentry = GET_PHYSICAL_ADDR(gpage[j]);
                    gpage = pentry;
                    for(int k = 0; k < PENTRY_CNT; k++){
                        if (GET_PAGE_SET(ppage[k]) != 0){
                            pentry = GET_PHYSICAL_ADDR(ppage[k]);
                            lpage = pentry;
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
/**
 * @brief create page table for kernel
 */
pagetable_t make_kpagetable(){
    struct Pagetable *pud;
    if(kalloc(&pud) != 0) {
        return NULL;
    }

    pagetable_t kpagetable = pud;    
    uint64_t va_start = VKERN_START + &_start;
    uint64_t va_end = VKERN_START + &_end;

    uint8_t *kernel_memory = &_start;

    // in memory_map.h
    // MMIO_BASE and MMIO_END is defined with VKERN_START already added
    uint8_t *mmio_memory = MMIO_BASE - VKERN_START;

    // whole kernel address space is 1 GB
    // and also map peripherials
    if (map_region(kpagetable, va_start, &_start, va_end - va_start , 0) == -1 ){
        return NULL;
    } 
    //map peripherials
    if(map_region(kpagetable, MMIO_BASE, mmio_memory, MMIO_BASE-MMIO_END, 1) == -1){
        return NULL;
    }
    return kpagetable;
}


/**
 * @brief clone pagetable for fork
 *  at each level we should copy page 
 * then walk the copy and change phyisical addresses
 */

int clone_pagetable(pagetable_t from, pagetable_t to){
    strcpy((uint8_t *)from, (uint8_t *)to, PageSize); 
    pagetable_t from_upage, from_mpage, from_page;
    pagetable_t to_upage, to_mpage, to_page;
    uint64_t fppage, tppage;
    for(int i = 0 ; i < PENTRY_CNT; i++){
        if(GET_PAGE_SET(from[i]) == 0){
            continue;
        }
        fppage = GET_PHYSICAL_ADDR(from[i]);
        from_upage = fppage;
        kalloc(&to_upage);
        strcpy(from_upage, to_upage, PageSize);
        to[i] = (uint64_t)to_upage;
        for(int j = 0; j < PENTRY_CNT; j++){
            if(GET_PAGE_SET(from_upage[j]) == 0){
                continue;
            }
            fppage = GET_PHYSICAL_ADDR(from_upage[j]);
            from_mpage = fppage;
            kalloc(&to_mpage);
            strcpy(from_mpage, to_mpage, PageSize);
            to_upage[j] = (uint64_t)to_mpage;
            for(int k = 0; k < PENTRY_CNT; k++){
                if(GET_PAGE_SET(from_mpage[k]) == 0){
                    continue;
                }
                fppage = GET_PHYSICAL_ADDR(from_mpage[k]);
                from_page = fppage;
                kalloc(&to_page);
                strcpy(from_page, to_page, PageSize);
                to_mpage[j] = (uint64_t)to_page;
            }
        }
    }

}


void copy_from_user(pagetable_t user_pgtb, uint8_t* from, uint8_t* to, uint64_t size){
    pagetable_t va = (pagetable_t)(( (uint64_t)from / PageSize) * PageSize);
    from = (uint8_t *)((uint64_t)from % PageSize);
    struct pageframe *frame = get_physical_page(user_pgtb, va,0, 2);
    from = (uint8_t *)frame + (uint64_t)from;
    strcpy(from, to, size);
    if( (uint64_t)from % PageSize + size < PageSize){
        return;
    }
    size = size - PageSize + (uint64_t)from;
    while(size > 0){
        va += PageSize;
        to += size;
        frame = get_physical_page(user_pgtb, va,0, 2);
        strcpy((uint8_t *)frame, to, size);
        size -= PageSize;
    }

}

void copy_to_user(pagetable_t user_pgtb, char* from, char *to, uint64_t size){
    pagetable_t va = (pagetable_t)(( (uint64_t)to / PageSize) * PageSize);
    to = (uint8_t *)((uint64_t)to % PageSize);
    struct pageframe *frame = get_physical_page(user_pgtb, va,0, 2);
    to = (uint8_t *)frame + (uint64_t)to;
    strcpy(from, to, size);
    if( (uint64_t)from % PageSize + size < PageSize){
        return;
    }
    size = size - PageSize + (uint64_t)to;
    while(size > 0){
        va += PageSize;
        from += size;
        frame = get_physical_page(user_pgtb, va,0, 2);
        strcpy(from, (uint8_t *)frame, size);
        size -= PageSize;
    }
}


