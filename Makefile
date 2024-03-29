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
	$K/pci.o \
	$K/vga.o \
	$K/picturedata_codegen.o
LDSCRIPT = $K/kernel.ld
OUTELF = $K/kernel.elf

TOOLPREFIX=riscv64-unknown-elf-

CC = $(TOOLPREFIX)gcc
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -Wall -ggdb -MD -mcmodel=medany \
	-nostdlib -ffreestanding


QEMU = qemu-system-riscv64
QEMUOPTS = -M virt -bios none -kernel $(OUTELF) -m 128M -smp 1 \
	-device VGA -serial stdio

elf: $(OUTELF)

$(OUTELF): $(LDSCRIPT) $(OBJS)
	$(LD) $(LDFLAGS) -T $< -o $@ $(OBJS)
	$(OBJDUMP) -S $@ > $@.asm

$K/picturedata_codegen.c: colonel.jpg
	magick convert $< -resize 640x480\! BGR:- | xxd -i -n picturedata > $@

objdump: $(OUTELF)
	$(OBJDUMP) -d $< > $<.asm


clean:
	rm -f */*.o */*.d */*.asm */*codegen.c $(OUTELF) $(OUTBIN)

qemu: $(OUTELF)
	$(QEMU) $(QEMUOPTS)

qemu-gdb: $(OUTELF)
	@echo "use 'gdb' in another terminal"
	$(QEMU) $(QEMUOPTS) -S -s

qemu-dtc:
	$(QEMU) $(QEMUOPTS) -machine dumpdtb=qemu.dtb
	dtc qemu.dtb > qemu.dtc
