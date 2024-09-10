#include "types.h"
#include "memory_map.h"
#include "definitions.h"
#include "low_level.h"

__attribute__((aligned(16))) uint8_t kernel_stack[NCPU * 4096];

extern char _end;

void tick()
{
    uart_puts("SYSCALL\n");
}

void main()
{
    uint8_t cpu_id = get_cpu_id();

    // print hello world from core 0
    if (cpu_id == 0)
    {
        uint8_t *rootfs_start = (char *)RAMFS_START;
        uint8_t *rootfs_end;
        if(init_ramfs(&rootfs_end) != 0){
            rootfs_end = rootfs_start;
        }
        // add padding to rootfs_end
        rootfs_end = (char *) (( (uint32_t)(rootfs_end + PageSize -1) / PageSize ) * PageSize);

        // vmem starts after rootfs_end so we are good
        kalloc_init();

        // map kernel pages 
        pagetable_t kernel_pagetable = make_kpagetable(); 
        // load kernel page table
        enable_vmem(kernel_pagetable);


        irq_vector_init();
        // enable all kinds interrupts for el0
        enable_interrupts();
        // set up serial console
        uart_init();
        uart_puts("Hello World!\n");
        enable_timer_interrupt();
        // uncomment to enable user timer interrupt with demo user code
        // this function needs general enable_timer_interrupt to work
        user_timer_interrputs_enable();

        //vmem_init(); 
       
        create_first_process();
        /*
         // say hello
         while (1)
         {
             char c= uart_getc();
             uart_puts(&c);
         }
         */
    }
    // loop forever on all cores
    while (1)
        ;
}