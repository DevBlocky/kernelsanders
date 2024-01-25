#include "vm.h"
#include "types.h"
#include "riscv.h"
#include "pgalloc.h"
#include "printf.h"

typedef uint64_t *pagetable_t;
typedef uint64_t pte_t;

#define PTE_V (1 << 0)
#define PTE_R (1 << 1)
#define PTE_W (1 << 2)
#define PTE_X (1 << 3)
#define PTE_U (1 << 4)

#define SATP_SV39 (8L << 60)

// convert a physical address into a page table entry
static inline pte_t pa2pte(usize_t pa)
{
    return (uint64_t)(pa >> 12) << 10;
}
// convert a page table entry into a physical address
static inline usize_t pte2pa(pte_t pte)
{
    return (usize_t)(pte >> 10) << 12;
}
// convert a virtual address into an index in a page table
static inline usize_t va2idx(usize_t level, usize_t va)
{
    // select the 9-bit level index
    return (va >> (12 + level * 9)) & 0x1ff;
}

pagetable_t kpagetable;

// find a leaf entry in a pagetable
// if alloc == TRUE, allocate required pages
pte_t *vmwalk(pagetable_t tbl, usize_t vaddr, BOOL alloc)
{
    for (usize_t level = 2; level > 0; level--)
    {
        pte_t *pte = &tbl[va2idx(level, vaddr)];
        if (*pte & PTE_V)
        {
            tbl = (pagetable_t)pte2pa(*pte);
        }
        else
        {
            if (!alloc || !(tbl = (pagetable_t)pgalloc()))
                return 0;
            memset(tbl, 0, PGSIZE);
            *pte = pa2pte((usize_t)tbl) | PTE_V;
        }
    }
    return &tbl[va2idx(0, vaddr)];
}

// map a virtual address to a physical address in a pagetable
int vmmap(pagetable_t tbl, usize_t vaddr, usize_t paddr, usize_t size, int perm)
{
    usize_t vaddrend = PGFLOOR(vaddr + size - 1);
    vaddr = PGFLOOR(vaddr);

    for (; vaddr <= vaddrend; vaddr += PGSIZE, paddr += PGSIZE)
    {
        pte_t *pte = vmwalk(tbl, vaddr, TRUE);
        if (pte == NULL)
            return -1;
        *pte = pa2pte(paddr) | perm | PTE_V;
    }
    return 0;
}
// map a virtual address for the kernel page table
void kvmmap(usize_t vaddr, usize_t paddr, usize_t size, int perm)
{
    if (vmmap(kpagetable, vaddr, paddr, size, perm) != 0)
        panic("kvmmap");
}

void vmuse(pagetable_t tbl)
{
    sfence_vma();
    w_satp(((usize_t)tbl >> 12) | SATP_SV39);
    sfence_vma();
}
void kvmuse(void)
{
    vmuse(kpagetable);
}

void kvminit(void)
{
    kpagetable = (pagetable_t)pgalloc();
    memset(kpagetable, 0, PGSIZE);

    // kernel code
    kvmmap((usize_t)kstart, (usize_t)kstart, (usize_t)(ktextend - kstart), PTE_R | PTE_X);
    // give r/w to rest of RAM
    kvmmap((usize_t)ktextend, (usize_t)ktextend, (usize_t)(PHYSTOP - ktextend), PTE_R | PTE_W);

    // serial io
    kvmmap(UART0_MMIO, UART0_MMIO, PGSIZE, PTE_R | PTE_W);
    // pci mmio config
    kvmmap(PCI_MMIO, PCI_MMIO, 0x10000000, PTE_R | PTE_W);
    // vga fb and mmio
    kvmmap(0x40000000, 0x40000000, 16 * 1024 * 1024 + PGSIZE, PTE_R | PTE_W);

    kvmuse();
}
