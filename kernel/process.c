// user process
#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"

// first process adress
extern void user_program(void);

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
    struct PageFrame *trap_frame;
    struct process *parent;
};

#define PROCCNT 64
struct process process_array[PROCCNT];

////////////////////////////////////////////////////////////////
////           Process creation and scheduling              ////
////////////////////////////////////////////////////////////////

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


uint8_t get_pid(){
    uint8_t pid = 1;
    while(1){
        for(int i =0; i < PROCCNT; i++){
            if(process_array[i].pid == pid){
                break;
            }
            // if last process doesn't have this pid it's free to use
            if(i == PROCCNT-1){
                return pid;
            }
        }
        pid++;
    }
}

// basically set stack, set registers state page
// and jump to code_start_addr
// return -1 if there is no space left for new process
__attribute__((noreturn)) void create_first_process(){
    
    // allocate process from proc array
    struct process *proc = &process_array[0];
    proc->state = WAITING;

    // allocate page for stack     
    struct PageFrame *stack_frame, *trap_frame;
    kalloc(stack_frame);
    kalloc(trap_frame);

    // setup stack_frame and reg_state_page
    free_registers(proc);
    proc->sp = (uint64_t)&(proc->stack_frame[PageSize-1]);
    proc->trap_frame = trap_frame;
    proc->state = RUNNABLE;
    proc->pid = 0;

    user_start(user_program, proc->sp);
}
int fork(){
    // get caller process
    struct process *caller = get_current_process();
    
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
    struct PageFrame *stack_frame, *trap_frame;
    if(kalloc(stack_frame) == -1){
        proc->state = FREE;
        return -1;
    }

    if(kalloc(trap_frame) == -1){
        kfree(stack_frame);
        proc->state = FREE;
        return -1;
    }

    free_registers(proc);

    proc->sp = (uint64_t)&(proc->stack_frame[PageSize-1]);
    //copy content from caller stack_frame
    strcpy(caller->stack_frame,proc->stack_frame,PageSize);

    proc->trap_frame = trap_frame;

    // copy rest of registers from caller
    proc->x19 = caller->x19;
    proc->x20 = caller->x20;
    proc->x21 = caller->x21;
    proc->x22 = caller->x22;
    proc->x23 = caller->x23;
    proc->x24 = caller->x24;
    proc->x25 = caller->x25;
    proc->x26 = caller->x26;
    proc->x27 = caller->x27;
    proc->x28 = caller->x28;
    proc->fp  = caller->fp;
    proc->lr  = caller->lr;

    proc->pid = get_pid();
    proc->parent = caller;

    proc->state = RUNNABLE;
}

// later switch to searching from initramfs and elf parsing
// for now just provide a function we want to jump to
void execve( void(*fun)()  ){
    // get caller process
    struct process *proc = get_current_process();
    clear_page(proc->stack_frame);

    // set elr_e,1 to entry point of <fun> so that when
    // kernel eret el0 will start execution in it
    __asm__ volatile (
        "msr elr_el1, %0\n"
        :
        : "r"(fun)
    );
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
    kfree(proc->stack_frame);
    kfree(proc->trap_frame);
    proc->state = FREE; 
    return 0;
}
// Timer interrupt jumps here 

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

///////////////////////////////////////////////////////////////////
////                        Syscalls                            ///
///////////////////////////////////////////////////////////////////
//
// User call appropriate api, function with argument
// then our function call svc instruction
// this generates synchronous exception
// this exception is handled by el1(kernel)
// os then validates all args passed
// performs action and return, which ensure that execution
// will resume at ELO
// right after svc
//
// so user is provided with wrappers that 
// pass syscall number and args to kernel
// and generates exception 


///////////////////////////////////////////////////////////////////
////                          Signals                           ///
///////////////////////////////////////////////////////////////////
int user_fork(){
    int ret;
    __asm__ volatile(
        "mov x8, 4\n"
        "svc 0 \n"
        : "=r"(ret)
        : 
        : "x0","x8"
    );
}

/*
int execve(const char *pathname, char *const argv[], int argc){
    int ret;
    __asm__ volatile(
        "mov x8, 5"
        "svc 0 \n"
        : "=r"(ret)
        : "r" (pathname), "r"(argv), "r"(argc)
        : "x0", "x1", "x2", "x8"
    );
}
*/
