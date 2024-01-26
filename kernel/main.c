#include "kernel.h"
#include "picturedata.h"

extern int alloc;

void main(void)
{
    printf("kernel sanders is starting...\n");
    allocinit();
    kvminit();
    trapinit();
    vgainit();

    for (usize_t i = 0; i < picturedata_len; i++)
        vga_lset(i, picturedata[i]);

    printf("KiB used: %u\n", alloc * 4);

    while (1)
        asm("wfi");
}
