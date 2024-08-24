#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"

// single user program with global stack for now
__attribute__ ((aligned (16))) uint8_t user_stack[4096];