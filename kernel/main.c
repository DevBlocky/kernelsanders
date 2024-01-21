#include "types.h"
#include "printf.h"
#include "trap.h"
#include "vga.h"

#include "picturedata.h"

void main(void)
{
    printf("kernel sanders is starting...\n");
    trapinit();
    vgainit();

    for (usize_t i = 0; i < picturedata_len; i++)
        vga_lset(i, picturedata[i]);

    while (1)
        asm("wfi");
}
