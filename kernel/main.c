#include "kernel.h"
#include "riscv.h"
#include "picturedata.h"

void main(void)
{
    uartinit(); // must init serial before any printf
    printf("kernel sanders is starting...\n");
    allocinit();
    kvminit();
    trapinit();
    vgainit();

    vgasetfb(picturedata, picturedata_len);

    printf("clock: %p\n", *CLINT_MTIME);
    printf("KiB used:  %u\n", alloc * 4);
    printf("KiB avail: %u\n", allocmax * 4);

    panic("main return");
}
