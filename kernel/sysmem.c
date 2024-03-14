#include "kernel.h"
#include "riscv.h"

struct run {
  struct run *next;
};
static struct run *freelist;

struct sysmeminfo sysmem;

// free a page created with pgalloc()
void pgfree(void *page) {
  struct run *r;
  usize addr = (usize)page;

  // make sure it's a valid page address
  assert((usize)page % PGSIZE == 0);
  assert(addr < (usize)&kstart || addr >= (usize)&kend);

  // add this page to the freelist
  r = (struct run *)page;
  r->next = freelist;
  freelist = r;
  sysmem.alloc--;
}

// allocate 4096 bytes on a page boundary
void *pgalloc(void) {
  if (!freelist)
    return NULL;
  void *page = (void *)freelist;
  freelist = freelist->next;
  sysmem.alloc++;
  return page;
}

void sysmeminit(void) {
  freelist = NULL;
  sysmem.memsz = dtgetmmio("memory", &sysmem.memstart);
  assert(sysmem.memsz != 0);

  usize memstart = (usize)sysmem.memstart;
  usize memend = memstart + sysmem.memsz;
  int n = 0;
  // loop through all memory pages
  for (usize p = PGCEIL(memstart); p + PGSIZE <= memend; p += PGSIZE) {
    // if this page is not part of the kernel memory
    if (p < (usize)&kstart || p >= (usize)&kend) {
      pgfree((void *)p);
      n++;
    }
  }

  sysmem.allocmax = n;
  sysmem.alloc = 0;
  printf("mem pages: %u\n", sysmem.allocmax);
}
