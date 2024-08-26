// user process
#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"


// free process can be allocated by create_process
enum ProcesState {FREE, RUNNING, RUNNABLE, WAITING, KILLED};

struct process{
    uint8_t pid;
    enum ProcesState state; // running runnable, waiting, killed
    struct PageFrame *stack_frame;
    // frame to store registers after context switch
    struct PageFrame *reg_state_frame;
};

#define PROCCNT 64
struct process process_array[PROCCNT];

// basically set stack, set registers state page
// and jump to code_start_addr
// return -1 if there is no space left for new process
int create_process(void *code_start_addr, uint8_t pid){
    
    // allocate process from proc array
    struct process *proc;
    for(int i = 0; i < PROCCNT; i++){
        proc = &process_array[i];
        if(proc->state == FREE){
            proc->state = WAITING;
            break;
        }
        // didn't find free proc
        if(i == PROCCNT -1){
            return -1;
        }
    }

    // allocate page for stack     
    struct PageFrame *stack_frame, *reg_state_frame;
    if(alloc_page(stack_frame) == -1){
        proc->state = FREE;
        return -1;
    }
    if(alloc_page(reg_state_frame) == -1){
        proc->state = FREE;
        free_page(stack_frame);
        return -1;
    }

    /**
     * @Todo 
     **/
    // setup stack_frame and reg_state_page
    proc->state = RUNNABLE;

}

int kill_process(uint8_t pid){
    // mark process as dead
    struct process *proc;
    for(int i = 0; i < PROCCNT; i++){
        proc = &process_array[i];
        if (proc->pid == pid){
            proc->state = KILLED;
            break;
        }
        if(i = PROCCNT - 1){
            return -1;
        }
    }
    // free state
    free_page(proc->stack_frame);
    free_page(proc->reg_state_frame);
    proc->state = FREE; 
    return 0;
}


/*
Timer interrupt jumps here 
*/
void scheduler(){
    int proc = 0;
    for(;;) {
        for(int proc = 0; proc < PROCCNT; proc++ ){
            //save current process state

            // load next process


        }
    }
}

//void user_start(void *prog, struct PageFrame *stackframe, struct PageFrame *reg_state_frame){
    //__asm__ volatile("nop");
//}


/**
 * TODO 
 * write asm to load state into pages
 * write asm to switch between two processes
 */