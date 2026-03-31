#include "kernel.h"
#include "picturedata.h"

struct bmp_file_header {
  u16 type;
  u32 size;
  u16 reserved0;
  u16 reserved1;
  u32 offset;
} __attribute__((packed));
struct bmp_info_header {
  u32 hsize;
  i32 width, height;
  u16 cplanes;
  u16 bpp;
  u32 compression;
  u32 imgsize;
  i32 hres, vres;
  u32 palettesize;
  u32 impcolors;
} __attribute__((packed));

static usize bmp2bgr(u8 *bmp, u8 *bgr) {
  struct bmp_file_header *file = (struct bmp_file_header *)bmp;
  struct bmp_info_header *info =
      (struct bmp_info_header *)(bmp + sizeof(*file));
  assert(file->type == 0x4D42);   // type is BM (most common)
  assert(info->compression == 0); // BGR
  assert(info->bpp == 24);        // 24-bit (3 elements)

  // stride for a row in BMP is the width*elememnts padded to a multiple of 4
  usize stride = (info->width * 3 + 3) & ~3;
  for (i32 row = 0; row < info->height; row++) {
    u8 *src = (bmp + file->offset) + (info->height - 1 - row) * stride;
    u8 *dst = bgr + row * info->width * 3;
    memcpy(dst, src, info->width * 3);
  }
  return info->width * info->height * 3;
}

// kernel starts here
void init(void *dtb) {
  dtsysinit((const u32 *)dtb);
  uartinit(); // must init serial before any printf
  printf("kernel sanders is starting...\n");
  sysmeminit();
  kvminit();
  kallocinit();
  dtinit();
  vioblkinit();
  trapinit();
  vgainit();

  // read the entire block device
  usize nsectors = vioblkcap();
  usize blksz = vioblkblksz();
  u8 *diskbuf = kmallocalign(nsectors * blksz, blksz);
  vioblkrw(diskbuf, 0, nsectors, FALSE);
  printf("disk read complete: %u sectors\n", (u32)nsectors);

  // convert the block device data (bmp) to bgr
  struct bmp_info_header *info =
      (struct bmp_info_header *)(diskbuf + sizeof(struct bmp_file_header));
  usize bgrsz = info->width * info->height * 3;
  u8 *bgr = kmalloc(bgrsz);
  bmp2bgr(diskbuf, bgr);

  // use the bgr buffer as the vga output
  vgasetfb((u8 *)bgr, bgrsz);

  // free memory
  kfreealign(diskbuf);
  kfree(bgr);

  panic("init return");
}
