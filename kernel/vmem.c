#include "kernel.h"
#include "riscv.h"

typedef u64 *pagetable_t;
typedef u64 pte_t;

#define SATP_SV39 (8L << 60)

// convert a physical address into a page table entry
static inline pte_t pa2pte(usize pa) { return (pte_t)(pa >> 12) << 10; }
// convert a page table entry into a physical address
static inline usize pte2pa(pte_t pte) { return (usize)(pte >> 10) << 12; }
// convert a virtual address into an index in a page table
static inline usize va2idx(usize level, usize va) {
  // select the 9-bit level index
  return (va >> (12 + level * 9)) & 0x1ff;
}

pagetable_t kpagetable;

// find a leaf entry in a pagetable
// if alloc == TRUE, allocate required pages
pte_t *vmwalk(pagetable_t tbl, usize vaddr, BOOL alloc) {
  for (usize level = 2; level > 0; level--) {
    pte_t *pte = &tbl[va2idx(level, vaddr)];
    if (*pte & PTE_V) {
      tbl = (pagetable_t)pte2pa(*pte);
    } else {
      if (!alloc || !(tbl = (pagetable_t)pgalloc()))
        return NULL;
      memset(tbl, 0, PGSIZE);
      *pte = pa2pte((usize)tbl) | PTE_V;
    }
  }
  return &tbl[va2idx(0, vaddr)];
}

// map a virtual address to a physical address in a pagetable
int vmmap(pagetable_t tbl, usize vaddr, usize paddr, usize size, int perm) {
  usize vaddrend = PGFLOOR(vaddr + size - 1);
  vaddr = PGFLOOR(vaddr);

  for (; vaddr <= vaddrend; vaddr += PGSIZE, paddr += PGSIZE) {
    pte_t *pte = vmwalk(tbl, vaddr, TRUE);
    if (pte == NULL)
      return -1;
    *pte = pa2pte(paddr) | perm | PTE_V;
  }
  return 0;
}
// map a virtual address for the kernel page table
void kvmmap(usize vaddr, usize paddr, usize size, int perm) {
  if (vmmap(kpagetable, vaddr, paddr, size, perm) != 0)
    panic("kvmmap");
}

void vmunmap(pagetable_t tbl, usize vaddr, usize *paddr, usize size) {
  usize vaddrend = PGFLOOR(vaddr + size - 1);
  vaddr = PGFLOOR(vaddr);

  for (int i = 0; vaddr <= vaddrend; vaddr += PGSIZE, i++) {
    pte_t *pte = vmwalk(tbl, vaddr, FALSE);
    if (!pte)
      continue;
    if (paddr)
      paddr[i] = *pte | PTE_V ? pte2pa(*pte) : 0;
    *pte = 0;
  }
}
void kvmunmap(usize vaddr, usize *paddr, usize size) {
  vmunmap(kpagetable, vaddr, paddr, size);
}

void vmuse(pagetable_t tbl) {
  sfence_vma();
  w_satp(((usize)tbl >> 12) | SATP_SV39);
  sfence_vma();
}
void kvmuse(void) { vmuse(kpagetable); }

void kvminit(void) {
  kpagetable = (pagetable_t)pgalloc();
  memset(kpagetable, 0, PGSIZE);

  // give r/w to all of RAM
  kvmmap((usize)sysmem.memstart, (usize)sysmem.memstart, sysmem.memsz,
         PTE_R | PTE_W);
  // change r/x to kernel code
  // the kernel text should be page aligned so the page entry
  // doesn't bleed into other RAM
  kvmmap((usize)&kstart, (usize)&kstart, (usize)&ktextend - (usize)&kstart,
         PTE_R | PTE_X);

  // mmio virutal memory regions
  // clint
  kvmmap(CLINT_MMIO, CLINT_MMIO, 0x10000, PTE_R);
  // plic
  kvmmap(PLIC_MMIO, PLIC_MMIO, 0x600000, PTE_R | PTE_W);
  // serial io
  kvmmap(UART0_MMIO, UART0_MMIO, PGSIZE, PTE_R | PTE_W);
  // pci mmio config
  kvmmap(PCI_MMIO, PCI_MMIO, 0x10000, PTE_R | PTE_W);
  // vga fb and mmio
  // ? assigning custom mmio region maps should be moved to a separate function
  // ? since vga.c determines the mmio address for the graphics card
  kvmmap(0x40000000, 0x40000000, 16 * 1024 * 1024 + PGSIZE, PTE_R | PTE_W);

  kvmuse();
}
