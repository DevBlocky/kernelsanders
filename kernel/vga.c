#include "kernel.h"

#define VBE_DISPI_INDEX_ID 0x0
#define VBE_DISPI_INDEX_XRES 0x1
#define VBE_DISPI_INDEX_YRES 0x2
#define VBE_DISPI_INDEX_BPP 0x3
#define VBE_DISPI_INDEX_ENABLE 0x4
#define VBE_DISPI_INDEX_BANK 0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH 0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT 0x7
#define VBE_DISPI_INDEX_X_OFFSET 0x8
#define VBE_DISPI_INDEX_Y_OFFSET 0x9

#define VBE_DISPI_ID0 0xB0C0
#define VBE_DISPI_ID1 0xB0C1
#define VBE_DISPI_ID2 0xB0C2
#define VBE_DISPI_ID3 0xB0C3
#define VBE_DISPI_ID4 0xB0C4

#define VBE_DISPI_DISABLED 0x00
#define VBE_DISPI_ENABLED 0x01
#define VBE_DISPI_VBE_ENABLED 0x40
#define VBE_DISPI_NOCLEARMEM 0x80

// both of these must fit to u32 space because of PCI BARs
#define VGA_FB ((volatile u8 *)0x40000000)
#define VGA_MMIO (VGA_FB + (16 * 1024 * 1024))

#define VGAIO(reg) (VGA_MMIO + (reg) + (0x400 - 0x3c0))
#define BOCHSIO(idx) (volatile u16 *)(VGA_MMIO + 0x500 + ((idx) << 1))

void vgainit(void)
{
    pci_device_t device;
    struct pci_iterator iter;
    pci_enum_begin(&iter);
    while (pci_enum_next(&iter, &device) && device->class != 0x3)
        ;
    if (device->vendor_id == 0x0 || device->vendor_id == 0xffff || device->class != 0x3)
        panic("find display device");

    // set the addresses of the frame-buffer and the vga io
    device->bar[0] = (u32)(u64)VGA_FB;
    device->bar[2] = (u32)(u64)VGA_MMIO;
    device->command = PCI_CMD_IOSPACE | PCI_CMD_MEMSPACE;

    // enable ram and color output
    *VGAIO(0x3c2) = (1 << 0) | (1 << 1);
    // stop blanking
    *VGAIO(0x3c0) = 0x20;

    // init the display through bochs VBE extensions
    *BOCHSIO(VBE_DISPI_INDEX_ENABLE) = VBE_DISPI_DISABLED;
    *BOCHSIO(VBE_DISPI_INDEX_XRES) = 640; // width
    *BOCHSIO(VBE_DISPI_INDEX_YRES) = 480; // height
    *BOCHSIO(VBE_DISPI_INDEX_BPP) = 24;   // bpp
    *BOCHSIO(VBE_DISPI_INDEX_ENABLE) = VBE_DISPI_ENABLED | VBE_DISPI_VBE_ENABLED;
    printf("display initialized with bochs VBE\n");
}

void vgasetfb(u8 *fb, usize size)
{
    memcpy((u8 *)VGA_FB, fb, size);
}
