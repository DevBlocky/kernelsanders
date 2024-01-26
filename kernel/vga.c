#include "kernel.h"

// both of these must fit to uint32_t space because of PCI BARs
#define VGA_FB ((volatile uint8_t *)0x40000000)
#define VGA_MMIO (VGA_FB + (16 * 1024 * 1024))

#define VGAIO(reg) (VGA_MMIO + reg + (0x400 - 0x3c0))
#define BOCHSIO(idx) (volatile uint16_t *)(VGA_MMIO + 0x500 + (idx << 1))

void vgainit(void)
{
    volatile struct pci_device *device;
    struct pci_iterator iter;
    pci_enum_begin(&iter);
    while (pci_enum_next(&iter, &device) && device->class != 0x3)
        ;
    if (device->vendor_id == 0x0 || device->vendor_id == 0xffff || device->class != 0x3)
        panic("find display device");

    // set the addresses of the frame-buffer and the vga io
    device->bar[0] = (uint32_t)(uint64_t)VGA_FB;
    device->bar[2] = (uint32_t)(uint64_t)VGA_MMIO;
    device->command = PCI_CMD_IOSPACE | PCI_CMD_MEMSPACE;

    // if this is a VGA interface, we need to stop blanking
    *VGAIO(0x3c0) = 0x20;

    // init the display through bochs VBE extensions
    *BOCHSIO(VBE_DISPI_INDEX_ENABLE) = 0;
    *BOCHSIO(VBE_DISPI_INDEX_XRES) = 640; // width
    *BOCHSIO(VBE_DISPI_INDEX_YRES) = 480; // height
    *BOCHSIO(VBE_DISPI_INDEX_BPP) = 32;   // bpp
    *BOCHSIO(VBE_DISPI_INDEX_ENABLE) = VBE_DISPI_ENABLED | VBE_DISPI_VBE_ENABLED | VBE_DISPI_NOCLEARMEM;
    printf("bochs display initialized\n");
}

void vga_lset(usize_t i, uint8_t data)
{
    VGA_FB[i] = data;
}
