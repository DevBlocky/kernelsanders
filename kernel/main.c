#include "kernel.h"
#include "picturedata.h"

// kernel starts here
void init(void *dtb) {
  dtsysinit(dtb);
  uartinit(); // must init serial before any printf
  printf("kernel sanders is starting...\n");
  sysmeminit();
  kvminit();
  kallocinit();
  dtinit();
  trapinit();
  vgainit();

  vgasetfb((u8 *)picturedata, picturedata_len);

  panic("init return");
}
