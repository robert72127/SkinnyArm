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
//|        |   level 0      |  level1        | level 2        |   LEVEL 3      |
//+------------------------------------------------------------------------------------------------+
/**
 * Handle extraction from virtual address
 */

// there are 3 indirect leveles we need to go before our final addr
#define IND_LVL 3
// bits63..0
// get offset from va into given level pte
#define LVL_ADDR(addr, level) ( ( (addr) >> (39 - (level*9) )) & 0x1FF)
// get page offset from va
#define OFFSET(addr) ((addr) & 0x1FF)
/**
 * Construct page_table_Entry
 */
// set bit in a range 63..0 to val 1/0
#define SET_BIT(addr, bit, val)  ( ( (addr) & (~ ( 1ULL << (bit) ) ) )  | (val << (bit) ) )
#define GET_BIT(addr, bit)  (( (addr) & (1ULL << (bit)) ) >> (bit) )
//bit 54, non executable by user
#define SET_U_NON_EXEC(addr) SET_BIT(addr, 54,0ULL)
#define SET_U_EXEC(addr)  SET_BIT(addr, 54, 1ULL)
#define GET_U_EXEC(addr) GET_BIT(addr, 54)
// bit 53, non executable by kernel
#define SET_K_NON_EXEC(addr)  SET_BIT(addr, 53, 0ULL)
#define SET_K_EXEC(addr)  SET_BIT(addr, 53, 1ULL)
#define GET_K_EXEC(addr)  SET_BIT(addr, 53)
// set physical page address in page table entry
#define SET_PHYSICAL_ADDR(addr, phys_addr) ( ( (addr) & (~ 0x0000FFFFFFFFF000) ) |   ( ( (phys_addr) << 12  )  & 0x0000FFFFFFFFF000))
#define GET_PHYSICAL_ADDR(addr) ((addr &  0x0000FFFFFFFFF000 ) >> 12)  
//bit 10, if not set page fault is generated when accessing
#define SET_PAGE_SET(addr)  SET_BIT(addr, 10, 1ULL) 
#define SET_PAGE_NOT_SET(addr)  SET_BIT(addr, 10, 0ULL) 
#define GET_PAGE_SET(addr)  GET_BIT(addr, 10ULL) 
// bit 7, 0 is read-write, 1 is for read only
#define SET_RONLY(addr) SET_BIT(addr, 7, 1ULL)
// warning u_rw means kernl is non executable in this region 
#define SET_RW(addr) SET_BIT(addr, 7, 0ULL)
#define GET_RW(addr) GET_BIT(addr, 7ULL)
// bit 6, 0 for kernel only 1 for user/kernel access 
#define SET_KACCES(addr) SET_BIT(addr, 6, 0ULL)
#define SET_KUACCES(addr) SET_BIT(addr, 6, 1ULL)
#define GET_KUACCES(addr) GET_BIT(addr, 6, 0ULL)
//bit 4-2, index to MAIR
#define SET_MAIR(addr, index)  (SET_BIT(SET_BIT(SET_BIT(addr, 4, ( 0x1 & ((index) >> 2 ))),  3, (0x1 & ((index) >> 1) ) ) , 2, (0x1 & index)  ) ) 
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
#define PGDPUD_ENTRY(addr, phys_addr) (SET_K_NON_EXEC(SET_PAGE_SET(SET_U_NON_EXEC(SET_NEXT_LEVEL_PAGE(SET_PHYSICAL_ADDR(addr, phys_addr) )))))
// points to page_table_entry (ie actuall page that holds user/kernel data)
#define PMD_ENTRY(addr, phys_addr) (SET_K_NON_EXEC(SET_PAGE_SET(SET_U_NON_EXEC(SET_NEXT_LEVEL_PAGE(SET_PHYSICAL_ADDR(addr, phys_addr) )))))
// points to kernel page
#define KERNEL_ENTRY(addr, phys_addr) (SET_K_EXEC(SET_U_NON_EXEC(SET_PAGE_SET(SET_KACCES(SET_MAIR (SET_NEXT_LEVEL_INVALID(SET_PHYSICAL_ADDR(addr, phys_addr)),0))))))
// kernel device pages
#define DEVICE_ENTRY(addr, phys_addr) (SET_K_EXEC(SET_U_NON_EXEC(SET_PAGE_SET(SET_RW(SET_KACCES(SET_MAIR(SET_NEXT_LEVEL_INVALID(SET_PHYSICAL_ADDR(addr, phys_addr)),1)))))))
// points to user page
#define USER_ENTRY(addr, phys_addr) (SET_K_EXEC(SET_U_EXEC(SET_PAGE_SET(SET_RW(SET_KUACCES(SET_MAIR(SET_NEXT_LEVEL_INVALID(SET_PHYSICAL_ADDR(addr, phys_addr) ),0)))))))
// invalid entry
#define INVALID_ENTRY(addr) (SET_PAGE_NOT_SET(SET_PHYSICAL_ADDR(addr, 0)))

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
    uint64_t pentry;
    for(int i = 0; i <= 3; i++){
        pentry = 0;
        // if this indirect page doesn't exists yet create it
        if( GET_PAGE_SET(pagetable[LVL_ADDR(va,i)]) == 0 ){
            if(i == 3 && pa != 0) {
                switch (kind)
                    {
                        case 0:
                            pentry = PGDPUD_ENTRY(pentry, pa);
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
                    pagetable[LVL_ADDR(va,i)] = pentry;
                    return (struct PageFrame *)pa;
            }
            else if(kalloc(&frame) == 0){
                switch (i)
                {
                case 0: case 1:
                    pentry = PGDPUD_ENTRY(pentry, (uint64_t)frame);
                    break;
                case 2:
                    pentry = PMD_ENTRY(pentry, (uint64_t)frame);
                    break;
                case 3:
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
                
                pagetable[LVL_ADDR(va,i)] = pentry;
            }
            else{
                return NULL;
            }
        }
        pagetable = GET_PHYSICAL_ADDR(pagetable[LVL_ADDR(va ,i)]);
    }
    return (struct PageFrame *)pagetable; 
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
        if(map_page(pagetable, va_curr, pa_start, kind) == -1){
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
    pagetable_t levels[5];

    levels[0] = pagetable;
    
    for(int i = 1 ; i < 5; i ++){
        if(GET_PAGE_SET(levels[0][LVL_ADDR(va,i-1)])  == 0){
            return;
        }
        levels[i] = GET_PHYSICAL_ADDR(levels[i-1][LVL_ADDR(va,i-1)]);
    }
    kfree(levels[4]);
    for(int i = 3; i >= 0; i--){
        levels[i][LVL_ADDR(va,i)] = 0;
        for(int j = 0; j < PENTRY_CNT; j++){
            if(GET_PAGE_SET(levels[i][j]) != 0){
                return;
            }
        }
        kfree(levels[i]);
    }
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
    pagetable_t levels[5];
    levels[0] = pagetable;
    for(int i = 0; i < PENTRY_CNT; i++){
        levels[1] = GET_PHYSICAL_ADDR(levels[0][i]);
        if (GET_PAGE_SET(levels[0][i]) ==0 ){
            continue;
        }
        for(int j = 0; j < PENTRY_CNT; j++){
            levels[2] = GET_PHYSICAL_ADDR(levels[1][j]);
            if (GET_PAGE_SET(levels[1][j]) ==0 ){
                continue;
            }
            for(int k = 0; k< PENTRY_CNT; k++ ){
                levels[3] = GET_PHYSICAL_ADDR(levels[2][k]);
                if (GET_PAGE_SET(levels[2][k]) ==0 ){
                    continue;
                }
                for(int l = 0; l < PENTRY_CNT; l++){
                    levels[4] = GET_PHYSICAL_ADDR(levels[3][l]);
                    if (GET_PAGE_SET(levels[3][l]) ==0 ){
                        continue;
                    }
                    kfree(levels[4]);
                }
                kfree(levels[3]);
            }
            kfree(levels[2]);
        }
        kfree(levels[1]);
    }
    kfree(levels[0]);
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
    if(map_region(kpagetable, MMIO_BASE, mmio_memory, MMIO_END-MMIO_BASE, 1) == -1){
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
    pagetable_t froms[5];
    pagetable_t tos[5];
    struct PageFrame *frame;
    froms[0] = from;
    tos[0] = tos;
    strcpy(froms[0], tos[0], PageSize);
    for(int i = 0; i < PENTRY_CNT; i++){
        froms[1] = GET_PHYSICAL_ADDR(froms[0][i]);
        if (GET_PAGE_SET(froms[0][i]) ==0 ){
            continue;
        }
        if(kalloc(frame) == -1){
            free_pagetable(to);
            return -1;
        }
        strcpy(froms[1], tos[1], PageSize);
        tos[0][i] = PGDPUD_ENTRY(0LL, (uint64_t)frame );
        tos[1] = frame;

        for(int j = 0; j < PENTRY_CNT; j++){
            froms[2] = GET_PHYSICAL_ADDR(froms[1][j]);
            if (GET_PAGE_SET(froms[1][j]) ==0 ){
                continue;
            }
            if(kalloc(frame) == -1){
                    free_pagetable(to);
                    return -1;
            }
            strcpy(froms[2], tos[2], PageSize);
            tos[1][j] = PGDPUD_ENTRY(0LL, (uint64_t)frame );
            tos[2] = frame;

            for(int k = 0; k< PENTRY_CNT; k++ ){
                froms[3] = GET_PHYSICAL_ADDR(froms[2][k]);
                if (GET_PAGE_SET(froms[2][k]) ==0 ){
                    continue;
                }
                if(kalloc(frame) == -1){
                        free_pagetable(to);
                        return -1;
                }
                strcpy(froms[3], tos[3], PageSize);
                tos[2][k] = PGDPUD_ENTRY(0LL, (uint64_t)frame );
                tos[3] = frame;

                for(int l = 0; l < PENTRY_CNT; l++){
                    froms[4] = GET_PHYSICAL_ADDR(tos[3][l]);
                    if (GET_PAGE_SET(tos[3][l]) ==0 ){
                        continue;
                    }
                    if(kalloc(frame) == -1){
                            free_pagetable(to);
                            return -1;
                        }
                    tos[3][l] = PGDPUD_ENTRY(0LL, (uint64_t)frame );
                    tos[4] = frame;
                    strcpy(froms[4],tos[4], PageSize);
                }
            }
        }
    }
    return 0;
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
