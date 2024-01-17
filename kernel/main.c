#include "types.h"
#include "printf.h"
#include "vga.h"
#include "picturedata.h"

void main(void)
{
    printf("kernel sanders is starting...\n");
    initvga();

    // enable supervisor hardware interrupts
    asm("csrw sie, %0" : : "r"(1 << 9));

    // enable global supervisor interrupts
    uint64_t status;
    asm("csrr %0, sstatus" : "=r"(status));
    status |= 1 << 1;
    asm("csrw sstatus, %0" : : "r"(status));

    for (usize_t i = 0; i < sizeof(picturedata) / sizeof(uint8_t); i++)
        vga_lset(i, picturedata[i]);

    while (1)
        ;
}
