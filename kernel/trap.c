#include "printf.h"
#include "riscv.h"

uint64_t ticks = 0;

void _trapkernel(void);
void trapinit(void)
{
    w_stvec((usize_t)_trapkernel);
    w_sie(SIE_SSIE);
    intr_on();
}

static void sintr(usize_t cause)
{
    // software timer interrupt
    if (cause == 1)
    {
        ticks++;
        // clear pending interrupt flag
        w_sip(r_sip() & ~SIP_SSIP);
    }
}
void strap(void)
{
    usize_t scause = r_scause();
    if (scause & (1UL << 63))
        sintr(scause & 0xff);
    else
        panic("kernel exception");
}
