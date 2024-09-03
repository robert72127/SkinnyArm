#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"

uint8_t strequal(uint8_t *str1, uint8_t *str2){
    uint8_t *c1 = str1;
    uint8_t *c2 = str2;
    while(*c1){
        if(*c1 == *c2){
            c1++;
            c2++;
        }
        else{
            return 0;
        }
    }
    return 1;
}

void strcpy(uint8_t *src, uint8_t *dst, uint64_t size){
    for(int i = 0; i < size; i++){
        *dst = *src;
    }
}