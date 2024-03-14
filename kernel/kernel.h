#ifndef __KERNEL_H
#define __KERNEL_H

#include "types.h"

// panic.S
extern BOOL panicking;

__attribute((noreturn)) void _panic(const char *s);
inline static __attribute__((noreturn)) void panic(const char *s) {
  // assembly used to force "call _panic"
  // otherwise compiler might optimize with "j _panic"
  // and ra won't contain the calling pc
  asm("mv a0, %0\n"
      "call _panic\n"
      :
      : "r"(s)
      : "a0");
  for (;;)
    ;
}
#define assert(b) ((b) ? (void)0 : panic("assert failed: " #b))

// util.c
void memset(void *ptr, usize val, usize size);
void memcpy(void *dst, void *src, usize size);
usize strlen(const char *c);
int strcmp(const char *a, const char *b);
u32 be2cpu32(u32 be);
u64 be2cpu64(u64 be);

// devtree.c
usize dtgetmmio(const char *compat, void **addr);

// printf.c
void uartinit(void);
void uartintr(void);
void printf(const char *format, ...);

// sysmem.c
struct sysmeminfo {
  void *memstart;
  usize memsz;
  usize alloc, allocmax;
};
extern struct sysmeminfo sysmem;
void sysmeminit(void);
void pgfree(void *page);
void *pgalloc(void);

// vmem.c
#define PTE_V (1 << 0)
#define PTE_R (1 << 1)
#define PTE_W (1 << 2)
#define PTE_X (1 << 3)
#define PTE_U (1 << 4)

void kvminit(void);
void kvmmap(usize vaddr, usize paddr, usize size, int perm);
void kvmunmap(usize vaddr, usize *paddr, usize size);
void kvmuse(void);

// kalloc.c
void kallocinit(void);
void *kmalloc(usize size);
void kfree(void *ptr);

// trap.c
void trapinit(void);

// pci.c
#define PCI_CMD_IOSPACE (1 << 0)
#define PCI_CMD_MEMSPACE (1 << 1)

struct pci_iterator {
  u16 bus, slot, func;
};

struct __attribute((packed)) pci_device {
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
