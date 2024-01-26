#ifndef __KERNEL_H
#define __KERNEL_H

#include "types.h"

// printf.c
void printf(const char *format, ...);
void panic(const char *s);

// pgalloc.c
void allocinit(void);
void pgfree(void *page);
void *pgalloc(void);
void memset(void *ptr, usize_t val, usize_t size);

// vm.c
void kvminit(void);

// trap.c
void trapinit(void);

// pci.c
#define PCI_CMD_IOSPACE  (1 << 0)
#define PCI_CMD_MEMSPACE (1 << 1)

struct pci_iterator
{
    uint16_t bus, slot, func;
};

__attribute((packed)) struct pci_device
{
    // each row = 32bits
    uint16_t vendor_id, device_id;
    uint16_t command, status;
    uint8_t revision_id, prog_if, subclass, class;
    uint8_t cl_size, l_timer, header_type, bist;

    uint32_t bar[6];
};


void pci_enum_begin(struct pci_iterator *iter);
BOOL pci_enum_next(struct pci_iterator *iter, volatile struct pci_device **device);

// vga.c
#define VBE_DISPI_INDEX_ID              0x0
#define VBE_DISPI_INDEX_XRES            0x1
#define VBE_DISPI_INDEX_YRES            0x2
#define VBE_DISPI_INDEX_BPP             0x3
#define VBE_DISPI_INDEX_ENABLE          0x4
#define VBE_DISPI_INDEX_BANK            0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH      0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT     0x7
#define VBE_DISPI_INDEX_X_OFFSET        0x8
#define VBE_DISPI_INDEX_Y_OFFSET        0x9

#define VBE_DISPI_ID0                   0xB0C0
#define VBE_DISPI_ID1                   0xB0C1
#define VBE_DISPI_ID2                   0xB0C2
#define VBE_DISPI_ID3                   0xB0C3
#define VBE_DISPI_ID4                   0xB0C4

#define VBE_DISPI_DISABLED              0x00
#define VBE_DISPI_ENABLED               0x01
#define VBE_DISPI_VBE_ENABLED           0x40
#define VBE_DISPI_NOCLEARMEM            0x80

void vgainit(void);
void vga_lset(usize_t i, uint8_t data);

#endif // __KERNEL_H
 