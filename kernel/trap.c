#include "kernel.h"
#include "riscv.h"

#define UART0_IRQ 0x0a
#define PCI_IRQ1  0x20
#define PCI_IRQ2  0x21
#define PCI_IRQ3  0x22
#define PCI_IRQ4  0x23

uint64_t ticks;

// set a PLIC irq as enabled at a given priority
static void plic_intrset(int irq, int priority)
{
    // num of bytes and bits per PLIC register
    // 4 and 32 respectively
    const usize_t nbytes = sizeof(uint32_t);
    const usize_t nbits = nbytes * 8;

    // set the priority of irq
    const usize_t reg_priority = PLIC_MMIO + irq * nbytes;
    *(volatile int *)reg_priority = priority;

    // set irq as enabled
    const usize_t reg_senable = PLIC_MMIO + 0x2080 + nbytes * (irq / nbits);
    *(volatile uint32_t *)reg_senable |= 1 << (irq % nbits);
}
// set the PLIC threshold for hart 0
static void plic_thresholdset(int threshold)
{
    const usize_t reg_sthreshold = PLIC_MMIO + 0x201000;
    *(volatile int *)reg_sthreshold = threshold;
}
static int plic_claim(void)
{
    const usize_t reg_sclaim = PLIC_MMIO + 0x201004;
    return *(volatile int *)reg_sclaim;
}
static void plic_complete(int irq)
{
    const usize_t reg_sclaim = PLIC_MMIO + 0x201004;
    *(volatile int *)reg_sclaim = irq;
}

void _trapkernel(void);
void trapinit(void)
{
    ticks = 0;
    w_stvec((usize_t)_trapkernel);
    w_sie(SIE_SEIE | SIE_SSIE);
    intr_on();

    plic_intrset(UART0_IRQ, 1);
    plic_intrset(PCI_IRQ1, 1);
    plic_intrset(PCI_IRQ2, 1);
    plic_intrset(PCI_IRQ3, 1);
    plic_intrset(PCI_IRQ4, 1);
    plic_thresholdset(0);
}

static void devintr(void)
{
    uint32_t irq;
    while ((irq = plic_claim()))
    {
        switch (irq)
        {
        case 0xa:
            uartintr();
            break;
        default:
            printf("unknown irq: %hu\n", irq);
        }
        plic_complete(irq);
    }
}
static void sintr(usize_t cause)
{
    // software timer interrupt
    switch (cause)
    {
    case 1:
        ticks++;
        // clear pending interrupt flag
        w_sip(r_sip() & ~SIP_SSIP);
        break;
    case 9:
        devintr();
        break;
    default:
        panic("kernel interrupt");
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
