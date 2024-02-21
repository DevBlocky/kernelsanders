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
#define PTRSIZE sizeof(usize)
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
static void split(struct block *b, usize fit) {
  assert(fit <= b->size + METASIZE);

  struct block *split = (struct block *)((usize)b + fit);
  split->magic = MAGIC;
  split->size = b->size - fit;
  split->next = b->next;
  b->size = fit;
  b->next = split;
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

// merge b and b->next if they are adjacent
static BOOL coalesce(struct block *b) {
  // while b->next is adjacent to b
  if ((usize)b->next == (usize)b + b->size) {
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
  if (cur && cur->size > blocksz + METASIZE + PTRSIZE)
    split(cur, blocksz);

  if (!cur) // if no block is found, allocate a new one
    cur = makeblock(blocksz);
  else if (last) // otherwise remove the block from the freelist
    last->next = cur->next;
  else
    freelist = cur->next;

  return (void *)((usize)cur + METASIZE);
}
void kfree(void *ptr) {
  struct block *find, *prev, **next;
  struct block *b = (struct block *)((usize)ptr - METASIZE);
  CHECKMAGIC(b);

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

void kallocinit(void) {
  // start of kernel heap (in virtual memory)
  kmem.begin = kmem.brk = PGCEIL((usize)PHYSTOP + PGSIZE);
  freelist = NULL;
}
