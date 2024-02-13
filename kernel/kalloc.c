#include "kernel.h"
#include "riscv.h"

static struct {
  usize begin;
  usize brk;
} kmem;

static void *ksbrk(isize bytes) {
  void *page;
  usize oldbrk = kmem.brk;

  if (bytes < 0)
    panic("ksbrk todo");
  kmem.brk += bytes;

  for (usize addr = PGCEIL(oldbrk); addr < kmem.brk; addr += PGSIZE) {
    if (!(page = pgalloc()))
      panic("ksbrk pgalloc");
    kvmmap(addr, (usize)page, PGSIZE, PTE_R | PTE_W);
    printf("ksbrk alloc va=%hp pa=%hp\n", addr, page);
  }

  return (void *)oldbrk;
}

struct block {
  usize size;
  struct block *next;
};
static struct block *freelist;

#define METASIZE sizeof(struct block)
#define PTRSIZE sizeof(usize)
#define ALIGNPTR(x) (((x) + (PTRSIZE - 1)) & ~(PTRSIZE - 1))

// create a new block of size by extending the heap
static struct block *makeblock(usize size) {
  assert(size >= METASIZE);
  struct block *b = (struct block *)ksbrk(size);
  b->next = NULL;
  b->size = size;
  return b;
}

// split the mem block into two
static void split(struct block *b, usize fit) {
  assert(fit <= b->size + METASIZE);

  struct block *split = (struct block *)((usize)b + fit);
  split->next = b->next;
  split->size = b->size - fit;
  b->next = split;
  b->size = fit;
}

// search for the predecesor block of b in the freelist
// i.e. this is where b should be inserted into the freelist
static struct block *search(struct block *b) {
  struct block *cur = freelist, *last = NULL;
  while (cur && (usize)cur < (usize)b) {
    last = cur;
    cur = cur->next;
  }
  return last;
}

// merge b and b->next if they are adjacent
static void coalesce(struct block *b) {
  // while b->next is adjacent to b
  while ((usize)b->next == (usize)b + b->size) {
    b->size += b->next->size;
    b->next = b->next->next;
  }
}

void *kmalloc(usize size) {
  usize blocksz = ALIGNPTR(size + METASIZE);

  // look for an available block in the freelist
  struct block *cur = freelist, *last = NULL;
  while (cur && cur->size < blocksz) {
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
  struct block *b = (struct block *)((usize)ptr - METASIZE);
  struct block *find = search(b);
  struct block **next = find ? &find->next : &freelist;

  // TODO: double free check?

  // insert the block
  b->next = *next;
  *next = b;
  // merge adj blocks
  coalesce(b);
  if (find)
    coalesce(find);
}

void kallocinit(void) {
  // start of kernel heap (in virtual memory)
  kmem.begin = kmem.brk = PGCEIL((usize)PHYSTOP + PGSIZE);
  freelist = NULL;
}
