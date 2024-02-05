#include "kernel.h"
#include "riscv.h"

struct run
{
    struct run *next;
};
struct run *freelist;

int alloc;
int allocmax;

// free a page created with pgalloc()
void pgfree(void *page)
{
    struct run *r;
    usize_t addr = (usize_t)page;

    // make sure it's a valid page address
    if (addr % PGSIZE != 0 || addr < (usize_t)kend || addr >= (usize_t)PHYSTOP)
        panic("page free");

    // add this page to the freelist
    r = (struct run *)page;
    r->next = freelist;
    freelist = r;
    alloc--;
}

// allocate 4096 bytes on a page boundary
void *pgalloc(void)
{
    if (!freelist)
        return 0;
    void *page = (void *)freelist;
    freelist = freelist->next;
    alloc++;
    return page;
}

void allocinit(void)
{
    freelist = NULL;
    uint32_t n = 0;

    // "free" all available pages of physical memory
    // (i.e. from the end of kernel data to PHYSTOP)
    void *p = (void *)PGCEIL((usize_t)kend);
    for (; p + PGSIZE <= (void *)PHYSTOP; p += PGSIZE, n++)
        pgfree(p);

    allocmax = -alloc;
    alloc = 0;
    printf("mem pages: %u\n", n);
}

void memset(void *ptr, usize_t val, usize_t size)
{
    // set usize_t bytes at a time
    // usize_t so the compiler can use registers
    usize_t *dst = (usize_t *)ptr;
    for (usize_t i = 0; i < size / sizeof(usize_t); i++)
        *dst++ = val;

    // if size isn't a multiple of sizeof(usize_t), we need
    // to set the remaining bytes
    uint8_t *dst2 = (uint8_t *)dst;
    for (usize_t i = 0; i < size % sizeof(usize_t); i++)
        *dst2++ = ((uint8_t *)&val)[i];
}
void memcpy(void *dst, void *src, usize_t size)
{
    usize_t *ldst = dst, *lsrc = src;
    for (usize_t i = 0; i < size / sizeof(usize_t); i++)
        *ldst++ = *lsrc++;

    uint8_t *sdst = (uint8_t*)ldst, *ssrc = (uint8_t*)lsrc;
    for (usize_t i = 0; i < size % sizeof(usize_t); i++)
        *sdst++ = *ssrc++;
}
