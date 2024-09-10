CC = aarch64-none-linux-gnu-gcc
AS = aarch64-none-linux-gnu-as
OBJCOPY = aarch64-none-linux-gnu-objcopy
CFLAGS =  -O0 -ffreestanding -nostdlib -nostartfiles -ggdb -mno-outline-atomics
#CFLAGS += -Wall
QEMU = qemu-system-aarch64

KERNELDIR = kernel
BUILDDIR = build

SRCS_C = $(wildcard $(KERNELDIR)/*.c)
SRCS_S = $(wildcard $(KERNELDIR)/*.S)
OBJS = $(patsubst $(KERNELDIR)/*.c,  $(BUILDDIR)/*.o,  $(SRCS_C))
OBJS += $(patsubst $(KERNELDIR)/*.S,  $(BUILDDIR)/*.o,  $(SRCS_S))

SRCS_U = user/cat.c user/init.c
OBJS_U = rootfs/cat rootfs/init

ROOTFS := $(wildcard rootfs/*)

all: kernel8.img rootfs.cpio

.PHONY: run clean

$(BUILDDIR)/%.o: $(KERNELDIR)/%.S
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $^

$(BUILDDIR)/%.o: $(KERNELDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $^

$(OBJS_U): $(SRCS_U)
	$(CC) $(CFLAGS)  -T user/user.ld  $< user/libc.c -o $@    

kernel8.img:  $(OBJS)   $(KERNELDIR)/link.ld
	$(CC) $(CFLAGS)  $(OBJS) -T $(KERNELDIR)/link.ld -o $(BUILDDIR)/kernel8.elf
	$(OBJCOPY) $(BUILDDIR)/kernel8.elf -O binary kernel8.img

# QEMU loads the cpio archive file to 0x8000000 by default.
rootfs.cpio: $(ROOTFS)
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
	rm kernel8.img
	rm -r $(BUILDDIR)/*
	rm rootfs.cpio