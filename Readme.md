## Custom Operating System for Raspberry Pi 3b+

### Overview
 Custom operating system designed for the Raspberry Pi 3b+, targeting the ARMv8-A (AArch64) architecture.

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
