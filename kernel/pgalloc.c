#include "kernel.h"
#include "riscv.h"

struct run {
  struct run *next;
};
static struct run *freelist;

int alloc;
int allocmax;

// free a page created with pgalloc()
void pgfree(void *page) {
  struct run *r;
  usize addr = (usize)page;

  // make sure it's a valid page address
  if (addr % PGSIZE != 0 || addr < (usize)kend || addr >= (usize)PHYSTOP)
    panic("page free");

  // add this page to the freelist
  r = (struct run *)page;
  r->next = freelist;
  freelist = r;
  alloc--;
}

// allocate 4096 bytes on a page boundary
void *pgalloc(void) {
  if (!freelist)
    return 0;
  void *page = (void *)freelist;
  freelist = freelist->next;
  alloc++;
  return page;
}

void allocinit(void) {
  freelist = NULL;
  u32 n = 0;

  // "free" all available pages of physical memory
  // (i.e. from the end of kernel data to PHYSTOP)
  void *p = (void *)PGCEIL((usize)kend);
  for (; p + PGSIZE <= (void *)PHYSTOP; p += PGSIZE, n++)
    pgfree(p);

  allocmax = -alloc;
  alloc = 0;
  printf("mem pages: %u\n", n);
}

void memset(void *ptr, usize val, usize size) {
  // set usize bytes at a time
  // usize so the compiler can use registers
  usize *dst = (usize *)ptr;
  for (usize i = 0; i < size / sizeof(usize); i++)
    *dst++ = val;

  // if size isn't a multiple of sizeof(usize), we need
  // to set the remaining bytes
  u8 *dst2 = (u8 *)dst;
  for (usize i = 0; i < size % sizeof(usize); i++)
    *dst2++ = ((u8 *)&val)[i];
}
void memcpy(void *dst, void *src, usize size) {
  usize *ldst = dst, *lsrc = src;
  for (usize i = 0; i < size / sizeof(usize); i++)
    *ldst++ = *lsrc++;

  u8 *sdst = (u8 *)ldst, *ssrc = (u8 *)lsrc;
  for (usize i = 0; i < size % sizeof(usize); i++)
    *sdst++ = *ssrc++;
}
