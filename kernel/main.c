#include "kernel.h"
#include "picturedata.h"

extern void *devtree;
// kernel starts here
void init(void *dtb) {
  devtree = dtb;
  // dtsysinit(dtb);
  uartinit(); // must init serial before any printf
  printf("dtb = %hp\n", dtb);
  printf("kernel sanders is starting...\n");
  sysmeminit();
  kvminit();
  kallocinit();
  trapinit();
  vgainit();

  vgasetfb((u8 *)picturedata, picturedata_len);

  panic("init return");
}
