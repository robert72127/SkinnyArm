#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"

void spinlock_init(struct SpinLock *lk, char *name){
    lk->locked = 0;
    lk->cpu = -1;
    lk->name = name;
}

int spinlock_holds(struct SpinLock *lck){
    uint8_t cpu_id = get_cpu_id();
    return (lck->cpu == cpu_id);
}

void spinlock_acquire(struct SpinLock *lck){
    // disable interrupts to avoid deadlock
    disable_interrupts();

    while(__atomic_test_and_set(&lck->locked, __ATOMIC_ACQUIRE) != 0);
    __sync_synchronize();
    lck->cpu = get_cpu_id();
}

/**
 *  if -1 there is critical error
 */
int spinlock_release(struct SpinLock *lck){
   if (! spinlock_holds(lck)){
    return -1;
   }
   lck->cpu = 0;
   __sync_synchronize();
    __atomic_clear(&lck->locked, __ATOMIC_RELEASE);
    return 0;

}

void sleeplock_init(struct SleepLock *lk, char *name){
    lk->locked = 0;
    lk->pid = 0;
    lk->name = name;
    spinlock_init(&lk->spin_lk, "spinlock");

}

int sleeplock_holds(struct SleepLock *lk){
    struct process *proc = get_current_process();
    return (lk->pid == proc->pid);
}

void sleeplock_acquire(struct SleepLock *lk){
    spinlock_acquire(&lk->spin_lk);
    while(lk->locked){
       sleep(lk, &lk->spin_lk);
    }
    lk->locked = 1;
    lk->pid = get_current_process();
    spinlock_release(&lk->spin_lk);
}

void sleeplock_release(struct SleepLock *lk){
    spinlock_acquire(&lk->spin_lk);
    lk->locked =0;
    lk->pid = 0;
    // wakeup all waiting on that lock
    wakeup(lk);
    spinlock_release(&lk->spin_lk);
}