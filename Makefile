CC = aarch64-none-linux-gnu-gcc
AS = aarch64-none-linux-gnu-as
OBJCOPY = aarch64-none-linux-gnu-objcopy
CFLAGS =  -O0 -ffreestanding -nostdlib -nostartfiles -ggdb
CFLAGS += -Wall
QEMU = qemu-system-aarch64

KERNELDIR = kernel
BUILDDIR = build

SRCS_C = $(wildcard $(KERNELDIR)/*.c)
SRCS_S = $(wildcard $(KERNELDIR)/*.S)
OBJS = $(patsubst $(KERNELDIR)/*.c,  $(BUILDDIR)/*.o,  $(SRCS_C))
OBJS += $(patsubst $(KERNELDIR)/*.S,  $(BUILDDIR)/*.o,  $(SRCS_S))

ROOTFS := $(wildcard rootfs/*)

all: kernel8.img

.PHONY: run clean

$(BUILDDIR)/%.o: $(KERNELDIR)/%.S
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $^

$(BUILDDIR)/%.o: $(KERNELDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $^

kernel8.img:  $(OBJS)   $(KERNELDIR)/link.ld
	$(CC) $(CFLAGS)  $(OBJS) -T $(KERNELDIR)/link.ld -o $(BUILDDIR)/kernel8.elf
	$(OBJCOPY) $(BUILDDIR)/kernel8.elf -O binary kernel8.img

# QEMU loads the cpio archive file to 0x8000000 by default.
rootfs: $(ROOTFS)
	find ./rootfs | cpio -o -H newc > rootfs.cpio	

run: kernel8.img rootfs.cpio 
	$(QEMU) -M raspi3b -kernel kernel8.img -initrd rootfs.cpio -serial null -serial stdio 

run-gdb: kernel8.img rootfs.cpio 
	$(QEMU) -M raspi3b -kernel kernel8.img  -initrd rootfs.cpio -serial null -serial stdio  -S -gdb tcp::1234  
	# then we load :
	# aarch64-none-linux-gnu-gdb build/kernel8.elf   
	# target remote :1234
	# and we can start debugging

clean:
	rm -r $(BUILDDIR)/*
	rm kernel8.img