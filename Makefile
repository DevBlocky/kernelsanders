K=kernel

OBJS = \
	$K/entry.o \
	$K/mstart.o \
	$K/main.o \
	$K/traptimer.o \
	$K/printf.o \
	$K/pci.o \
	$K/vga.o \
	$K/picturedata.o
LDSCRIPT = $K/kernel.ld
OUTELF = $K/kernel.elf

TOOLPREFIX=riscv64-unknown-elf-

CC = $(TOOLPREFIX)gcc
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -Wall -O -ggdb \
	-MD -mcmodel=medany -nostdlib \
	-fno-omit-frame-pointer -fno-stack-protector \
	-ffreestanding


QEMU = qemu-system-riscv64
QEMUOPTS = -M virt -bios none -kernel $(OUTELF) -m 128M -smp 1 \
	-device bochs-display -serial stdio

elf: $(OUTELF)

$(OUTELF): $(LDSCRIPT) $(OBJS)
	$(LD) $(LDFLAGS) -T $< -o $@ $(OBJS)
	$(OBJDUMP) -S $@ > $@.asm

objdump: $(OUTELF)
	$(OBJDUMP) -d $< > $<.asm


clean:
	rm -f */*.o */*.d */*.asm $(OUTELF) $(OUTBIN)

qemu: $(OUTELF)
	@echo "use CTRL+A X to exit" 1>&2
	$(QEMU) $(QEMUOPTS)

qemu-gdb: $(OUTELF)
	@echo "use 'gdb' in another terminal"
	$(QEMU) $(QEMUOPTS) -S -s

dumpdtb:
	$(QEMU) $(QEMUOPTS) -machine dumpdtb=qemu.dtb
