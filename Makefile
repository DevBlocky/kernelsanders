K=kernel

OBJS = \
	$K/entry.o \
	$K/panic.o \
	$K/traptimer.o \
	$K/trapkernel.o \
	$K/mstart.o \
	$K/main.o \
	$K/util.o \
	$K/devtree.o \
	$K/printf.o \
	$K/sysmem.o \
	$K/vmem.o \
	$K/kalloc.o \
	$K/trap.o \
	$K/virtio_blk.o \
	$K/pci.o \
	$K/vga.o
LDSCRIPT = $K/kernel.ld
OUTELF = $K/kernel.elf

TOOLPREFIX=riscv64-unknown-elf-

NCC = clang # native cc
CC = $(TOOLPREFIX)gcc
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -Wall -ggdb -MD -mcmodel=medany \
	-nostdlib -ffreestanding -O3

.PHONY: all elf objdump clean qemu qemu-gdb qemu-dtc
all: elf

mkfs/mkfs: mkfs/mkfs.c
	$(NCC) -Werror -Wall -o $@ $<
fs.img: colonel.jpg mkfs/mkfs
	magick $< -resize 640x480\! BMP3:- | mkfs/mkfs $@


QEMU = qemu-system-riscv64
QEMUOPTS = -M virt -bios none -kernel $(OUTELF) -m 128M -smp 1 \
	-device VGA -serial stdio
QEMUOPTS += -global virtio-mmio.force-legacy=false
QEMUOPTS += -drive id=blk0,file=fs.img,if=none,format=raw \
			-device virtio-blk-device,drive=blk0,bus=virtio-mmio-bus.0

elf: $(OUTELF)
$(OUTELF): $(LDSCRIPT) $(OBJS) fs.img
	$(LD) $(LDFLAGS) -T $< -o $@ $(OBJS)
	$(OBJDUMP) -S $@ > $@.asm

objdump: $(OUTELF)
	$(OBJDUMP) -d $< > $<.asm
clean:
	rm -f */*.o */*.d */*.asm */*codegen.c fs.img mkfs/mkfs $(OUTELF) $(OUTBIN)

qemu: $(OUTELF) fs.img
	$(QEMU) $(QEMUOPTS)
qemu-gdb: $(OUTELF)
	@echo "use 'gdb' in another terminal"
	$(QEMU) $(QEMUOPTS) -S -s
qemu-dtc:
	$(QEMU) $(QEMUOPTS) -machine dumpdtb=qemu.dtb
	dtc qemu.dtb > qemu.dtc
