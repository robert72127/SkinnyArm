## Custom Operating System for Raspberry Pi 3b+

### Overview
 Operating system designed for the Raspberry Pi 3b+, targeting the ARMv8-A (AArch64) architecture.

### Requirements:
    * aarch64-none-linux-gnu toolchain

### Running:
```
make
make run
```

### To be done:
 * Add more syscalls
 * Add Signal handling mechanism
 * Add multicore support
    * Cores are already initialized
    * Locking primitives are implemented
    * Remainig work is introducing spinlocks to safeguard critical sections


### Reference:
    * https://github.com/bztsrc/raspi3-tutorial/tree/master
    * https://people.cs.nycu.edu.tw/~ttyeh/course/2024_Spring/IOC5226/outline.html
    * https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf
