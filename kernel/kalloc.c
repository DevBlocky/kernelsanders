#include "kernel.h"
#include "riscv.h"

static struct {
  usize begin;
  usize brk;
} kmem;

static void *ksbrk(isize bytes) {
  void *page;
  usize oldbrk = kmem.brk;

  kmem.brk += bytes;

  // allocate new pages (only runs if bytes > 0)
  for (usize addr = PGCEIL(oldbrk); addr < kmem.brk; addr += PGSIZE) {
    if (!(page = pgalloc()))
      panic("ksbrk pgalloc");
    kvmmap(addr, (usize)page, PGSIZE, PTE_R | PTE_W);
    printf("ksbrk alloc va=%hp pa=%hp\n", addr, page);
  }
  // deallocate pages (only runs if bytes < 0)
  for (usize addr = PGFLOOR(oldbrk); addr >= kmem.brk; addr -= PGSIZE) {
    // NOTE: kvmunmap takes an array for pa, but we are only unmapping one page
    kvmunmap(addr, (usize *)&page, PGSIZE);
    pgfree(page);
    printf("ksbrk dealloc va=%hp pa=%hp\n", addr, page);
  }

  return (void *)oldbrk;
}

struct block {
  usize magic;
  usize size;
  struct block *next;
};
static struct block *freelist;

#define MAGIC 0xabcd0123
#define CHECKMAGIC(b) assert((b)->magic == MAGIC)

#define METASIZE sizeof(struct block)
#define PTRSIZE sizeof(void *)
#define ALIGNPTR(x) (((x) + (PTRSIZE - 1)) & ~(PTRSIZE - 1))

// create a new block of size by extending the heap
static struct block *makeblock(usize size) {
  assert(size >= METASIZE);
  struct block *b = (struct block *)ksbrk(size);
  b->magic = MAGIC;
  b->size = size;
  b->next = NULL;
  return b;
}

// split the mem block into two
static BOOL split(struct block *b, usize fit) {
  assert(b->size >= fit); // sanity check
  if (b->size < fit + METASIZE + PTRSIZE)
    return FALSE; // not enough room

  struct block *split = (struct block *)((usize)b + fit);
  split->magic = MAGIC;
  split->size = b->size - fit;
  split->next = b->next;
  b->size = fit;
  b->next = split;
  return TRUE;
}

// search for the predecesor block of b in the freelist
// i.e. this is where b should be inserted into the freelist
static struct block *search(struct block *b, struct block **prev) {
  struct block *cur = freelist, *last = NULL, *last2 = NULL;
  while (cur && (usize)cur < (usize)b) {
    last2 = last;
    last = cur;
    cur = cur->next;
    // cur == last if double free (sometimes)
    assert(cur != last);
  }
  if (prev)
    *prev = last2;
  return last;
}

// returns if a and b are adjacent (contiguous in memory)
static inline BOOL adjacent(const struct block *a, const void *b) {
  return (usize)a + a->size == (usize)b;
}
// merge b and b->next if they are adjacent
static BOOL coalesce(struct block *b) {
  // if b->next is adjacent to b
  if (adjacent(b, b->next)) {
    b->size += b->next->size;
    b->next = b->next->next;
    return TRUE;
  }
  return FALSE;
}

void *kmalloc(usize size) {
  usize blocksz = ALIGNPTR(size + METASIZE);

  // look for an available block in the freelist
  struct block *cur = freelist, *last = NULL;
  while (cur && cur->size < blocksz) {
    CHECKMAGIC(cur);
    last = cur;
    cur = cur->next;
  }

  // split block if it has enough space for another block
  if (cur)
    split(cur, blocksz);

  if (!cur) // if no block is found, allocate a new one
    cur = makeblock(blocksz);
  else if (last) // otherwise remove the block from the freelist
    last->next = cur->next;
  else
    freelist = cur->next;
  return (void *)((usize)cur + METASIZE);
}

static void freeblock(struct block *b) {
  struct block *find, *prev, **next;
  find = search(b, &prev);
  next = find ? &find->next : &freelist;

  // insert the block
  b->next = *next;
  *next = b;
  // merge adj blocks
  coalesce(b);
  if (find && coalesce(find)) {
    next = prev ? &prev->next : &freelist;
    b = find;
  }

  // unalloc ksbrk if b is the last block
  if (!b->next && (usize)b + b->size == (usize)ksbrk(0)) {
    ksbrk(-b->size);
    *next = NULL;
  }
}
void kfree(void *ptr) {
  struct block *b = (struct block *)((usize)ptr - METASIZE);
  CHECKMAGIC(b);
  freeblock(b);
}

void *krealloc(void *ptr, usize size) {
  struct block *b = (struct block *)((usize)ptr - METASIZE);
  usize newsz = ALIGNPTR(size + METASIZE);
  isize diff = newsz - b->size;
  CHECKMAGIC(b);

  // if the block is at the end of the heap, extend/contract the heap
  if (adjacent(b, ksbrk(0))) {
    ksbrk(diff);
    b->size = newsz;
    return ptr;
  }

  // if we're shrinking the block, split and put remainder in freelist
  if (diff <= 0) {
    // if split, insert new block into freelist
    if (split(b, newsz))
      freeblock(b->next);
    return ptr;
  }

  struct block *find = search(b, NULL);
  struct block **next = find ? &find->next : &freelist;
  // find the block after this one in memory
  if (*next && adjacent(b, *next) && (*next)->size >= diff) {
    // remove *next from freelist
    split(*next, diff);
    *next = (*next)->next;
    // the block we removed is adjacent to b, so just increase b->size
    b->size = newsz;
    return ptr;
  }

  // we've done everything we can, so now just allocate a new block and copy
  void *dest = kmalloc(newsz);
  memcpy(dest, ptr, b->size - METASIZE);
  kfree(ptr);
  return dest;
}

void kallocinit(void) {
  // start of kernel heap (in virtual memory)
  kmem.begin = kmem.brk =
      PGCEIL((usize)sysmem.memstart + sysmem.memsz + PGSIZE);
  freelist = NULL;
}
