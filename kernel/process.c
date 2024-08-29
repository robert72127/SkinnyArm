// user process
#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"


__attribute__((aligned(16))) uint8_t scheduler_stack[NCPU * 4096];



// free process can be allocated by create_process
enum ProcesState {FREE, RUNNING, RUNNABLE, WAITING, KILLED};


struct process{
    // registers this space is used during context switch
    int64_t x19;
    int64_t x20;
    int64_t x21;
    int64_t x22;
    int64_t x23;
    int64_t x24;
    int64_t x25;
    int64_t x26;
    int64_t x27;
    int64_t x28;
    int64_t fp;
    int64_t lr;
    int64_t sp;
    // used for thread management
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

void free_registers(struct process *proc){
    proc->x19 = 0;
    proc->x20 = 0;
    proc->x21 = 0;
    proc->x22 = 0;
    proc->x23 = 0;
    proc->x24 = 0;
    proc->x25 = 0;
    proc->x26 = 0;
    proc->x27 = 0;
    proc->x28 = 0;
    proc->fp = 0;
    proc->lr = 0;
    proc->sp = 0;
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
    proc->pid = 0;
    free_registers(proc);
    free_page(proc->stack_frame);
    free_page(proc->reg_state_frame);
    proc->state = FREE; 
    return 0;
}

/*
Timer interrupt jumps here 
*/
void scheduler(){
    // read current process from system register
    struct process *curr_proc = get_current_process();
    curr_proc->state = RUNNABLE;
    // next process in scheduler array
    struct process *next_proc = curr_proc + 1; 
    for(;;) {
        for(;next_proc < &process_array[PROCCNT]; next_proc++ ){
            if  (next_proc->state == RUNNABLE){
                
                //save current process state and load next process
                switch_to(curr_proc, next_proc);
                curr_proc = next_proc;
                // return to interrupt 
                return;
            }
        }
        next_proc = &process_array[0];
    }
}
