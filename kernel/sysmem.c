#include "kernel.h"
#include "riscv.h"

struct run {
  struct run *next;
};
static struct run *freelist;
struct sysmeminfo sysmem;

static inline BOOL overlaps(usize start1, usize end1, usize start2,
                            usize end2) {
  return start1 < end2 && start2 < end1;
}
static inline BOOL pgoverlaps(usize pg, usize start, usize end) {
  return overlaps(pg, pg + PGSIZE, start, end);
}

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
  usize memstart, memend, dtstart, dtend;
  freelist = NULL;

  // get memory info from device tree
  dtmem(&dtstart, &dtend);
  sysmem.memsz = dtgetmmio("memory", &sysmem.memstart);
  assert(sysmem.memsz != 0);

  memstart = (usize)sysmem.memstart;
  memend = memstart + sysmem.memsz;
  int n = 0;
  // loop through all memory pages
  for (usize p = PGCEIL(memstart); p + PGSIZE <= memend; p += PGSIZE) {
    // skip kernel memory and device tree memory
    if (pgoverlaps(p, (usize)&kstart, (usize)&kend) ||
        pgoverlaps(p, dtstart, dtend))
      continue;
    pgfree((void *)p);
    n++;
  }

  sysmem.allocmax = n;
  sysmem.alloc = 0;
  printf("mem pages: %u\n", sysmem.allocmax);
}
