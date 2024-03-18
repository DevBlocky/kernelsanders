#include "kernel.h"

#define FDT_MAGIC_IDX 0
#define FDT_SIZE_IDX 1
#define FDT_STRUCTOFF_IDX 2
#define FDT_STRINGOFF_IDX 3
#define FDT_VERSION_IDX 5
#define FDT_MAGIC 0xd00dfeed
#define FDT_VERSION 17

static u32 *devtree;

static inline u32 dtgetprop(usize prop) { return be2cpu32(devtree[prop]); }
static char *dtgetstring(u32 stroff) {
  return (char *)((usize)devtree + dtgetprop(FDT_STRINGOFF_IDX) + stroff);
}

void dtmem(usize *start, usize *end) {
  *start = (usize)devtree;
  *end = *start + dtgetprop(FDT_SIZE_IDX);
}

struct dtparsectx {
  u32 *ptr;
  u32 acells, scells; // shifting values of address-cells and size-cells
};
enum dttokentype {
  DT_TOKEN_BEGIN_NODE = 0x1,
  DT_TOKEN_END_NODE = 0x2,
  DT_TOKEN_PROP = 0x3,
  DT_TOKEN_NOP = 0x4,
  DT_TOKEN_END = 0x9,
};
struct dtparsetoken {
  enum dttokentype type;
  char *name;
  u32 *data;
  usize datalen;
  u32 acells, scells;
};

static void dtparsebegin(struct dtparsectx *ctx) {
  assert(dtgetprop(FDT_MAGIC_IDX) == FDT_MAGIC);
  assert(dtgetprop(FDT_VERSION_IDX) == FDT_VERSION); // only support version 17
  ctx->ptr = (u32 *)((void *)devtree + dtgetprop(FDT_STRUCTOFF_IDX));
  ctx->acells = ctx->scells = 0;
}
static int dtparse(struct dtparsectx *ctx, struct dtparsetoken *tok) {
  switch ((tok->type = be2cpu32(*ctx->ptr++))) {
  case DT_TOKEN_BEGIN_NODE:
    tok->name = (char *)ctx->ptr;
    // advance to next property
    ctx->ptr += 1 + strlen(tok->name) / sizeof(u32);

    tok->acells = ctx->acells & 0xf;
    tok->scells = ctx->scells & 0xf;
    ctx->acells <<= 4;
    ctx->scells <<= 4;
    break;
  case DT_TOKEN_END_NODE:
    ctx->acells >>= 4;
    ctx->scells >>= 4;
    break;
  case DT_TOKEN_PROP:
    tok->datalen = be2cpu32(*ctx->ptr++);
    tok->name = dtgetstring(be2cpu32(*ctx->ptr++));
    tok->data = ctx->ptr;

    // advance to next property
    ctx->ptr += 1 + (tok->datalen - 1) / sizeof(u32);
    // set acells or scells if we find them
    if (strcmp(tok->name, "#address-cells") == 0)
      ctx->acells += be2cpu32(*tok->data);
    else if (strcmp(tok->name, "#size-cells") == 0)
      ctx->scells += be2cpu32(*tok->data);
    break;
  case DT_TOKEN_NOP:
    break;
  case DT_TOKEN_END:
    return -1;
  }
  return 0;
}

static BOOL compatmatch(const void *data, usize datalen, const char *compat) {
  BOOL match;
  const char *cmp;
  const char *strlist = data;
  usize len = 0;

  while (len < datalen) {
    cmp = compat;
    match = TRUE;
    // compare each string and set match
    // we need to completely advance strlist to the next string too
    do {
      if (match && *strlist != *cmp++)
        match = FALSE;
      len++;
    } while (*strlist++);
    if (match)
      return TRUE;
  }
  return FALSE;
}
static usize usizecell(void *data, u32 cells) {
  if (cells * sizeof(u32) != sizeof(usize))
    return 0;
  // TODO: use define instead of if?
  if (sizeof(usize) == sizeof(u32))
    return be2cpu32(*(u32 *)data);
  return be2cpu64(*(u64 *)data);
}

usize dtgetmmio(const char *compat, void **addr) {
  struct dtparsectx iter;
  struct dtparsetoken tok;
  BOOL match = FALSE;
  usize size = 0;

  dtparsebegin(&iter);
  while (dtparse(&iter, &tok) == 0) {
    switch (tok.type) {
    case DT_TOKEN_BEGIN_NODE:
      // if we found a match but are here, there was no "reg" property to parse
      // so just return not found
      if (match)
        return 0;
      size = 0;
      break;
    case DT_TOKEN_PROP:
      if (strcmp(tok.name, "compatible") == 0 ||
          strcmp(tok.name, "device_type") == 0) {
        match = compatmatch(tok.data, tok.datalen, compat);
        // if size is non-zero, we already parsed "reg" and can return
        if (match && size != 0)
          return size;
      } else if (strcmp(tok.name, "reg") == 0) {
        *addr = (void *)usizecell(tok.data, tok.acells);
        size = usizecell(tok.data + tok.acells, tok.scells);
        // if this node is a match, we parsed the correct "reg" and can return
        if (match)
          return size;
      }
    default:
      break;
    }
  }

  // no match
  return 0;
}

void dtsysinit(void *dtb) { devtree = dtb; }
void dtinit(void) {
  // TODO: implement full device tree parsing
}
