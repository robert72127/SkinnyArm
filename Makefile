CC = aarch64-none-linux-gnu-gcc
AS = aarch64-none-linux-gnu-as
OBJCOPY = aarch64-none-linux-gnu-objcopy
CFLAGS =  -O0 -ffreestanding -nostdlib -nostartfiles -ggdb
#CFLAGS += -Wall
QEMU = qemu-system-aarch64

KERNELDIR = kernel
BUILDDIR = build

SRCS = $(wildcard $(KERNELDIR)/*.c)
OBJS = $(patsubst $(KERNELDIR)/*.c,  $(BUILDDIR)/*.o,  $(SRCS))

all: kernel8.img

.PHONY: run clean

$(BUILDDIR)/boot.o: $(KERNELDIR)/boot.S
	@mkdir -p $(BUILDDIR)
	$(AS) -c $(KERNELDIR)/boot.S -o $(BUILDDIR)/boot.o

$(BUILDDIR)/%.o: $(KERNELDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $^

kernel8.img: $(BUILDDIR)/boot.o $(OBJS)   $(KERNELDIR)/link.ld
	$(CC) $(CFLAGS) $(BUILDDIR)/boot.o  $(OBJS) -T $(KERNELDIR)/link.ld -o $(BUILDDIR)/kernel8.elf
	$(OBJCOPY) $(BUILDDIR)/kernel8.elf -O binary kernel8.img

run: kernel8.img 
	$(QEMU) -M raspi3b -kernel kernel8.img -serial null -serial stdio 

run-gdb: kernel8.img 
	$(QEMU) -M raspi3b -kernel kernel8.img -serial null -serial stdio  -S -gdb tcp::1234  
	# then we load :
	# aarch64-none-linux-gnu-gdb build/kernel8.elf   
	# target remote :1234
	# and we can start debugging

clean:
	rm -r $(BUILDDIR)
	rm kernel8.img