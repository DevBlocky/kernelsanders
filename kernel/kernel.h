#ifndef __KERNEL_H
#define __KERNEL_H

#include "types.h"

// panic.S
void _panic(const char *s);
inline static void panic(const char *s)
{
    // assembly used to force "call _panic"
    // otherwise compiler might optimize with "j _panic"
    // and ra won't contain the calling pc
    asm(
        "mv a0, %0\n"
        "call _panic\n"
        :
        : "r"(s)
        : "a0");
}

// printf.c
void uartinit(void);
void uartintr(void);
void printf(const char *format, ...);

// pgalloc.c
extern int alloc;
extern int allocmax;

void allocinit(void);
void pgfree(void *page);
void *pgalloc(void);
void memset(void *ptr, usize_t val, usize_t size);
void memcpy(void *dst, void *src, usize_t size);

// vm.c
void kvminit(void);

// trap.c
void trapinit(void);

// pci.c
#define PCI_CMD_IOSPACE (1 << 0)
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

    uint32_t padd[5];
    uint8_t intr_line, intr_pin;
};
typedef volatile struct pci_device *pci_device_t;

void pci_enum_begin(struct pci_iterator *iter);
BOOL pci_enum_next(struct pci_iterator *iter, pci_device_t *device);

// vga.c
void vgainit(void);
void vgasetfb(uint8_t *fb, usize_t size);

#endif // __KERNEL_H
