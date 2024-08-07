CC = aarch64-none-linux-gnu-gcc
AS = aarch64-none-linux-gnu-as
OBJCOPY = aarch64-none-linux-gnu-objcopy
CFLAGS = -Wall -O2 -ffreestanding -nostdlib -nostartfiles

SRCDIR = kernel
BUILDDIR = build

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/*.c,  $(BUILDDIR)/*.o,  $(SRCS))

all: kernel8.img

.PHONY: run clean

$(BUILDDIR)/boot.o: $(SRCDIR)/boot.S
	@mkdir -p $(BUILDDIR)
	$(AS) -c $(SRCDIR)/boot.S -o $(BUILDDIR)/boot.o

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $^

kernel8.img: $(BUILDDIR)/boot.o $(OBJS)   $(SRCDIR)/linker.ld
	$(CC) $(CFLAGS) $(BUILDDIR)/boot.o  $(OBJS) -T $(SRCDIR)/linker.ld -o $(BUILDDIR)/kernel8.elf
	$(OBJCOPY) $(BUILDDIR)/kernel8.elf -O binary kernel8.img

run: kernel8.img 
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial stdio 

clean:
	rm -r $(BUILDDIR)
	rm kernel8.img