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
void memset(void *ptr, usize val, usize size);
void memcpy(void *dst, void *src, usize size);

// vm.c
void kvminit(void);

// trap.c
void trapinit(void);

// pci.c
#define PCI_CMD_IOSPACE (1 << 0)
#define PCI_CMD_MEMSPACE (1 << 1)

struct pci_iterator
{
    u16 bus, slot, func;
};

__attribute((packed)) struct pci_device
{
    // each row = 32bits
    u16 vendor_id, device_id;
    u16 command, status;
    u8 revision_id, prog_if, subclass, class;
    u8 cl_size, l_timer, header_type, bist;

    u32 bar[6];

    u32 padd[5];
    u8 intr_line, intr_pin;
};
typedef volatile struct pci_device *pci_device_t;

void pci_enum_begin(struct pci_iterator *iter);
BOOL pci_enum_next(struct pci_iterator *iter, pci_device_t *device);

// vga.c
void vgainit(void);
void vgasetfb(u8 *fb, usize size);

#endif // __KERNEL_H
